BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui network

win32:LIBS *= -lshell32

RC_FILE = $$BASEDIR/data/windows/icon.rc

SOURCES *= \
    iconstorage.cpp \
    igotugui.cpp \
    mainwindow.cpp \
    plugindialog.cpp \
    preferencesdialog.cpp \
    updatenotification.cpp \
    qticonloader.cpp \

HEADERS *= \
    iconstorage.h \
    mainwindow.h \
    plugindialog.h \
    preferencesdialog.h \
    updatenotification.h \
    qticonloader.h \

FORMS *= \
    igotugui.ui \
    plugindialog.ui \
    preferencesdialog.ui \

unix {
    desktopfile.files = igotugui.desktop
    desktopfile.path = $$APPDIR
    INSTALLS *= desktopfile
}

manual.files = igotugui.1
manual.path = $$MANDIR/man1
INSTALLS *= manual
