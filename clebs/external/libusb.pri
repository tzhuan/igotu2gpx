clebsCheck(libusb) {
    unix {
	system(pkg-config libusb):CLEBS_DEPENDENCIES *= libusb
    }

    macx {
	isEmpty(LIBUSBINCLUDEDIR):LIBUSBINCLUDEDIR = /usr/local/include

	exists($${LIBUSBINCLUDEDIR}/usb.h):CLEBS_DEPENDENCIES *= libusb
    }
}

clebsDependency(libusb) {
    unix {
	PKGCONFIG *= libusb
    }

    macx {
	INCLUDEPATH *= $${LIBUSBINCLUDEDIR}
	LIBS *= -lusb
    }
}
