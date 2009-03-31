CLEBS_REQUIRED *= boost-po 
unix:CLEBS_REQUIRED *= libusb
unix:CLEBS_SUGGESTED *= chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

clebsDirs(src/igotu)
clebsDirs(src/igotu2gpx, src/igotu)

license.files = LICENSE
license.path = $$DOCDIR
INSTALLS *= license

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.cpp src/*/*.h
