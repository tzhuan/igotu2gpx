CLEBS_REQUIRED *= qtversion440 boost-po
unix:CLEBS_REQUIRED *= libusb
unix:!macx:CLEBS_SUGGESTED *= chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

clebsDirs(src/igotu)
clebsDirs(src/igotu2gpx, src/igotu)
clebsDirs(src/igotugui, src/igotu)
clebsDirs(data)

docs.files = LICENSE HACKING
docs.path = $$DOCDIR
INSTALLS *= docs

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.cpp src/*/*.h
QMAKE_EXTRA_TARGETS *= todo
