CLEBS_REQUIRED *= boost-po libusb
unix:CLEBS_SUGGESTED *= chrpath

include(clebs.pri)

OBJECTS_DIR =
TEMPLATE = subdirs

clebsDirs(src/igotu)
clebsDirs(src/dumper, src/igotu)
clebsDirs(src/info, src/igotu)

license.files = LICENSE
license.path = $$DOCDIR
INSTALLS *= license

doxygen.commands = doxygen doc/Doxyfile > /dev/null
doxytag.commands = doxytag -t doc/qt4.tag $(QTDIR)/doc/html
todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.cpp src/*/*.h src/*/*/*.cpp src/*/*/*.h
test.commands = @echo [tester] Entering dir "\\'src/tests\\'" && $$DESTDIR/tester -silent
QMAKE_EXTRA_TARGETS *= doxygen doxytag todo test
