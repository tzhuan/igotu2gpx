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

todo.commands = @grep -Rn '\'TODO\|FIXME\|XXX\|\\todo\'' src/*/*.pro src/*/*.h src/*/*.cpp tools/*
QMAKE_EXTRA_TARGETS *= todo

po.commands = lupdate src -ts translations/igotu2gpx.ts; lconvert translations/igotu2gpx.ts -o translations/igotu2gpx.pot -of po; msgfmt -c translations/igotu2gpx.pot -o translations/igotu2gpx.mo
QMAKE_EXTRA_TARGETS *= po

pofiles = $$files(translations/*.po)
for(pofile, pofiles) {
    tsfile = $$replace(pofile, '_(.*)\.po$', '_\1.ts')
    qmfiles *= $$replace(tsfile, '\.ts$', '.qm')
    poconvertcommand += lconvert $$pofile -o $$tsfile -of ts;
    # Untranslated entries are not marked with fuzzy in the launchpad exports,
    # fiddle a bit with the xml output
    poconvertcommand += perl -0pi -e \'s!(<translation)(>\\s*(?:<numerusform></numerusform>\\s*)*</translation>)!\\1 type=\"unfinished\"\\2!g\' $$tsfile;
}
porelease.commands = $$poconvertcommand lrelease translations/igotu2gpx_*.ts
QMAKE_EXTRA_TARGETS *= porelease

locale.files = $$qmfiles
locale.path = $$TRANSLATIONDIR
locale.CONFIG *= no_check_exist
INSTALLS *= locale
