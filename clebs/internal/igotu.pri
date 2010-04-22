clebsCheck(igotu) {
    CLEBS_DEPENDENCIES *= igotu
}

clebsDependency(igotu) {
    LIBS *= -ligotu -L$$DESTDIR

    CLEBS *= boost -chrpath

    INCLUDEPATH *= $$BASEDIR/src/lib
    DEPENDPATH *= $$BASEDIR/src/lib
}
