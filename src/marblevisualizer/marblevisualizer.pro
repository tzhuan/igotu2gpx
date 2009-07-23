BASEDIR = ../..
CLEBS *= buildplugin visualizer libigotu libmarble
CLEBS_INSTALL *= libmarble
TARGET = marblevisualizer

include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= marblevisualizer.cpp
