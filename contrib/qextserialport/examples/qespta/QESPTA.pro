BASEDIR = ../../../..
CLEBS *= qextserialport

include($$BASEDIR/clebs.pri)

QT *= gui

TARGET = QESPTA

SOURCES *= main.cpp MainWindow.cpp MessageWindow.cpp QespTest.cpp
HEADERS *= MainWindow.h MessageWindow.h QespTest.h
