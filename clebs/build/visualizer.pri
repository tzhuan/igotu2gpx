clebsCheck(visualizer) {
    CLEBS_DEPENDENCIES *= visualizer
}

clebsDependency(visualizer) {
    INCLUDEPATH *= $$BASEDIR/src/include
    DEPENDPATH *= $$BASEDIR/src/include $$BASEDIR/src/programs/igotugui
}
