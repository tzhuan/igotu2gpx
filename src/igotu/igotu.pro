BASEDIR = ../..
CLEBS *= boost pch builddll
unix|macx:CLEBS *= libusb
CLEBS_INSTALL *= boost-po
TARGET = igotu
include($$BASEDIR/clebs.pri)

DEFINES *= IGOTU_MAKEDLL

SOURCES *= \
    commands.cpp \
    dataconnection.cpp \
    igotucommand.cpp \
    igotucontrol.cpp \
    igotupoints.cpp \
    optionutils.cpp \
    utils.cpp \
    verbose.cpp \
    xmlutils.cpp \

unix|macx:SOURCES *= libusbconnection.cpp
win32:SOURCES *= win32serialconnection.cpp

HEADERS *= \
    commands.h \
    dataconnection.h \
    exception.h \
    global.h \
    igotucommand.h \
    igotucontrol.h \
    igotupoints.h \
    optionutils.h \
    pch.h \
    threadutils.h \
    utils.h \
    verbose.h \
    xmlutils.h \

unix|macx:HEADERS *= libusbconnection.h
win32:HEADERS *= win32serialconnection.h

unix:ctags.commands  = echo !_TAG_FILE_FORMAT 2 dummy > $$BASEDIR/tags;
unix:ctags.commands += echo !_TAG_FILE_SORTED 1 dummy >> $$BASEDIR/tags;
unix:ctags.commands += sed -i \'s/ /\\t/g\' $$BASEDIR/tags;
unix:ctags.commands += cd $$BASEDIR && ctags -R --c++-kinds=+p-n --fields=+iaS --extra=+fq --exclude=.build -f - src | sed \'s/igotu:://g;s/\tnamespace:igotu//g\' | LC_ALL=C sort >> tags || cd .
win32:ctags.commands = cd $$BASEDIR && ctags -R --c++-kinds=+p-n --fields=+iaS --extra=+fq --exclude=.build src || cd .
ctags.target = CTAGS
QMAKE_EXTRA_TARGETS *= ctags
ALL_DEPS *= CTAGS

win32 {
    win32-g++|win32-x-g++*:installfiles *= mingwm10.dll
    win32-x-g++-4.2:installfiles *= libgcc_sjlj_1.dll libstdc++_sjlj_6.dll
    CONFIG(debug, debug|release) {
        installfiles *= QtCored4.dll QtGuid4.dll QtXmld4.dll QtNetworkd4.dll QtSvgd4.dll
    } else {
        installfiles *= QtCore4.dll QtGui4.dll QtXml4.dll QtNetwork4.dll QtSvg4.dll
    }
			    
    # native bin path for mingwm10.dll
    # all lib paths (will also work for cross-compiles)
    installpaths *= $$[QT_INSTALL_BINS] $${QMAKE_LIBDIR_QT} /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj
    for(installfile, installfiles) {
        for(installpath, installpaths) {
            exists($${installpath}/$${installfile}) {
                qt4.files *= $${installpath}/$${installfile}
                break()
            }
        }
    }
    qt4.path = $$BINDIR
    INSTALLS *= qt4
}
