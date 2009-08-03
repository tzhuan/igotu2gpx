#!/usr/bin/python
from launchpadlib.launchpad import Launchpad, STAGING_SERVICE_ROOT
from launchpadlib.credentials import Credentials
from datetime import datetime
import subprocess
import email

seriesName = "0.3"
milestoneName = "0.2.91"

credentials = Credentials()
credentials.load(open("/home/mh21/.config/launchpadlib/credentials.txt"))
cachedir = "/home/mh21/.cache/launchpadlib/"
launchpad = Launchpad(credentials, STAGING_SERVICE_ROOT, cachedir)

project = launchpad.projects['igotu2gpx']
series = launchpad.load('https://api.staging.launchpad.net/beta/igotu2gpx/' + seriesName)

milestone = None
for m in series.active_milestones:
    if m.name == milestoneName:
        milestone = m
        break
if milestone == None:
    print 'Creating milestone'
    milestone = series.newMilestone(name=milestoneName, summary='First release candidate')

if milestone.release_link != None:
    release = milestone.release
else:
    print 'Creating release'
    release = milestone.createProductRelease(date_released=datetime.now(), release_notes=
        "The first release candidate for igotu2gpx 0.3.0 has been released.\n\n"
        "Igotu2gpx is a Linux/Mac OS X/Windows GUI and command line program to "
        "show the configuration, dump and clear the internal flash and decode "
        "the stored tracks and waypoints of a MobileAction i-gotU USB GPS "
        "travel logger. It uses libusb, so no special kernel drivers are "
        "needed.\n\n"
        "Changes in %s:\n"
        "  - Reworked UI.\n"
        "  - Trackpoints split into multiple tracks.\n"
        "  - Single tracks can be saved.\n"
        "  - GT120 and GT200 purge support.\n"
        "  - Localization (http://translations.launchpad.net/igotu2gpx).\n\n"
        "Homepage: https://launchpad.net/igotu2gpx/\n"
        "Binary releases and source tarballs are available from "
        "https://launchpad.net/igotu2gpx/+download\n"
        "Ubuntu packages are available from "
        "https://launchpad.net/~igotu2gpx/+archive\n"
        "Report bugs at https://bugs.launchpad.net/igotu2gpx/\n\n"
        "Let me know how it goes. Comments, contributions, bugs, bug fixes and "
        "feature requests welcome!" % milestoneName)

gpgCall = ['gnome-gpg', '--armor', '--sign', '--detach-sign']

baseName = 'igotu2gpx-' + milestoneName

files = [{
    'type': 'Code Release Tarball',
    'mime': 'application/x-gzip',
    'name': baseName + '.tar.gz',
    'description': 'source distribution',
    }, {
    'type': 'Installer file',
    'mime': 'application/octet-stream',
    'name': baseName + '.dmg',
    'description': 'Mac OS X installer',
    }, {
    'type': 'Installer file',
    'mime': 'application/octet-stream',
    'name': baseName + '.zip',
    'description': 'Microsoft Windows release, zip file',
    }]

for file in files:
    content = open(file['name'], 'r').read()
    signatureName = file['name'] + '.asc'
    signatureContent = subprocess.Popen(gpgCall, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE).communicate(content)[0]
    print 'Uploading ' + file['name']
    """ This does not work, as json cannot represent binary data
    https://bugs.launchpad.net/launchpadlib/+bug/408605
    uploadedFile = release.add_file(content_type=file['mime'],
            description=file['description'],
            filename=file['name'],
            file_content=content,
            file_type=file['type'],
            signature_filename=signatureName,
            signature_content=signatureContent)
    """
