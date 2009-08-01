BASEDIR = ../..
CLEBS *= buildplugin dataconnection libigotu
TARGET = libserialconnection

include($$BASEDIR/clebs.pri)

SOURCES *= serialconnection.cpp
