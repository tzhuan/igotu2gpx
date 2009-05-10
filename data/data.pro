BASEDIR = ..
include($$BASEDIR/clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

app22icons.files = icons/22x22/apps/*
app22icons.path = $$ICONDIR/22x22/apps
INSTALLS *= app22icons
app32icons.files = icons/32x32/apps/*
app32icons.path = $$ICONDIR/32x32/apps
INSTALLS *= app32icons
app48icons.files = icons/48x48/apps/*
app48icons.path = $$ICONDIR/48x48/apps
INSTALLS *= app48icons
app64icons.files = icons/64x64/apps/*
app64icons.path = $$ICONDIR/64x64/apps
INSTALLS *= app64icons
