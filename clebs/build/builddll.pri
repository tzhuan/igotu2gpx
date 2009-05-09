clebsCheck(builddll) {
    CLEBS_DEPENDENCIES *= builddll
}

clebsDependency(builddll) {
    unix:!novisibility:CONFIG += hide_symbols
    CONFIG += dll
    TEMPLATE = lib
    target.path = $$LIBDIR
    INSTALLS *= target
}
