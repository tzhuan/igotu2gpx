#!/bin/bash -e

[ -d src ] || exit 1

LASTVERSION=0.2.2
VERSION=0.2.90
DATE=`date "+%Y%m%d%H%M"`

echo "- Updated src/igotu/global.h to a .90 version number?"
echo "- Updated tools/ubuntu-ppa-snapshots.sh to the same version number?"
echo "- No uncommitted changes?"
read

bzr revert

MSGFILE=$(tempfile) || exit
trap "rm -f -- '$MSGFILE'" EXIT
echo "  * Changes since version $LASTVERSION:" >> "$MSGFILE"
COLUMNS=1000 bzr log --line -r tag:igotu2gpx-$LASTVERSION..-1 | head -n -1 | sed 's/^.*[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} /    - /' >> "$MSGFILE"

vi "$MSGFILE"

TARBALL=-sa
for dist in intrepid jaunty karmic; do
    dch -v $VERSION+bzr$DATE-1ubuntu1~${dist}1 -D $dist "XXXTEMPXXX"
    (
	cat debian/changelog | sed -n '0,/XXXTEMPXXX/p'
	echo "  * Released $dist bzr snapshot."
	cat "$MSGFILE"
	cat debian/changelog | sed -n '/XXXTEMPXXX/,$p'
    ) | grep -v XXXTEMPXXX | sponge debian/changelog

    bzr commit -m "Temporary commit."
    bzr builddeb --builder "debuild $TARBALL -S -pgnome-gpg -sgpg" || true
    bzr uncommit --force
    bzr revert
    TARBALL=-sd
done

rm -f -- "$MSGFILE"
trap - EXIT

echo "Really publish to launchpad?"
read

dput ppa:igotu2gpx/rc-archive ../build-area/igotu2gpx_$VERSION+bzr$DATE-1ubuntu1~*_source.changes
