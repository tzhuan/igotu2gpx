BASEDIR = ../..
CLEBS *= buildplugin dataconnection libigotu
TARGET = serialconnection

include($$BASEDIR/clebs.pri)

SOURCES *= serialconnection.cpp
