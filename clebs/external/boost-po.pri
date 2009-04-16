clebsCheck(boost-po) {
    win32 {
	isEmpty(BOOSTROOT):BOOSTROOT = $$BASEDIR/../api/boost
	isEmpty(BOOSTINCLUDEDIR):BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_33_1
	isEmpty(BOOSTLIBDIR):BOOSTLIBDIR = $${BOOSTROOT}/lib
	isEmpty(BOOSTLIBDIR_RELEASE):BOOSTLIBDIR_RELEASE = $${BOOSTLIBDIR}
	isEmpty(BOOSTLIBDIR_DEBUG):BOOSTLIBDIR_DEBUG = $${BOOSTLIBDIR}
	isEmpty(BOOSTPOLIB_RELEASE):BOOSTPOLIB_RELEASE = boost_program_options-vc71-mt-1_33_1
	isEmpty(BOOSTPOLIB_DEBUG):BOOSTPOLIB_DEBUG = boost_program_options-vc71-mt-gd-1_33_1

	exists($${BOOSTINCLUDEDIR}/boost/program_options):CLEBS_DEPENDENCIES *= boost-po
    }

    macx {
	isEmpty(BOOSTROOT):BOOSTROOT = /usr/local
	isEmpty(BOOSTINCLUDEDIR):BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_38
	isEmpty(BOOSTLIBDIR):BOOSTLIBDIR = $${BOOSTROOT}/lib
	isEmpty(BOOSTPOLIB):BOOSTPOLIB = boost_program_options-xgcc40-mt-1_38

	exists($${BOOSTINCLUDEDIR}/boost/program_options):CLEBS_DEPENDENCIES *= boost-po
    }

    unix:!macx {
	isEmpty(BOOSTINCLUDEDIR):BOOSTINCLUDEDIR = /usr/include

	exists($${BOOSTINCLUDEDIR}/boost/program_options):CLEBS_DEPENDENCIES *= boost-po
    }
}

clebsDependency(boost-po) {
    win32 {
	INCLUDEPATH *= $${BOOSTINCLUDEDIR}
	DEFINES *= BOOST_PROGRAM_OPTIONS_DYN_LINK
	CONFIG(debug, debug|release) {
	    LIBS *= -L$${BOOSTLIBDIR_DEBUG}
	} else {
	    LIBS *= -L$${BOOSTLIBDIR_RELEASE}
	}
    }

    macx {
	INCLUDEPATH *= $${BOOSTINCLUDEDIR}
	LIBS *= -L$${BOOSTLIBDIR} -l$${BOOSTPOLIB}
    }

    unix:!macx {
	INCLUDEPATH *= $${BOOSTINCLUDEDIR}
	LIBS *= -lboost_program_options-mt
    }
}

clebsInstall(boost-po) {
    win32 {
	CONFIG(debug, debug|release) {
	    boost-poinstall.files = $${BOOSTLIBDIR_DEBUG}/$${BOOSTPOLIB_DEBUG}.dll
	} else {
	    boost-poinstall.files = $${BOOSTLIBDIR_RELEASE}/$${BOOSTPOLIB_RELEASE}.dll
	}
	boost-poinstall.path = $$BINDIR
	INSTALLS *= boost-poinstall
    }
}

