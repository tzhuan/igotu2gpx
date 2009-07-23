clebsCheck(libmarble) {
    win32 {
	isEmpty(MARBLEROOT):MARBLEROOT = $$BASEDIR/../api/marble
	isEmpty(MARBLEINCLUDEDIR):MARBLEINCLUDEDIR = $${MARBLEROOT}/include
	isEmpty(MARBLELIBDIR):MARBLELIBDIR = $${MARBLEROOT}
	isEmpty(MARBLELIBDIR_RELEASE):MARBLELIBDIR_RELEASE = $${MARBLELIBDIR}
	isEmpty(MARBLELIBDIR_DEBUG):MARBLELIBDIR_DEBUG = $${MARBLELIBDIR}
	isEmpty(MARBLELIB_RELEASE):MARBLELIB_RELEASE = marblewidget
	isEmpty(MARBLELIB_DEBUG):MARBLELIB_DEBUG = marblewidgetd

	exists($${MARBLEINCLUDEDIR}):CLEBS_DEPENDENCIES *= libmarble
    }

    macx {
	isEmpty(MARBLEROOT):MARBLEROOT = /usr/local
	isEmpty(MARBLEINCLUDEDIR):MARBLEINCLUDEDIR = $${MARBLEROOT}/include
	isEmpty(MARBLELIBDIR):MARBLELIBDIR = $${MARBLEROOT}/lib
	isEmpty(MARBLELIB):MARBLELIB = libmarblewidget

	exists($${MARBLEINCLUDEDIR}/marble):CLEBS_DEPENDENCIES *= libmarble
    }

    unix:!macx {
	isEmpty(MARBLEINCLUDEDIR):MARBLEINCLUDEDIR = /usr/include
	isEmpty(MARBLELIB):MARBLELIB = marblewidget

	exists($${MARBLEINCLUDEDIR}/marble):CLEBS_DEPENDENCIES *= libmarble
    }
}

clebsDependency(libmarble) {
    win32 {
	INCLUDEPATH *= $${MARBLEINCLUDEDIR}
	CONFIG(debug, debug|release) {
	    LIBS *= -L$${MARBLELIBDIR_DEBUG} -l$${MARBLELIB_DEBUG}
	} else {
	    LIBS *= -L$${MARBLELIBDIR_RELEASE} -l$${MARBLELIB_RELEASE}
	}
    }

    macx {
	INCLUDEPATH *= $${MARBLEINCLUDEDIR}
	LIBS *= -L$${MARBLELIBDIR} -l$${MARBLELIB}
    }

    unix:!macx {
	INCLUDEPATH *= $${MARBLEINCLUDEDIR}
	LIBS *= -l$${MARBLELIB}
    }
}

clebsInstall(libmarble) {
    win32 {
	CONFIG(debug, debug|release) {
	    marbleinstall.files *= $${MARBLELIBDIR_DEBUG}/$${MARBLELIB_DEBUG}.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Itemd.dll 
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Plugind.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Mapd.dll
	} else {
	    marbleinstall.files = $${MARBLELIBDIR_RELEASE}/$${MARBLELIB_RELEASE}.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Item.dll 
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Plugin.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Map.dll
	}
	marbleinstall.path = $$BINDIR
	marblepluginsinstall.path = $$BINDIR/plugins
	INSTALLS *= marbleinstall marblepluginsinstall
    }
}

