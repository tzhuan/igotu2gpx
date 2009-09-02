#!/bin/bash
cp ../trunk/localconfig.pri .
qmake -spec macx-g++
make
make porelease
tools/macosx-build-install-package.sh
tools/macosx-build-dmg.sh
mv igotu2gpx.dmg $(basename $(pwd)).dmg
