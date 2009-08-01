clebsCheck(libusb10) {
    unix:!macx {
	system(pkg-config libusb-1.0):CLEBS_DEPENDENCIES *= libusb10
    }

    macx {
    }
}

clebsDependency(libusb10) {
    unix:!macx {
	PKGCONFIG *= libusb-1.0
    }

    macx {
	INCLUDEPATH *= $${LIBUSB10INCLUDEDIR}
	LIBS *= -lusb-1.0
    }
}
