# Only for installing on Windows
clebsCheck(libopenssl) {
    isEmpty(OPENSSLROOT):OPENSSLROOT = $$BASEDIR/../api/openssl
    isEmpty(OPENSSLLIBDIR):OPENSSLLIBDIR = $${OPENSSLROOT}
							
    exists($${OPENSSLLIBDIR}):CLEBS_DEPENDENCIES *= libopenssl
}

clebsDependency(libopenssl) {
}

clebsInstall(libopenssl) {
    win32 {
        libopensslinstall.path = $$BINDIR
        libopensslinstall.files *= $${OPENSSLLIBDIR}/libeay32.dll $${OPENSSLLIBDIR}/ssleay32.dll
	INSTALLS *= libopensslinstall
    }
}
