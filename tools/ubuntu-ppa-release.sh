#!/bin/bash -e

[ -d src ] || exit 1

LASTVERSION=0.2.2
VERSION=0.2.3

echo "global.h: `grep IGOTU_VERSION_STR src/igotu/global.h`"
echo "last version: $LASTVERSION"
echo "new version: $VERSION"

echo "- Updated src/igotu/global.h to the right version number?"
echo "- Updated tools/ubuntu-ppa-release.sh to the same version number?"
echo "- No uncommitted changes?"
read

rm ../build-area/igotu2gpx_$VERSION.orig.tar.gz
bzr revert

MSGFILE=$(tempfile) || exit
trap "rm -f -- '$MSGFILE'" EXIT
echo "  * New release:" >> "$MSGFILE"
COLUMNS=1000 bzr log --line -r tag:igotu2gpx-$LASTVERSION..-1 | head -n -1 | sed 's/^.*[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} /    - /' >> "$MSGFILE"

vi "$MSGFILE"

TARBALL=-sa
for dist in intrepid jaunty karmic; do
    dch -v $VERSION-1~${dist}1 -D $dist "XXXTEMPXXX"
    (
	cat debian/changelog | sed -n '0,/XXXTEMPXXX/p'
	echo "  * Released $dist deb package."
	cat "$MSGFILE"
	cat debian/changelog | sed -n '/XXXTEMPXXX/,$p'
    ) | grep -v XXXTEMPXXX | sponge debian/changelog

    bzr builddeb --builder "debuild $TARBALL -S -pgnome-gpg -sgpg" || true
    TARBALL=-sd
done

rm -f -- "$MSGFILE"
trap - EXIT

echo "Really publish to launchpad?"
read

dput ppa:igotu2gpx/ppa ../build-area/igotu2gpx_$VERSION-1~*_source.changes

echo "Be sure to tag this release with bzr tag igotu2gpx-$VERSION"
