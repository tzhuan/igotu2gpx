BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotugui
include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= \
    igotugui.cpp \
    mainwindow.cpp \

HEADERS *= \
    mainwindow.h \

FORMS *= \
    igotugui.ui
