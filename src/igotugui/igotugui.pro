BASEDIR = ../..
CLEBS *= libigotu boost-po libmarble
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui

win32:LIBS *= -lshell32

RC_FILE = $$BASEDIR/data/windows/icon.rc

SOURCES *= \
    iconstorage.cpp \
    igotugui.cpp \
    mainwindow.cpp \
    paths.cpp \
    preferencesdialog.cpp \
    qticonloader.cpp \

HEADERS *= \
    iconstorage.h \
    mainwindow.h \
    paths.h \
    preferencesdialog.h \
    qticonloader.h \

FORMS *= \
    igotugui.ui \
    preferencesdialog.ui \

unix {
    desktopfile.files = igotugui.desktop
    desktopfile.path = $$APPDIR
    INSTALLS *= desktopfile
}

