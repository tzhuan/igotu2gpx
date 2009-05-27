#!/bin/sh

[ -d src ] || exit 1

ROOT=igotu2gpx
APP="$ROOT".app
DMG="$ROOT".dmg
TEMPLATE="$DMG"-temp

rm -rf "$TEMPLATE"
rm -f "$DMG"

mkdir "$TEMPLATE"
cp -R "$APP" "$TEMPLATE"
ln -s /Applications "$TEMPLATE"/Applications

contrib/create-dmg/create-dmg \
    --window-size 460 260 \
    --background data/mac/background.png \
    --icon-size 96 \
    --volname "$ROOT" \
    --icon "Applications" 90 130 \
    --icon "igotu2gpx" 370 130 \
    "$DMG" "$TEMPLATE"

rm -rf "$TEMPLATE"
rm rw."$DMG"
