CLEBS *= igotu
TARGET = igotu2gpx
include(../../../clebs.pri)

SOURCES *= $$files(*.cpp)
HEADERS *= $$files(*.h)

manual.files = igotu2gpx.1
manual.path = $$MANDIR/man1
INSTALLS *= manual

