#!/bin/sh

[ -d src ] || exit 1

ROOT=igotu2gpx
APP="$ROOT".app
DMG="$ROOT".dmg
IDENTIFIER=de.mh21.igotu2gpx
GUI=igotugui
CMDLINE=igotu2gpx

SOURCE=bin/debug
CONTENTS="$APP"/Contents
MACOS="$CONTENTS"/MacOS
FRAMEWORKS="$CONTENTS"/Frameworks
RESOURCES="$CONTENTS"/Resources

make
rm -rf "$APP" "$DMG"
mkdir -p "$FRAMEWORKS" "$RESOURCES" "$MACOS"
cp "$SOURCE"/"$GUI" "$MACOS"
cp "$SOURCE"/"$CMDLINE" "$MACOS"
cp "$SOURCE"/libigotu.1.dylib "$FRAMEWORKS"
cp data/igotugui.icns "$RESOURCES"/igotu2gpx.icns
cp -r data/icons "$RESOURCES"

cat > "$CONTENTS"/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundleIconFile</key>
	<string>igotu2gpx.icns</string>
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

cp -R /Library/FrameWorks/QtCore.framework "$FRAMEWORKS"
cp -R /Library/FrameWorks/QtGui.framework "$FRAMEWORKS"

cp /usr/local/lib/libboost_program_options-xgcc40-mt-1_39.dylib "$FRAMEWORKS"
cp /usr/local/lib/libusb-0.1.4.dylib "$FRAMEWORKS"

strip -x "$FRAMEWORKS"/QtCore.framework/Versions/Current/QtCore
strip -x "$FRAMEWORKS"/QtGui.framework/Versions/Current/QtGui
strip -x "$FRAMEWORKS"/libigotu.1.dylib
strip -x "$FRAMEWORKS"/libboost_program_options-xgcc40-mt-1_39.dylib
strip -x "$FRAMEWORKS"/libusb-0.1.4.dylib
strip -x "$MACOS"/"$CMDLINE"
strip -x "$MACOS"/"$GUI"

install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../FrameWorks/QtCore.framework/Versions/4/QtCore "$FRAMEWORKS"/QtGui.framework/Versions/Current/QtGui
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../FrameWorks/QtCore.framework/Versions/4/QtCore "$FRAMEWORKS"/libigotu.1.dylib
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../FrameWorks/QtCore.framework/Versions/4/QtCore "$MACOS"/"$CMDLINE"
install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../FrameWorks/QtCore.framework/Versions/4/QtCore "$MACOS"/"$GUI"
install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../FrameWorks/QtGui.framework/Versions/4/QtGui "$MACOS"/"$GUI"
install_name_tool -change libigotu.1.dylib @executable_path/../FrameWorks/libigotu.1.dylib "$MACOS"/"$CMDLINE"
install_name_tool -change libigotu.1.dylib @executable_path/../FrameWorks/libigotu.1.dylib "$MACOS"/"$GUI"
install_name_tool -change libboost_program_options-xgcc40-mt-1_39.dylib @executable_path/../FrameWorks/libboost_program_options-xgcc40-mt-1_39.dylib "$MACOS"/"$CMDLINE"
install_name_tool -change libboost_program_options-xgcc40-mt-1_39.dylib @executable_path/../FrameWorks/libboost_program_options-xgcc40-mt-1_39.dylib "$MACOS"/"$GUI"
install_name_tool -change libusb-0.1.4.dylib @executable_path/../FrameWorks/libusb-0.1.4.dylib "$FRAMEWORKS"/libigotu.1.dylib

hdiutil create "$DMG" -srcfolder "$APP" -format UDZO -volname "$ROOT"
