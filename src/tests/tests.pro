CLEBS *= igotu
TARGET = tester
include(../../clebs.pri)

CONFIG += qtestlib

SOURCES *= $$files(*.cpp)
HEADERS *= $$files(*.h)
