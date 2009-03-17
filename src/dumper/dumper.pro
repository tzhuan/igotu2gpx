BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = dumper
include($$BASEDIR/clebs.pri)

SOURCES *= \
    dumper.cpp \
