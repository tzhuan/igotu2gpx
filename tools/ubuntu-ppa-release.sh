#!/bin/bash -e

[ -d src ] || exit 1

LASTVERSION=`sed -n 's/igotu2gpx (\([^-~]*\).*/\1/p;T;q' debian/changelog`
VERSION=`echo $LASTVERSION | perl -ne '($ma,$mi,$pa) = /^(\d*)\.(\d*)(?:\.(\d*))?/; $pa ||= 0; $pa += 1; print "$ma.$mi.$pa"'`

[ -z "`bzr status`" ] || {
    echo "Uncommitted changes, aborting" >&2
    exit 1
}

echo "last version: $LASTVERSION"
echo "new version: $VERSION"

rm -f ../build-area/igotu2gpx_$VERSION.orig.tar.gz
bzr revert

sed -i 's/#define \{1,\}\(IGOTU_VERSION_STR \).*/\1'"\"$VERSION\"/" src/igotu/global.h
bzr commit -m "New release."
bzr tag igotu2gpx-$VERSION

MSGFILE=$(tempfile) || exit
trap "rm -f -- '$MSGFILE'" EXIT
echo "  * New release:" >> "$MSGFILE"
COLUMNS=1000 bzr log --line -r tag:igotu2gpx-$LASTVERSION..-1 | head -n -1 | sed 's/^.*[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} /    - /' >> "$MSGFILE"

vi "$MSGFILE"

TARBALL=-sa
for dist in intrepid jaunty karmic; do
    dch -v $VERSION-1~${dist}1 -D $dist "YYYTEMPYYY"
    (
	cat debian/changelog | sed -n '0,/YYYTEMPYYY/p'
	echo "  * Released $dist deb package."
	cat "$MSGFILE"
	cat debian/changelog | sed -n '/YYYTEMPYYY/,$p'
    ) | grep -v YYYTEMPYYY | sponge debian/changelog

    bzr builddeb --builder "debuild $TARBALL -S -pgnome-gpg -sgpg" || true
    bzr revert
    TARBALL=-sd
done

rm -f -- "$MSGFILE"
trap - EXIT


echo "Really publish to launchpad?"
read

dput ppa:igotu2gpx/ppa ../build-area/igotu2gpx_$VERSION-1~*_source.changes
