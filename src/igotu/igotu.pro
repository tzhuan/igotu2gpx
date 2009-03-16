BASEDIR = ../..
CLEBS *= boost qextserialport pch builddll
unix:CLEBS *= libusb
TARGET = igotu
include($$BASEDIR/clebs.pri)

DEFINES *= IGOTU_MAKEDLL

SOURCES *= \
    commands.cpp \
    dataconnection.cpp \
    igotucommand.cpp \
    serialconnection.cpp \

unix:SOURCES *= libusbconnection.cpp

HEADERS *= \
    commands.h \
    dataconnection.h \
    exception.h \
    global.h \
    igotucommand.h \
    pch.h \
    serialconnection.h \

unix:HEADERS *= libusbconnection.h

unix:ctags.commands  = echo !_TAG_FILE_FORMAT 2 dummy > $$BASEDIR/tags;
unix:ctags.commands += echo !_TAG_FILE_SORTED 1 dummy >> $$BASEDIR/tags;
unix:ctags.commands += sed -i \'s/ /\\t/g\' $$BASEDIR/tags;
unix:ctags.commands += cd $$BASEDIR && ctags -R --c++-kinds=+p-n --fields=+iaS --extra=+fq --exclude=.build -f - src | sed \'s/rba:://g;s/\tnamespace:rba//g\' | LC_ALL=C sort >> tags;
win32:ctags.commands = cd $$BASEDIR && ctags -R --c++-kinds=+p-n --fields=+iaS --extra=+fq --exclude=.build  src
ctags.target = CTAGS
QMAKE_EXTRA_TARGETS *= ctags
ALL_DEPS *= CTAGS
