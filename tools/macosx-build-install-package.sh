#!/bin/sh

[ -d src ] || exit 1

ROOT=igotu2gpx
APP="$ROOT".app
DMG="$ROOT".dmg
IDENTIFIER=de.mh21.igotu2gpx
GUI=igotugui
CMDLINE=igotu2gpx
QT="Core Gui Network Svg Xml Script DBus"

SOURCE=bin/debug
CONTENTS="$APP"/Contents
MACOS="$CONTENTS"/MacOS
FRAMEWORKS="$CONTENTS"/Frameworks
RESOURCES="$CONTENTS"/Resources
PLUGINS="$CONTENTS"/PlugIns

make
rm -rf "$APP" "$DMG"
mkdir -p "$FRAMEWORKS" "$RESOURCES" "$MACOS" "$PLUGINS"
mkdir -p "$RESOURCES"/marble/maps/earth "$RESOURCES"/marble/svg "$RESOURCES"/marble/placemarks "$RESOURCES"/marble/stars "$RESOURCES"/marble/mwdbii "$RESOURCES"/marble/bitmaps "$PLUGINS"/marble "$RESOURCES"/locale
cp "$SOURCE"/"$GUI" "$MACOS"
cp "$SOURCE"/"$CMDLINE" "$MACOS"
cp "$SOURCE"/libigotu.1.dylib "$FRAMEWORKS"
cp "$SOURCE"/lib*visualizer.dylib "$PLUGINS"
cp "$SOURCE"/lib*connection.dylib "$PLUGINS"
cp data/mac/igotugui.icns "$RESOURCES"/igotu2gpx.icns
cp -R data/icons "$RESOURCES"
cp -R contrib/tango/icons "$RESOURCES"
cp translations/*.qm "$RESOURCES"/locale
cp LICENSE "$RESOURCES"

cat > "$CONTENTS"/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundleIconFile</key>
	<string>igotu2gpx.icns</string>
	<key>CFBundleName</key>
	<string>igotu2gpx</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
        <key>CFBundleGetInfoString</key>
	<string>Short description</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleExecutable</key>
	<string>$GUI</string>
	<key>CFBundleIdentifier</key>
	<string>$IDENTIFIER</string>
</dict>
</plist>
EOF

echo "AAPL????" > "$CONTENTS"/PkgInfo

cat > "$RESOURCES"/qt.conf << EOF
[Paths]
Plugins = PlugIns
EOF

for i in $QT; do
    cp -R /Library/FrameWorks/Qt$i.framework "$FRAMEWORKS"
done
rm "$FRAMEWORKS"/*/Headers
rm "$FRAMEWORKS"/*/*.prl
rm -r "$FRAMEWORKS"/*/Versions/Current/Headers

cp /Applications/Marble.app/Contents/MacOS/lib/libmarblewidget.0.7.1.dylib "$FRAMEWORKS"
cp /Applications/Marble.app/Contents/Resources/plugins/*.so "$PLUGINS"/marble
cp -r /Applications/Marble.app/Contents/Resources/data/maps/earth/openstreetmap "$RESOURCES"/marble/maps/earth
cp /Applications/Marble.app/Contents/Resources/data/mwdbii/* "$RESOURCES"/marble/mwdbii
cp /Applications/Marble.app/Contents/Resources/data/svg/compass.svg "$RESOURCES"/marble/svg
cp /Applications/Marble.app/Contents/Resources/data/svg/worldmap.svg "$RESOURCES"/marble/svg
cp /Applications/Marble.app/Contents/Resources/data/stars/* "$RESOURCES"/marble/stars
cp /Applications/Marble.app/Contents/Resources/data/bitmaps/cursor_*.xpm "$RESOURCES"/marble/bitmaps
cp /Applications/Marble.app/Contents/Resources/data/bitmaps/default_location.png "$RESOURCES"/marble/bitmaps
cp /Applications/Marble.app/Contents/Resources/data/bitmaps/pole_1.png "$RESOURCES"/marble/bitmaps
cp /Applications/Marble.app/Contents/Resources/data/placemarks/b*.cache "$RESOURCES"/marble/placemarks
cp /Applications/Marble.app/Contents/Resources/data/LICENSE.txt "$RESOURCES"/marble

cp /usr/local/lib/libboost_program_options-xgcc40-mt-1_39.dylib "$FRAMEWORKS"
cp /usr/local/lib/libusb-0.1.4.dylib "$FRAMEWORKS"

if [ "$1" != "debug" ]; then
    for i in $QT; do
	strip -x "$FRAMEWORKS"/Qt$i.framework/Versions/Current/Qt$i
    done
    strip -x "$FRAMEWORKS"/libigotu.1.dylib
    strip -x "$FRAMEWORKS"/libboost_program_options-xgcc40-mt-1_39.dylib
    strip -x "$FRAMEWORKS"/libusb-0.1.4.dylib
    strip -x "$MACOS"/"$CMDLINE"
    strip -x "$MACOS"/"$GUI"
fi

for i in "$FRAMEWORKS"/Qt*.framework/Versions/Current/Qt* "$PLUGINS"/*.dylib "$PLUGINS"/marble/*.so "$FRAMEWORKS"/*.dylib "$MACOS"/*; do
    for j in $QT; do
	install_name_tool -change Qt$j.framework/Versions/4/Qt$j @executable_path/../FrameWorks/Qt$j.framework/Versions/4/Qt$j "$i"
    done
    install_name_tool -change libigotu.1.dylib @executable_path/../FrameWorks/libigotu.1.dylib "$i"
    install_name_tool -change libboost_program_options-xgcc40-mt-1_39.dylib @executable_path/../FrameWorks/libboost_program_options-xgcc40-mt-1_39.dylib "$i"
    install_name_tool -change /usr/local/lib/libusb-0.1.4.dylib @executable_path/../FrameWorks/libusb-0.1.4.dylib "$i"
    install_name_tool -change @executable_path/lib/libmarblewidget.7.dylib @executable_path/../FrameWorks/libmarblewidget.0.7.1.dylib "$i"
done
