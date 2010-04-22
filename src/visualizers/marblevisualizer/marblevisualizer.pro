CLEBS *= buildplugin visualizer igotu libmarble
CLEBS_INSTALL *= libmarble
TARGET = marblevisualizer
include(../../../clebs.pri)

QT *= gui

SOURCES *= marblevisualizer.cpp
