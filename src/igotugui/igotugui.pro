BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= \
    igotugui.cpp \
    mainwindow.cpp \
    waitdialog.cpp \

HEADERS *= \
    mainwindow.h \
    waitdialog.h \

FORMS *= \
    igotugui.ui \
    waitdialog.ui \
