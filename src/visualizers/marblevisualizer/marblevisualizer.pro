CLEBS *= buildplugin visualizer igotu libmarble
CLEBS_INSTALL *= libmarble
TARGET = marblevisualizer
include(../../../clebs.pri)

# Work around a bug in MarbleModel.h
DEFINES -= QT_NO_CAST_FROM_ASCII

QT *= gui

SOURCES *= $$files(*.cpp)
