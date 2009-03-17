#!/bin/sh

# Source this file in the home directory of igotu
[ -d src ] || exit 1

pushd .. > /dev/null

if test -z "$OLDPATH"; then
    export OLDPATH=$PATH
else
    export PATH=$OLDPATH
fi

BUILD_BASE=`pwd`
WINBASE=`command.com /c cd 2>/dev/null | sed 'y/\\\\/\\//; s/\r//'`

export PATH="$PATH:/cygdrive/c/Program Files/Microsoft Visual Studio 9.0/VC/BIN:/cygdrive/c/Program Files/Microsoft Visual Studio 9.0/Common7/IDE:/cygdrive/c/Program Files/Microsoft SDKs/Windows/v6.0A/bin"
export PATH="$PATH:$BUILD_BASE/qt-4.3.5/bin"

export MSVCDir="C:\Program Files\Microsoft Visual Studio 9.0\VC"
export INCLUDE="C:\Program Files\Microsoft Visual Studio 9.0\VC\INCLUDE;C:\Program Files\Microsoft Visual Studio 9.0\VC\INCLUDE;C:\Program Files\Microsoft SDKs\Windows\v6.0A\include"
export LIB="C:\Program Files\Microsoft Visual Studio 9.0\VC\lib;C:\Program Files\Microsoft Visual Studio 9.0\VC\LIB;C:\Program Files\Microsoft SDKs\Windows\v6.0A\lib"

# TODO: no official support in Qt4 yet
export QMAKESPEC="win32-msvc2005"

popd > /dev/null
