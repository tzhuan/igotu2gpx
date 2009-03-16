clebsCheck(qextserialport) {
    CLEBS_DEPENDENCIES *= qextserialport
}

clebsDependency(qextserialport) {
    LIBS *= -lqextserialport -L$$DESTDIR

    CLEBS *= chrpath

    INCLUDEPATH *= $$BASEDIR/contrib
    DEPENDPATH *= $$BASEDIR/contrib

    unix:DEFINES *= _TTY_POSIX_
    win32:DEFINES *= _TTY_WIN_
}
