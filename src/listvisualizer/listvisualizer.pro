BASEDIR = ../..
CLEBS *= buildplugin visualizer libigotu
TARGET = listvisualizer

include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= listvisualizer.cpp
