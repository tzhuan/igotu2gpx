clebsCheck(dataconnection) {
    CLEBS_DEPENDENCIES *= dataconnection
}

clebsDependency(dataconnection) {
    INCLUDEPATH *= $$BASEDIR/src/include
    DEPENDPATH *= $$BASEDIR/src/include $$BASEDIR/src/lib/igotu
}
