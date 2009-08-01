BASEDIR = ../..
CLEBS *= buildplugin dataconnection libusb10 libigotu
TARGET = usb10connection

include($$BASEDIR/clebs.pri)

SOURCES *= libusb10connection.cpp

