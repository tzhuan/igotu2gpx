BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = igotu2gpx
include($$BASEDIR/clebs.pri)

SOURCES *= \
    igotu2gpx.cpp \

manual.files = igotu2gpx.1
manual.path = $$MANDIR/man1
INSTALLS *= manual

