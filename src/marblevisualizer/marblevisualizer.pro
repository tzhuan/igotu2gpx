BASEDIR = ../..
CLEBS *= buildplugin visualizer libigotu libmarble
TARGET = marblevisualizer

include($$BASEDIR/clebs.pri)

QT *= gui

SOURCES *= marblevisualizer.cpp
