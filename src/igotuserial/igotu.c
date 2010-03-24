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
	atomic_t write_count;		/* number of write urbs pending */
	struct urb *write_control_urb;
	unsigned char *write_control_buffer;
	struct usb_ctrlrequest *write_control_cr;
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
	struct usb_serial_port *port = urb->context;
	int result;
	unsigned char *data = urb->transfer_buffer;
	struct tty_struct *tty;
	int status = urb->status;

	switch (status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(&port->dev,
				"%s - urb shutting down with status: %d",
				__func__, status);
		return;
	default:
		dev_dbg(&port->dev,
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
	result = usb_submit_urb(urb, GFP_ATOMIC);
	if (result) {
		dev_err(&port->dev,
				"%s - error %d submitting interrupt urb\n",
				__func__, result);
		return;
	}
}

static void igotu_write_control_callback(struct urb *urb)
{
	struct usb_serial_port *port = urb->context;
	struct igotu_private *priv = usb_get_serial_port_data(port);
	int result;
	int status = urb->status;

	atomic_dec(&priv->write_count);

	switch (status) {
	case 0:
		/* success */
		break;
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		/* this urb is terminated, clean up */
		dev_dbg(&port->dev,
				"%s - urb shutting down with status: %d",
				__func__, status);
		return;
	default:
		dev_dbg(&port->dev,
				"%s - nonzero urb status received: %d",
				__func__, status);
		goto submit;
	}

	dev_dbg(&port->dev, "%s", __func__);

submit:
	result = usb_submit_urb(port->interrupt_in_urb, GFP_ATOMIC);
	if (result) {
		dev_err(&port->dev,
				"%s - error %d submitting interrupt urb\n",
				__func__, result);
		return;
	}
}

static int igotu_attach(struct usb_serial *serial)
{
	struct usb_serial_port *port = serial->port[0];
	struct igotu_private *priv;

	dev_dbg(&port->dev, "%s", __func__);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		goto error;

	priv->write_control_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!priv->write_control_urb)
		goto error;

	priv->write_control_buffer = kmalloc(8, GFP_KERNEL);
	if (!priv->write_control_buffer)
		goto error;

	priv->write_control_cr = kzalloc(sizeof(*priv->write_control_cr),
			GFP_KERNEL);
	if (!priv->write_control_cr)
		goto error;

	atomic_set(&priv->write_count, 0);

	usb_set_serial_port_data(port, priv);

	return 0;

error:
	if (priv) {
		kfree(priv->write_control_cr);
		kfree(priv->write_control_buffer);
		usb_free_urb(priv->write_control_urb);
	}
	kfree(priv);
	return -ENOMEM;
}

static void igotu_release(struct usb_serial *serial)
{
	struct usb_serial_port *port = serial->port[0];
	struct igotu_private *priv = usb_get_serial_port_data(port);

	dev_dbg(&port->dev, "%s", __func__);

	kfree(priv->write_control_cr);
	kfree(priv->write_control_buffer);
	usb_free_urb(priv->write_control_urb);
	kfree(priv);
	usb_set_serial_port_data(port, NULL);
}

static int igotu_open(struct tty_struct *tty,
			struct usb_serial_port *port, struct file *filp)
{
	struct igotu_private *priv = usb_get_serial_port_data(port);
	int result = 0;

	dev_dbg(&port->dev, "%s", __func__);

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

	priv->write_control_cr->bRequestType = 0x21;
	priv->write_control_cr->bRequest = 0x09;
	priv->write_control_cr->wValue = cpu_to_le16(0x0200);
	priv->write_control_cr->wIndex = 0;
	priv->write_control_cr->wLength = 0;

	usb_fill_control_urb(priv->write_control_urb, port->serial->dev,
			usb_sndctrlpipe(port->serial->dev, 0),
			(char *) priv->write_control_cr,
			priv->write_control_buffer, 0,
			igotu_write_control_callback, port);

	if (port->interrupt_in_urb) {
		dev_dbg(&port->dev, "%s - adding interrupt input", __func__);
		result = usb_submit_urb(port->interrupt_in_urb, GFP_KERNEL);
		if (result) {
			dev_err(&port->dev,
				"%s - error %d submitting interrupt urb\n",
				__func__, result);
		}
	}

	return result;
}

static void igotu_close(struct usb_serial_port *port)
{
	struct igotu_private *priv = usb_get_serial_port_data(port);

	dev_dbg(&port->dev, "%s", __func__);

	usb_kill_urb(priv->write_control_urb);
	usb_kill_urb(port->interrupt_in_urb);
}

static int igotu_write(struct tty_struct *tty, struct usb_serial_port *port,
			const unsigned char *buf, int count)
{
	struct igotu_private *priv = usb_get_serial_port_data(port);
	int result;

	dev_dbg(&port->dev, "%s", __func__);

	if (!atomic_add_unless(&priv->write_count, 1, 1)) {
		dev_dbg(&port->dev, "%s - write urb already pending", __func__);
		return 0;
	}

	usb_kill_urb(port->interrupt_in_urb);

	count = min(8, count);
	memcpy(priv->write_control_buffer, buf, count);

	priv->write_control_urb->transfer_buffer_length = count;
	priv->write_control_cr->wLength = cpu_to_le16(count);
	result = usb_submit_urb(priv->write_control_urb, GFP_KERNEL);

	if (result) {
		atomic_dec(&priv->write_count);
		dev_err(&port->dev,
			"%s - error %d submitting control urb\n",
			__func__, result);
		count = 0;
	}

	usb_serial_debug_data(debug, &port->dev, __func__, count,
			priv->write_control_buffer);

	return count;
}

int igotu_write_room(struct tty_struct *tty)
{
	struct usb_serial_port *port = tty->driver_data;
	struct igotu_private *priv = usb_get_serial_port_data(port);
	int room;

	dev_dbg(&port->dev, "%s", __func__);

	room = atomic_read(&priv->write_count) ? 0 : 8;

	dev_dbg(&port->dev, "%s - room for %d bytes",
			__func__, room);

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
	.attach =		igotu_attach,
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
