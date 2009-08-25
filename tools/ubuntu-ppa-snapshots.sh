#!/bin/bash -e

[ -d src ] || exit 1

LASTVERSION=`sed -n 's/igotu2gpx (\([^-~]*\).*/\1/p;T;q' debian/changelog`
VERSION=`echo $LASTVERSION | sed 's/^\([0-9]*\.[0-9]*\).*$/\1.90/'`
DATE=`date "+%Y%m%d%H%M"`
FULLVERSION=$VERSION+bzr$DATE

[ -z "`bzr status`" ] || {
    echo "Uncommitted changes, aborting" >&2
    exit 1
}

echo "last version: $LASTVERSION"
echo "new version: $FULLVERSION (temporary)"

rm -f ../build-area/igotu2gpx_$FULLVERSION.orig.tar.gz
bzr revert

MSGFILE=$(tempfile) || exit
trap "rm -f -- '$MSGFILE'" EXIT
echo "  * Changes since version $LASTVERSION:" >> "$MSGFILE"
COLUMNS=1000 bzr log --line -r tag:igotu2gpx-$LASTVERSION..-1 | head -n -1 | sed 's/^.*[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} /    - /' >> "$MSGFILE"

vi "$MSGFILE"

TARBALL=-sa
for dist in intrepid jaunty karmic; do
    dch -v $FULLVERSION-1~${dist}1 -D $dist "YYYTEMPYYY"
    (
	cat debian/changelog | sed -n '0,/YYYTEMPYYY/p'
	echo "  * Released $dist bzr snapshot."
	cat "$MSGFILE"
	cat debian/changelog | sed -n '/YYYTEMPYYY/,$p'
    ) | grep -v YYYTEMPYYY | sponge debian/changelog

    sed -i 's/\(#define \{1,\}IGOTU_VERSION_STR \).*/\1'"\"$FULLVERSION\"/" src/igotu/global.h

    bzr builddeb --builder "debuild $TARBALL -S -pgnome-gpg -sgpg" || true
    bzr revert
    TARBALL=-sd
done

rm -f -- "$MSGFILE"
trap - EXIT

echo "Really publish to launchpad?"
read

dput ppa:igotu2gpx/rc-archive ../build-area/igotu2gpx_$FULLVERSION-1~*_source.changes
