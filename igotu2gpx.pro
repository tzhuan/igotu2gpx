CLEBS_REQUIRED *= qtversion430 boost-po
CLEBS_SUGGESTED *= libmarble
unix:CLEBS_SUGGESTED *=  libusb
unix:!macx:CLEBS_SUGGESTED *= libusb10 chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

!clebsCheckModulesOne(serialconnection libusbconnection libusb10connection):error("No device drivers found, please install either libusb or libusb-1.0 (alpha quality driver)")

clebsDirs(src/igotu)
clebsDirs(src/igotu2gpx, src/igotu)
clebsDirs(src/igotugui, src/igotu)

clebsDirs(src/marblevisualizer, src/igotu, marblevisualizer)
clebsDirs(src/listvisualizer, src/igotu)

clebsDirs(src/libusbconnection, src/igotu, libusbconnection)
clebsDirs(src/libusb10connection, src/igotu, libusb10connection)
clebsDirs(src/serialconnection, src/igotu, serialconnection)

clebsDirs(data)
clebsDirs(contrib/tango)

docs.files = LICENSE HACKING
docs.path = $$DOCDIR
INSTALLS *= docs

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.pro src/*/*.h src/*/*.cpp
QMAKE_EXTRA_TARGETS *= todo

pocheck.commands = lupdate src -ts translations/igotu2gpx.ts; lconvert translations/igotu2gpx.ts -o translations/igotu2gpx.pot -of po; msgfmt -c translations/igotu2gpx.pot -o translations/igotu2gpx.mo
QMAKE_EXTRA_TARGETS *= pocheck
