#!/bin/bash

if [ -f Makefile ]; then
    QMAKEARGS=`sed -n 's/# Command: [\\\/A-Za-z]\+ //p' Makefile`
fi

if [ "`uname -o`" == "Cygwin" ]; then
    MAKE=nmake
    MAKEOPTS=
else
    MAKE=make
    MAKEOPTS="-j 4"
fi

find . -depth \( -name .build -o -name 'Makefile*' -o -name 'object_script.*' \) -exec rm -r {} \;
rm -r bin
echo qmake $QMAKEARGS
qmake-qt4 $QMAKEARGS || qmake $QMAKEARGS
$MAKE $MAKEOPTS
