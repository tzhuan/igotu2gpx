CLEBS_REQUIRED *= qtversion430 boost-po
CLEBS_SUGGESTED *= libmarble
unix:CLEBS_REQUIRED *= libusb
unix:!macx:CLEBS_SUGGESTED *= chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

clebsDirs(src/igotu)
clebsDirs(src/igotu2gpx, src/igotu)
clebsDirs(src/igotugui, src/igotu)

clebsDirs(src/marblevisualizer, src/igotu, marblevisualizer)
clebsDirs(src/listvisualizer, src/igotu)

clebsDirs(data)
clebsDirs(contrib/tango)

docs.files = LICENSE HACKING
docs.path = $$DOCDIR
INSTALLS *= docs

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.pro src/*/*.h src/*/*.cpp
QMAKE_EXTRA_TARGETS *= todo
