clebsCheck(libusb10) {
    unix {
	system(pkg-config libusb-1.0):CLEBS_DEPENDENCIES *= libusb10
    }
}

clebsDependency(libusb10) {
    unix {
	PKGCONFIG *= libusb-1.0
    }
}
