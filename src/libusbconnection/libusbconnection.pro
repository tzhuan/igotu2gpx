BASEDIR = ../..
CLEBS *= buildplugin dataconnection libusb libigotu
TARGET = usbconnection

include($$BASEDIR/clebs.pri)

SOURCES *= libusbconnection.cpp

