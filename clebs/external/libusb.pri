clebsCheck(libusb) {
    unix:!macx {
	system(pkg-config libusb):CLEBS_DEPENDENCIES *= libusb
    }

    macx {
	isEmpty(LIBUSBINCLUDEDIR):LIBUSBINCLUDEDIR = /usr/local/include

	exists($${LIBUSBINCLUDEDIR}/usb.h):CLEBS_DEPENDENCIES *= libusb
    }
}

clebsDependency(libusb) {
    unix:!macx {
	PKGCONFIG *= libusb
    }

    macx {
	INCLUDEPATH *= $${LIBUSBINCLUDEDIR}
	LIBS *= -lusb
    }
}
