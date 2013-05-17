CLEBS *= igotu
TARGET = igotugui
include(../../../clebs.pri)

QT *= gui network

win32:LIBS *= -lshell32

RC_FILE = $$BASEDIR/data/windows/icon.rc

SOURCES *= $$files(*.cpp)
HEADERS *= $$files(*.h)
FORMS *= $$files(*.ui)

unix {
    desktopfile.files = igotugui.desktop
    desktopfile.path = $$APPDIR
    INSTALLS *= desktopfile
}

manual.files = igotugui.1
manual.path = $$MANDIR/man1
INSTALLS *= manual
