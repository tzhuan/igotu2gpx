CLEBS *= buildplugin visualizer igotu
TARGET = listvisualizer
include(../../../clebs.pri)

QT *= gui

SOURCES *= $$files(*.cpp)
