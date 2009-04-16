CLEBS_REQUIRED *= boost-po
unix:CLEBS_REQUIRED *= libusb
unix:CLEBS_SUGGESTED *= chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

clebsDirs(src/igotu)
clebsDirs(src/igotu2gpx, src/igotu)

docs.files = LICENSE HACKING
docs.path = $$DOCDIR
INSTALLS *= docs

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.cpp src/*/*.h
