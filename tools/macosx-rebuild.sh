#!/bin/bash
cp ../trunk/localconfig.pri .
qmake -spec macx-g++
make
lrelease translations/*_*.ts
tools/macosx-build-install-package.sh
tools/macosx-build-dmg.sh
mv igotu2gpx.dmg $(basename $(pwd)).dmg
