BASEDIR = ../..
CLEBS *= builddll

include($$BASEDIR/clebs.pri)

TARGET = igotuserialport

HEADERS *= \
	qextserialbase.h \
	qextserialport.h \
	qextserialenumerator.h \

SOURCES *= \
	qextserialbase.cpp \
	qextserialport.cpp \
	qextserialenumerator.cpp

unix:HEADERS *= posix_qextserialport.h
unix:SOURCES *= posix_qextserialport.cpp
unix:DEFINES *= _TTY_POSIX_

win32:HEADERS *= win_qextserialport.h
win32:SOURCES *= win_qextserialport.cpp
win32:DEFINES *= _TTY_WIN_

win32:LIBS *= -lsetupapi -ladvapi32

DEFINES *= QEXTSERIALPORT_MAKEDLL

unix:VERSION = 1.2.0
