/*
 * Mobileaction i-gotU Travel Logger Serial USB driver
 *
 * Copyright (C) 2009 Michael Hofmann <mh21@piware.de>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>

static int debug;

struct igotu_private {
	spinlock_t writelock;		/* write urb lock */
	unsigned writecount;		/* write urb pending */

	spinlock_t readlock;		/* read urb lock */
	unsigned readcount;		/* read urb pending */
};

static struct usb_device_id id_table[] = {
	{ USB_DEVICE(0x0df7, 0x0900) },	/* Mobile Action Technology Inc. MA
					   i-gotU Travel Logger GPS */
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver igotu_driver = {
	.name =			"igotu",
	.probe =		usb_serial_probe,
	.disconnect =		usb_serial_disconnect,
	.id_table =		id_table,
	.no_dynamic_id =	1,
};

static void igotu_read_int_callback(struct urb *urb)
{
	struct igotu_private *priv;
	struct usb_serial_port *port = urb->context;
	int result;
	unsigned long flags;
	unsigned char *data = urb->transfer_buffer;
	struct tty_struct *tty;
	int status = urb->status;

	priv = usb_get_serial_port_data(port);

	spin_lock_irqsave(&priv->readlock, flags);
	--priv->readcount;
	spin_unlock_irqrestore(&priv->readlock, flags);

	switch (status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(&urb->dev->dev,
				"%s - urb shutting down with status: %d",
				__func__, status);
		return;
	default:
		dev_dbg(&urb->dev->dev,
				"%s - nonzero urb status received: %d",
				__func__, status);
		goto submit;
	}

	usb_serial_debug_data(debug, &port->dev, __func__, urb->actual_length,
			data);

	tty = tty_port_tty_get(&port->port);
	if (tty && urb->actual_length) {
		tty_buffer_request_room(tty, urb->actual_length);
		tty_insert_flip_string(tty, data, urb->actual_length);
		tty_flip_buffer_push(tty);
	}
	tty_kref_put(tty);

submit:
	spin_lock_irqsave(&priv->readlock, flags);
	if (priv->readcount) {
		spin_unlock_irqrestore(&priv->readlock, flags);
		return;
	}
	++priv->readcount;
	spin_unlock_irqrestore(&priv->readlock, flags);

	result = usb_submit_urb(urb, GFP_ATOMIC);
	if (result) {
		dev_err(&urb->dev->dev,
				"%s - Error %d submitting interrupt urb\n",
				__func__, result);
		return;
	}
}

static int igotu_startup(struct usb_serial *serial)
{
	struct igotu_private *priv;
	struct usb_serial_port *port = serial->port[0];

	dev_dbg(&serial->dev->dev, "%s - port %d", __func__, port->number);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		goto error;

	spin_lock_init(&priv->writelock);
	spin_lock_init(&priv->readlock);

	/* TODO needed? */
	usb_reset_configuration(serial->dev);

	usb_set_serial_port_data(port, priv);

	return 0;

error:
	kfree(priv);
	return -ENOMEM;
}

static void igotu_release(struct usb_serial *serial)
{
	struct usb_serial_port *port = serial->port[0];

	dev_dbg(&serial->dev->dev, "%s - port %d", __func__, port->number);

	kfree(usb_get_serial_port_data(port));
	usb_set_serial_port_data(port, NULL);
}

static int igotu_open(struct tty_struct *tty,
			struct usb_serial_port *port, struct file *filp)
{
	struct igotu_private *priv;
	int result = 0;

	dev_dbg(&port->dev, "%s - port %d", __func__, port->number);

	/* similar to the USB Cypress M8 driver */

	tty->termios->c_iflag	/* input modes */
		&= ~(IGNBRK	/* disable ignore break */
		| BRKINT	/* disable break causes interrupt */
		| PARMRK	/* disable mark parity errors */
		| ISTRIP	/* disable clear high bit of input char */
		| INLCR		/* disable translate NL to CR */
		| IGNCR		/* disable ignore CR */
		| ICRNL		/* disable translate CR to NL */
		| IXON);	/* disable enable XON/XOFF flow control */

	tty->termios->c_oflag	/* output modes */
		&= ~OPOST;	/* disable postprocess output char */

	tty->termios->c_lflag	/* line discipline modes */
		&= ~(ECHO	/* disable echo input characters */
		| ECHONL	/* disable echo new line */
		| ICANON	/* disable erase, kill, werase, and rprnt
				   special characters */
		| ISIG		/* disable interrupt, quit, and suspend special
				   characters */
		| IEXTEN);	/* disable non-POSIX special characters */

	if (port->interrupt_in_urb) {
		dev_dbg(&port->dev, "%s - adding interrupt input", __func__);
		priv = usb_get_serial_port_data(port);
		++priv->readcount;
		result = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
		if (result) {
			--priv->readcount;
			dev_err(&port->dev,
				"%s - Error %d submitting interrupt urb\n",
				__func__, result);
		}
	}
	return result;
}

static void igotu_close(struct usb_serial_port *port)
{
	struct igotu_private *priv;

	dev_dbg(&port->dev, "%s - port %d", __func__, port->number);

	priv = usb_get_serial_port_data(port);

	usb_kill_urb(port->interrupt_in_urb);
}

static int igotu_write(struct tty_struct *tty, struct usb_serial_port *port,
			const unsigned char *buf, int count)
{
	struct igotu_private *priv;
	int result;
	unsigned long flags;
	bool pendingread;

	dev_dbg(&port->dev, "%s - port %d", __func__, port->number);

	if (count == 0) {
		dev_dbg(&port->dev, "%s - port %d: empty write",
				__func__, port->number);
		return 0;
	}

	priv = usb_get_serial_port_data(port);

	spin_lock_irqsave(&priv->writelock, flags);
	if (priv->writecount) {
		dev_dbg(&port->dev, "%s - port %d: urb already pending",
				__func__, port->number);
		spin_unlock_irqrestore(&priv->writelock, flags);
		return 0;
	}
	++priv->writecount;
	spin_unlock_irqrestore(&priv->writelock, flags);

	/* Cancel any existing read urb and prevent the scheduling of another */
	spin_lock_irqsave(&priv->readlock, flags);
	pendingread = priv->readcount;
	++priv->readcount;
	spin_unlock_irqrestore(&priv->readlock, flags);
	if (pendingread)
		usb_kill_urb(port->interrupt_in_urb);

	/* TODO: wait for the urb to be killed */

	result = usb_control_msg(port->serial->dev,
			usb_sndctrlpipe(port->serial->dev, 0),
			0x09, 0x21, 0x0200, 0, (void *) buf, count, 100);

	spin_lock_irqsave(&priv->writelock, flags);
	--priv->writecount;
	spin_unlock_irqrestore(&priv->writelock, flags);

	if (result != count) {
		dev_err(&port->dev,
			"%s - Error %d submitting control urb\n",
			__func__, result);
		count = 0;
	}

	/* Resubmit the read urb */
	if (port->interrupt_in_urb) {
		result = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
		if (result) {
			dev_err(&port->dev,
				"%s - Error %d submitting interrupt urb\n",
				__func__, result);
		}
	} else {
		result = 1;
	}

	if (result) {
		spin_lock_irqsave(&priv->readlock, flags);
		--priv->readcount;
		spin_unlock_irqrestore(&priv->readlock, flags);
	}

	dev_dbg(&port->dev, "%s - port %d: %d bytes sent",
			__func__, port->number, count);

	return count;
}

int igotu_write_room(struct tty_struct *tty)
{
	struct igotu_private *priv;
	struct usb_serial_port *port = tty->driver_data;
	unsigned long flags;
	int room;

	dev_dbg(&port->dev, "%s - port %d", __func__, port->number);

	priv = usb_get_serial_port_data(port);

	spin_lock_irqsave(&priv->writelock, flags);
	room = priv->writecount ? 0 : 8;
	spin_unlock_irqrestore(&priv->writelock, flags);

	dev_dbg(&port->dev, "%s - port %d: room for %d bytes",
			__func__, port->number, room);

	return room;
}

static struct usb_serial_driver igotu_device = {
	.driver = {
		.owner =	THIS_MODULE,
		.name =		"igotu",
	},
	.id_table =		id_table,
	.usb_driver =		&igotu_driver,
	.num_ports =		1,
	.attach =		igotu_startup,
	.release =		igotu_release,
	.open =			igotu_open,
	.close =		igotu_close,
	.write =		igotu_write,
	.write_room =		igotu_write_room,
	.read_int_callback =	igotu_read_int_callback,
};

static int __init igotu_init(void)
{
	int retval;

	retval = usb_serial_register(&igotu_device);
	if (retval)
		return retval;
	retval = usb_register(&igotu_driver);
	if (retval)
		usb_serial_deregister(&igotu_device);
	return retval;
}

static void __exit igotu_exit(void)
{
	usb_deregister(&igotu_driver);
	usb_serial_deregister(&igotu_device);
}

module_init(igotu_init);
module_exit(igotu_exit);
MODULE_LICENSE("GPL");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug enabled or not");
/* TODO: remove for submission vi: set noet ts=8 sts=8 sw=8: */
