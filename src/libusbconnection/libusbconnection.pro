BASEDIR = ../..
CLEBS *= buildplugin dataconnection libusb libigotu
TARGET = libusbconnection

include($$BASEDIR/clebs.pri)

SOURCES *= libusbconnection.cpp

