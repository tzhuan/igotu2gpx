clebsCheck(fileexporter) {
    CLEBS_DEPENDENCIES *= fileexporter
}

clebsDependency(fileexporter) {
    INCLUDEPATH *= $$BASEDIR/src/include
    DEPENDPATH *= $$BASEDIR/src/include $$BASEDIR/src/lib/igotu
}
