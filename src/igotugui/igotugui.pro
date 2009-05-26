BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= \
    iconstorage.cpp \
    igotugui.cpp \
    mainwindow.cpp \
    paths.cpp \
    qticonloader.cpp \
    waitdialog.cpp \

HEADERS *= \
    iconstorage.h \
    mainwindow.h \
    paths.h \
    qticonloader.h \
    waitdialog.h \

FORMS *= \
    igotugui.ui \
    waitdialog.ui \

unix {
    desktopfile.files = igotugui.desktop
    desktopfile.path = $$APPDIR
    INSTALLS *= desktopfile
}

