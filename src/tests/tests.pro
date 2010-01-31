BASEDIR = ../..
CLEBS *= libigotu boost-po
TARGET = tester
include($$BASEDIR/clebs.pri)

CONFIG += qtestlib

SOURCES *= \
    igotuconfig.cpp \
    tests.cpp \

HEADERS *= \
    tests.h \
