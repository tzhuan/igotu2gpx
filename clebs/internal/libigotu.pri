clebsCheck(libigotu) {
    CLEBS_DEPENDENCIES *= libigotu
}

clebsDependency(libigotu) {
    LIBS *= -ligotu -L$$DESTDIR

    CLEBS *= boost chrpath

    INCLUDEPATH *= $$BASEDIR/src
    DEPENDPATH *= $$BASEDIR/src
}
