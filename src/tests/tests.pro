CLEBS *= igotu boost-po
TARGET = tester
include(../../clebs.pri)

CONFIG += qtestlib

SOURCES *= \
    igotuconfig.cpp \
    tests.cpp \

HEADERS *= \
    tests.h \
