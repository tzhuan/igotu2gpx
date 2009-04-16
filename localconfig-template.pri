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
#CLEBS_DISABLED *= examples unittests

#unix:BOOSTINCLUDEDIR = /usr/include

#macx:BOOSTROOT = /usr/local
#macx:BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_38
#macx:BOOSTLIBDIR = $${BOOSTROOT}/lib
#macx:BOOSTPOLIB = boost_program_options-xgcc40-mt
#macx:LIBUSBINCLUDEDIR = /usr/local/include

#win32:BOOSTROOT = $$BASEDIR/../api/boost
#win32:BOOSTINCLUDEDIR = $${BOOSTROOT}/include/boost-1_33_1
#win32:BOOSTLIBDIR = $${BOOSTROOT}/lib
#win32:BOOSTLIBDIR_RELEASE = $${BOOSTLIBDIR}
#win32:BOOSTLIBDIR_DEBUG = $${BOOSTLIBDIR}
#win32:BOOSTPOLIB_RELEASE = boost_program_options-vc71-mt-1_33_1
#win32:BOOSTPOLIB_DEBUG = boost_program_options-vc71-mt-gd-1_33_1
