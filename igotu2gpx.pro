SUBDIRS = \
    $$files(src/exporters/*) \
    $$files(src/connections/*) \
    $$files(src/visualizers/*) \
    $$files(src/programs/*) \
    src/tests \
    src/lib/igotu \
    data \

include(clebs.pri)

TEMPLATE = subdirs

docs.files = LICENSE HACKING
docs.path = $$DOCDIR
INSTALLS *= docs

todo.commands = @grep -Rn "'TODO\\|FIXME\\|XXX\\|\\\\todo'" src/*/*.pro src/*/*.h src/*/*.cpp tools/*
QMAKE_EXTRA_TARGETS *= todo

test.commands = @echo [tester] Entering dir "\\'src/tests\\'" && $$DESTDIR/tester -silent
QMAKE_EXTRA_TARGETS *= test

stripinstalled.commands = strip bin/debug-installed/bin/* bin/debug-installed/bin/plugins/* bin/debug-installed/lib/*
QMAKE_EXTRA_TARGETS *= stripinstalled

# Use iconv mangling as lupdate does not recognize octal/hex utf8 strings
po.commands = lupdate src -ts translations/igotu2gpx.ts; \
    iconv -f utf8 -t latin1 translations/igotu2gpx.ts | sponge translations/igotu2gpx.ts; \
    lconvert translations/igotu2gpx.ts -o translations/igotu2gpx.pot -of po; \
    msgfmt -c translations/igotu2gpx.pot -o translations/igotu2gpx.mo
QMAKE_EXTRA_TARGETS *= po

pofiles = $$files(translations/*.po)
tsfiles = $$files(translations/*.ts)
for(pofile, pofiles) {
    language = $$replace(pofile, '(.*)/(.*)\\.po$', '\\2')
    tsfile = $$replace(pofile, '(.*)/(.*)\\.po$', '\\1/igotu2gpx_\\2.ts')
    qmfiles *= $$replace(tsfile, '\\.ts$', '.qm')
    poconvertcommand += lconvert $$pofile -o $$tsfile -of ts --target-language $$language;
    # Untranslated entries are not marked with fuzzy in the launchpad exports,
    # fiddle a bit with the xml output
    poconvertcommand += perl -0pi -e \'s!(<translation)(>\\s*(?:<numerusform></numerusform>\\s*)*</translation>)!\\1 type=\"unfinished\"\\2!g\' $$tsfile;
}
for(tsfile, tsfiles) {
    qmfiles *= $$replace(tsfile, '\\.ts$', '.qm')
    win32:tsrelease += lrelease $$tsfile &&
}
win32 {
    tsrelease += cd .
} else {
    tsrelease = lrelease translations/igotu2gpx_*.ts || cd .
}


porelease.commands = $$poconvertcommand
QMAKE_EXTRA_TARGETS *= porelease

locale.files = $$qmfiles
locale.path = $$TRANSLATIONDIR
locale.extra = $$tsrelease
locale.CONFIG *= no_check_exist
INSTALLS *= locale
