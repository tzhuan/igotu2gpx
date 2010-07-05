# Template for a custom configuration
# You should not need this if you have all dependencies in the right
# directories or you are using Debian GNU/Linux.
#
# The only variable that is defined before is BASEDIR

# Uncomment for release version
#RELEASE = 1

# Uncomment to override make files that want precompiled headers
#CLEBS -= pch

# Uncomment to see the compiler parameters
#CLEBS *= nosilent

# Uncomment to disable certain modules although all dependencies are available
#CLEBS_DISABLED *= src/connections/libusb10connection

#unix:BOOSTINCLUDEDIR = /usr/include

#macx:BOOSTROOT = /usr/local
#macx:BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_38

#win32:BOOSTROOT = $$BASEDIR/../api/boost
#win32:BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_33_1

#win32:OPENSSLROOT = $$BASEDIR/../api/openssl
#win32:OPENSSLLIBDIR = $${OPENSSLROOT}

#win32:MARBLEROOT = $$BASEDIR/../../api/marble
#win32:MARBLEINCLUDEDIR = $${MARBLEROOT}/include
#win32:MARBLELIBDIR = $${MARBLEROOT}
#win32:MARBLELIBDIR_RELEASE = $${MARBLELIBDIR}
#win32:MARBLELIBDIR_DEBUG = $${MARBLELIBDIR}
#win32:MARBLELIB_RELEASE = marblewidget
#win32:MARBLELIB_DEBUG = marblewidgetd

