BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui

win32:LIBS *= -lshell32

# TODO: windows icon file

SOURCES *= \
    iconstorage.cpp \
    igotugui.cpp \
    mainwindow.cpp \
    paths.cpp \
    preferencesdialog.cpp \
    qticonloader.cpp \
    waitdialog.cpp \

HEADERS *= \
    iconstorage.h \
    mainwindow.h \
    paths.h \
    preferencesdialog.h \
    qticonloader.h \
    waitdialog.h \

FORMS *= \
    igotugui.ui \
    preferencesdialog.ui \
    waitdialog.ui \

unix {
    desktopfile.files = igotugui.desktop
    desktopfile.path = $$APPDIR
    INSTALLS *= desktopfile
}

