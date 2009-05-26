BASEDIR = ../..
include($$BASEDIR/clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

actions16icons.files = icons/16x16/actions/*
actions16icons.path = $$ICONDIR/16x16/actions
INSTALLS *= actions16icons
status16icons.files = icons/16x16/status/*
status16icons.path = $$ICONDIR/16x16/status
INSTALLS *= status16icons
actions22icons.files = icons/22x22/actions/*
actions22icons.path = $$ICONDIR/22x22/actions
INSTALLS *= actions22icons
status22icons.files = icons/22x22/status/*
status22icons.path = $$ICONDIR/22x22/status
INSTALLS *= status22icons
actions24icons.files = icons/24x24/actions/*
actions24icons.path = $$ICONDIR/24x24/actions
INSTALLS *= actions24icons
status24icons.files = icons/24x24/status/*
status24icons.path = $$ICONDIR/24x24/status
INSTALLS *= status24icons
actions32icons.files = icons/32x32/actions/*
actions32icons.path = $$ICONDIR/32x32/actions
INSTALLS *= actions32icons
status32icons.files = icons/32x32/status/*
status32icons.path = $$ICONDIR/32x32/status
INSTALLS *= status32icons
