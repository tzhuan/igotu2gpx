#!/bin/bash -e

[ -d src ] || exit 1

VERSION=0.2.90
DATE=`date "+%Y%m%d%H%M"`

echo "- Updated src/igotu/global.h to a .90 version number?"
echo "- Updated tools/ubuntu-ppa-snapshots.sh to the same version number?"
echo "- No uncommitted changes?"
read

bzr revert

TARBALL=-sa
for dist in intrepid jaunty karmic; do
    dch -v $VERSION+bzr$DATE-1ubuntu1~${dist}1 -D $dist "Released $dist bzr snapshot."
    bzr commit -m "Temporary commit."
    bzr builddeb --builder "debuild $TARBALL -S" || true
    bzr uncommit --force
    bzr revert
    TARBALL=-sd
done

echo "Really publish to launchpad?"
read

dput ppa:igotu2gpx/rc-archive ../build-area/igotu2gpx_$VERSION+bzr$DATE-1ubuntu1~*_source.changes
