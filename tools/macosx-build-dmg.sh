#!/bin/sh

[ -d src ] || exit 1

ROOT=igotu2gpx
APP="$ROOT".app
DMG="$ROOT".dmg

hdiutil create "$DMG" -srcfolder "$APP" -format UDZO -volname "$ROOT"
