CLEBS *= igotu +boost-po
TARGET = igotu2gpx
include(../../../clebs.pri)

SOURCES *= \
    igotu2gpx.cpp \
    mainobject.cpp \

HEADERS *= \
    mainobject.h \

manual.files = igotu2gpx.1
manual.path = $$MANDIR/man1
INSTALLS *= manual

