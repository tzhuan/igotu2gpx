#!/bin/sh

# Source this file in the home directory of rba
[ -d src ] || exit 1

pushd .. > /dev/null

if test -z "$OLDPATH"; then
    export OLDPATH=$PATH
else
    export PATH=$OLDPATH
fi

BUILD_BASE=`pwd`
WINBASE=`command.com /c cd 2>/dev/null | sed 'y/\\\\/\\//; s/\r//'`

export PATH="$PATH:$BUILD_BASE/qt-4.5.0/bin:$BUILD_BASE/qt-4.5.0/qt/bin:$BUILD_BASE/qt-4.5.0/mingw/bin"

export QMAKESPEC="win32-g++"

popd > /dev/null

echo "To run qmake, use:"
echo "  qmake"
echo "To run make, use:"
echo "  mingw32-make SHELL=cmd.exe"
