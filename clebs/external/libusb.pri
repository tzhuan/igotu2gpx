clebsCheck(libusb) {
    unix {
	system(pkg-config libusb):CLEBS_DEPENDENCIES *= libusb
    }
}

clebsDependency(libusb) {
    unix {
	PKGCONFIG *= libusb
    }
}
