# Set BASEDIR to the project base path
# Specify CLEBS_REQUIRED/CLEBS_SUGGESTED to check for required/suggested libs
# Specify CLEBS_INSTALL to add install commands to the make file
# Specify CLEBS if you need libraries or special build settings
#
# Use clebsDirs to add subdirectories

# Some macros ==================================================================

# clebsDirs(subdirs,deps,modules): Adds directories to the project that depend
# on other directories. If the named modules are not available or the
# directories are already defined, the directories will be ignored.
defineTest(clebsDirs) {
    for(subdirs, 1) {
	entries = $$files($$subdirs)
	for(entry, entries) {
	    name = $$replace(entry, [/\\\\], _)
	    contains(SUBDIRS, $$name)|contains(CLEBS_IGNORED_SUBDIRS, $$name):next()
	    isEmpty(3)|clebsCheckModules($$3) {
		SUBDIRS *= $$name
		eval($${name}.subdir = $$entry)
		for(dep, 2):eval($${name}.depends *= $$replace(dep, [/\\\\], _))
		export($${name}.subdir)
		export($${name}.depends)
	    } else {
		CLEBS_IGNORED_SUBDIRS *= $$name
		export(CLEBS_IGNORED_SUBDIRS)
	    }
	}
    }
    export(SUBDIRS)
}

# clebsModule(module,deps): Defines a module that will be enabled if all
# dependencies are available. clebsCheckModules can be used to test whether
# modules are enabled.
defineTest(clebsModule) {
    clebsCheckDependencies($$2):!contains(CLEBS_DISABLED, $$1) {
	CLEBS_MODULES *= $$1
	included = yes
    } else {
	included = no
    }
    export(CLEBS_MODULES)
    clebsVerbose() {
	temp = "Building with $$1 ______________"
	temp ~= s/(.{31}).*/\1: $$included/
	temp ~= s/_/ /
	message($$temp)
    }
}

# clebsCheckModules(list): Returns true if all specified modules are available
defineTest(clebsCheckModules) {
    for(module, 1):!contains(CLEBS_MODULES, $$module):return(false)
    return(true)
}

# clebsCheckModulesOne(list): Returns true if at least one of the specified
# modules is available
defineTest(clebsCheckModulesOne) {
    for(module, 1):contains(CLEBS_MODULES, $$module):return(true)
    return(false)
}

# clebsCheckDependencies(deps): Returns true if all specified dependencies are
# available
defineTest(clebsCheckDependencies) {
    for(dep, 1):!contains(CLEBS_DEPENDENCIES, $$dep):return(false)
    return(true)
}

# clebsCheck(modulename): Used in clebs dependency include files to mark the
# block that determines whether a dependency is available. The block should add
# the module name to CLEBS_DEPENDENCIES if the dependency is available
defineTest(clebsCheck) {
    contains(CLEBS, $$1) | \
    contains(CLEBS_INSTALL, $$1) | \
    contains(CLEBS_REQUIRED, $$1) | \
    contains(CLEBS_SUGGESTED, $$1) {
	return(true)
    }
    return(false)
}

# clebsInstall(modulename): Used in clebs dependency include files to mark the
# block that installs the library.
defineTest(clebsInstall) {
    contains(CLEBS_INSTALL, $$1):contains(CLEBS_DEPENDENCIES, $$1):return(true)
    return(false)
}

# clebsDependency(modulename): Used in clebs dependency include files to mark
# the block that does the setup for a dependency like adding include dirs and
# library dependencies. If the library in turn requires other additional
# libraries, add them to the CLEBS variable.
defineTest(clebsDependency) {
    contains(CLEBS, $$1):contains(CLEBS_DEPENDENCIES, $$1):return(true)
    return(false)
}

# clebsCheckQtVersion(major,minor,patch): Checks whether the current Qt version
# is larger than or equal to the given version consisting of major, minor and
# patch revision.
defineTest(clebsCheckQtVersion) {
    version = $$[QT_VERSION]
    major = $$section(version, '.', 0, 0)
    minor = $$section(version, '.', 1, 1)
    patch = $$section(version, '.', 2, 2)
    greaterThan(major,$$1):return(true)
    lessThan(major,$$1):return(false)
    greaterThan(minor,$$2):return(true)
    lessThan(minor,$$2):return(false)
    greaterThan(patch,$$3):return(true)
    lessThan(patch,$$3):return(false)
    return(true)
}

defineTest(clebsPrintAndCheckDependency) {
    contains(CLEBS_DEPENDENCIES, $$1) {
	found = yes
	2 = ignore
    } else {
	found = no
    }
    temp = "Looking for $$1 ________________"
    temp ~= s/(.{31}).*/\1: $$found/
    temp ~= s/_/ /
    message($$temp)
    isEmpty(2):error("Please install the necessary (devel) packages for $$1!")
}

defineTest(clebsVerbose) {
    isEmpty(CLEBS_REQUIRED):isEmpty(CLEBS_SUGGESTED):return(false)
    return(true)
}

# General settings =============================================================

unix|win32-x-g++* {
    BASEDIR = $$system(pwd)
} else {
    BASEDIR = $$system(cd)
}

isEmpty(LOCALCONFIG):LOCALCONFIG = localconfig.pri
include($$LOCALCONFIG)

CONFIG += stl warn_on exceptions thread rtti silent
CONFIG += debug
CONFIG -= release
CONFIG -= debug_and_release

!isEmpty(RELEASE) {
    CONFIG += release
    CONFIG -= debug
}

QT -= gui

CONFIG(release, debug|release) {
    DESTDIR = $$BASEDIR/bin/release
    OBJECTS_DIR = .build/release
} else {
    DESTDIR = $$BASEDIR/bin/debug
    OBJECTS_DIR = .build/debug
}
MOC_DIR = $$OBJECTS_DIR/moc
UI_DIR = $$OBJECTS_DIR/ui
RCC_DIR = $$OBJECTS_DIR/rcc
unix {
    isEmpty(PREFIXDIR):PREFIXDIR = /usr/local
    isEmpty(CONFDIR):CONFDIR = /etc
    isEmpty(DATADIR):DATADIR = $$PREFIXDIR/share/igotu2gpx
    isEmpty(DOCDIR):DOCDIR = $$PREFIXDIR/share/doc/igotu2gpx
    isEmpty(APPDIR):APPDIR = $$PREFIXDIR/share/applications
    isEmpty(ICONDIR):ICONDIR = $$PREFIXDIR/share/icons/hicolor
    isEmpty(MANDIR):MANDIR = $$PREFIXDIR/share/man
    isEmpty(BINDIR):BINDIR = $$PREFIXDIR/bin
    isEmpty(LIBDIR):LIBDIR = $$PREFIXDIR/lib
    isEmpty(PLUGINDIR):PLUGINDIR = $$PREFIXDIR/lib/igotu2gpx
}
win32 {
    isEmpty(PREFIXDIR):PREFIXDIR = $${DESTDIR}-installed
    isEmpty(CONFDIR):CONFDIR = $$PREFIXDIR/etc
    isEmpty(DATADIR):DATADIR = $$PREFIXDIR/share
    isEmpty(DOCDIR):DOCDIR = $$PREFIXDIR/doc
    isEmpty(ICONDIR):ICONDIR = $$PREFIXDIR/icons
    isEmpty(MANDIR):MANDIR = $$PREFIXDIR/man
    isEmpty(BINDIR):BINDIR = $$PREFIXDIR/bin
    isEmpty(LIBDIR):LIBDIR = $$PREFIXDIR/bin
    isEmpty(PLUGINDIR):PLUGINDIR = $$PREFIXDIR/lib
}

# Programs may need internal libraries...
LIBS *= -L$$DESTDIR
# ...and these libraries may need more internal libraries
unix:!macx:QMAKE_LFLAGS *=-Wl,-rpath-link=$$DESTDIR
# For includes from .ui files on MinGW
INCLUDEPATH *= .
DEPENDPATH *= .

# Install to the binary directory per default
target.path = $$BINDIR
INSTALLS *= target

# mingw creates scripts if there are too many source files
win32-g++|win32-x-g++*:QMAKE_CLEAN *= object_script.*

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII

unix {
    CONFIG += link_pkgconfig

    #QMAKE_CXXFLAGS_DEBUG *= -O2
}

macx {
    CONFIG -= app_bundle
}

win32 {
    CONFIG += embed_manifest_exe

    # Creates a console window so we see debug messages
    CONFIG(debug, debug|release):CONFIG += console

    # STL iterators and fopen are unsafe, we know that
    DEFINES *= _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS
    # Use our own assert handler so that we can coredump, does not work because
    # the boost definition has the wrong linkage
    # DEFINES *= BOOST_ENABLE_ASSERT_HANDLER
}

# Linking against libraries ====================================================

libstoinclude = $$CLEBS_REQUIRED $$CLEBS_SUGGESTED $$CLEBS_INSTALL $$CLEBS
for(ever) {
    includedsomething =
    for(ever) {
	isEmpty(libstoinclude):break()
	dep = $$first(libstoinclude)
	!contains(libsincluded, $$dep) {
	    foundinclude =
	    subdirs = clebs $$files(clebs/*)
	    for(subdir, subdirs) {
		exists($${subdir}/$${dep}.pri) {
		    include($${subdir}/$${dep}.pri)
		    foundinclude = true
		    break()
		}
	    }
	    isEmpty(foundinclude):error("Build configuration requested for unknown dependency $$dep!")
	    libsincluded *= $$dep
	    includedsomething = true
	}
	libstoinclude -= $$dep
	isEmpty(libstoinclude):break()
    }
    isEmpty(includedsomething):break()
    libstoinclude = $$CLEBS
}

clebsVerbose() {
    CONFIG(release, debug|release) {
	message("------------------------------------")
        message("BUILDING IN RELEASE MODE")
    }
    message("------------------------------------")
}

clebsVerbose() {
    message("Required dependencies:")
    message("------------------------------------")
    for(dep, CLEBS_REQUIRED):clebsPrintAndCheckDependency($$dep)
    message("------------------------------------")
    message("Suggested dependencies:")
    message("------------------------------------")
    for(dep, CLEBS_SUGGESTED):clebsPrintAndCheckDependency($$dep, ignore)
    message("------------------------------------")
}

# Module dependencies ==========================================================

clebsVerbose() {
    message("Additional features:")
    message("------------------------------------")
}
include(modules.pri)
clebsVerbose() {
    message("------------------------------------")
}
