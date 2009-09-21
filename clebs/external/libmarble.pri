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
	isEmpty(MARBLEROOT):MARBLEROOT = /Applications/Marble.app/Contents/MacOS
	isEmpty(MARBLEINCLUDEDIR):MARBLEINCLUDEDIR = $${MARBLEROOT}/resources/data/include
	isEmpty(MARBLELIBDIR):MARBLELIBDIR = $${MARBLEROOT}/lib
	isEmpty(MARBLELIB):MARBLELIB = marblewidget

	exists($${MARBLEINCLUDEDIR}/marble):CLEBS_DEPENDENCIES *= libmarble
    }

    unix:!macx {
	isEmpty(MARBLEINCLUDEDIR) {
	    exists(/usr/lib/kde4/include/marble) {
		MARBLEINCLUDEDIR = /usr/lib/kde4/include
		MARBLELIBDIR = /usr/lib/kde4/lib
	    } else {
		MARBLEINCLUDEDIR = /usr/include
	    }
	}
	isEmpty(MARBLELIB):MARBLELIB = marblewidget

	exists($${MARBLEINCLUDEDIR}/marble):CLEBS_DEPENDENCIES *= libmarble
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
	!isEmpty(MARBLELIBDIR):LIBS *= -L$${MARBLELIBDIR}
    }
}

clebsInstall(libmarble) {
    win32 {
	CONFIG(debug, debug|release) {
	    marbleinstall.files *= $${MARBLELIBDIR_DEBUG}/$${MARBLELIB_DEBUG}.dll
	    marbleinstall.files *= $${MARBLELIBDIR_DEBUG}/lib$${MARBLELIB_DEBUG}.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Itemd.dll 
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Plugind.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_DEBUG}/plugins/*Mapd.dll
	    marbledatainstall.files *= $${MARBLELIBDIR_DEBUG}/data/mwdbii
            marbledatainstall.files *= $${MARBLELIBDIR_DEBUG}/data/stars
            marbledatainstall.files *= $${MARBLELIBDIR_DEBUG}/data/LICENSE.txt
	    marblemapsinstall.files *= $${MARBLELIBDIR_DEBUG}/data/maps/earth/openstreetmap
	    marblesvginstall.files *= $${MARBLELIBDIR_DEBUG}/data/svg/compass.svg 
            marblesvginstall.files *= $${MARBLELIBDIR_DEBUG}/data/svg/worldmap.svg
            marblebitmapsinstall.files *= $${MARBLELIBDIR_DEBUG}/data/bitmaps/cursor_*.xpm
            marblebitmapsinstall.files *= $${MARBLELIBDIR_DEBUG}/data/bitmaps/default_location.png
            marblebitmapsinstall.files *= $${MARBLELIBDIR_DEBUG}/data/bitmaps/pole_1.png
            marbleplacemarksinstall.files *= $${MARBLELIBDIR_DEBUG}/data/placemarks/b*.cache
	} else {
	    marbleinstall.files = $${MARBLELIBDIR_RELEASE}/$${MARBLELIB_RELEASE}.dll
	    marbleinstall.files = $${MARBLELIBDIR_RELEASE}/lib$${MARBLELIB_RELEASE}.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_RELEASE}/plugins/*Item.dll 
	    marblepluginsinstall.files *= $${MARBLELIBDIR_RELEASE}/plugins/*Plugin.dll
	    marblepluginsinstall.files *= $${MARBLELIBDIR_RELEASE}/plugins/*Map.dll
	    marbledatainstall.files *= $${MARBLELIBDIR_RELEASE}/data/mwdbii
            marbledatainstall.files *= $${MARBLELIBDIR_RELEASE}/data/stars
            marbledatainstall.files *= $${MARBLELIBDIR_RELEASE}/data/LICENSE.txt
	    marblemapsinstall.files *= $${MARBLELIBDIR_RELEASE}/data/maps/earth/openstreetmap
	    marblesvginstall.files *= $${MARBLELIBDIR_RELEASE}/data/svg/compass.svg 
            marblesvginstall.files *= $${MARBLELIBDIR_RELEASE}/data/svg/worldmap.svg
            marblebitmapsinstall.files *= $${MARBLELIBDIR_RELEASE}/data/bitmaps/cursor_*.xpm
            marblebitmapsinstall.files *= $${MARBLELIBDIR_RELEASE}/data/bitmaps/default_location.png
            marblebitmapsinstall.files *= $${MARBLELIBDIR_RELEASE}/data/bitmaps/pole_1.png
            marbleplacemarksinstall.files *= $${MARBLELIBDIR_RELEASE}/data/placemarks/b*.cache
	}
	marbleinstall.path = $$BINDIR
	marblepluginsinstall.path = $$BINDIR/marble
	marbledatainstall.path = $$DATADIR/marble
        marblemapsinstall.path = $$DATADIR/marble/maps/earth
        marblesvginstall.path = $$DATADIR/marble/svg
        marblebitmapsinstall.path *= $$DATADIR/marble/bitmaps
        marbleplacemarksinstall.path *= $$DATADIR/marble/placemarks

	INSTALLS *= marbleinstall marblepluginsinstall marbledatainstall marblemapsinstall marblesvginstall marblebitmapsinstall marbleplacemarksinstall
    }
}

