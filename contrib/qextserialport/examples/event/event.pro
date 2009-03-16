BASEDIR = ../../../..
CLEBS *= qextserialport

include($$BASEDIR/clebs.pri)

TARGET = event

SOURCES *= main.cpp PortListener.cpp
HEADERS *= PortListener.h
