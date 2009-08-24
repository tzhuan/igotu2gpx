#!/usr/bin/python

'''Igotu2gpx release script.

Usage: %s <milestone-to-release>'''

import os
import sys
import getopt
import subprocess
from datetime import datetime

from launchpadlib.launchpad import Launchpad, STAGING_SERVICE_ROOT
from launchpadlib.credentials import Credentials

class MakeRelease:
    def __init__(self):
        self.launchpad = self.login()
        self.project = self.launchpad.projects['igotu2gpx']
        self.files = [{
            'file_type': 'Code Release Tarball',
            'content_type': 'application/x-gzip',
            'name': 'igotu2gpx-%s.tar.gz',
            'description': 'source distribution',
            }, {
            'file_type': 'Installer file',
            'content_type': 'application/octet-stream',
            'name': 'igotu2gpx-%s.dmg',
            'description': 'Mac OS X installer',
            }, {
            'file_type': 'Installer file',
            'content_type': 'application/octet-stream',
            'name': 'igotu2gpx-%s.zip',
            'description': 'Microsoft Windows release, zip file',
            }]
        self.announcement = '''%s

Igotu2gpx is a Linux/Mac OS X/Windows GUI and command line program to show the
configuration, dump and clear the internal flash and decode the stored tracks
and waypoints of a' MobileAction i-gotU USB GPS travel logger. It uses libusb,
so no special kernel drivers are needed.
%s
Homepage: https://launchpad.net/igotu2gpx/
Binary releases and source tarballs are available from https://launchpad.net/igotu2gpx/+download
Ubuntu packages are available from %s
Report bugs at https://bugs.launchpad.net/igotu2gpx/

Let me know how it goes. Comments, bugs, translations, bug fixes and feature
requests welcome!'''

    def splitVersion(self, milestone):
        (major, minor, patch) = milestone.split('.')
        details = patch.split('+')
        patch = details[0]
        details = details[1:]
        return [int(major), int(minor), int(patch)] + details

    def seriesFromMilestone(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            minor = minor + 1
        return '%s.%s' % (major, minor)

    def versionDescription(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            finalVersion = '%s.%s.%s' % (major, minor + 1, 0)
            if patch == 90:
                versionDescription = 'a beta version'
            elif patch >= 91:
                rcNames = ['first', 'second', 'third', 'fourth', 'fifth',
                        'sixth']
                versionDescription = ('the %s release candidate' %
                        rcNames[patch - 91])
            return ('%s of igotu2gpx %s' % (versionDescription, finalVersion))
        else:
            return 'igot2gpx %s.%s.%s' % (major, minor, patch)

    def repository(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            return 'https://launchpad.net/~igotu2gpx/+archive/rc-archive'
        else:
            return 'https://launchpad.net/~igotu2gpx/+archive/ppa'

    def firstLine(self, milestone):
        return ('%s has been released.' %
                self.versionDescription(milestone).capitalize())

    def login(self):
        cachedir = os.path.expanduser('~/.cache/launchpadlib')
        if not os.path.exists(cachedir):
            os.makedirs(cachedir, 0700)
        credfile = os.path.expanduser('~/.config/launchpadlib/credentials.txt')
        try:
            credentials = Credentials()
            credentials.load(open(credfile))
            launchpad = Launchpad(credentials, STAGING_SERVICE_ROOT, cachedir)
        except:
            launchpad = Launchpad.get_token_and_login('igotu2gpx-release-script',
                    STAGING_SERVICE_ROOT, cachedir)
            launchpad.credentials.save(open(credfile, "w", 0600))
        return launchpad

    def addFile(self, release, filename, description, content_type, file_type):
        print 'Adding ' + filename
        with open(filename, 'r') as f: content = f.read()
        signatureName = filename + '.asc'
        signatureContent = subprocess.Popen(['gnome-gpg', '--armor', '--sign',
            '--detach-sign'], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE).communicate(content)[0]
        with open(signatureName, 'w') as f: f.write(signatureContent)

        ''' This does not work, as json cannot represent binary data
        https://bugs.launchpad.net/launchpadlib/+bug/408605
        uploadedFile = release.add_file(content_type=content_type,
                description=description,
                filename=filename,
                file_content=content,
                file_type=file_type,
                signature_filename=signatureName,
                signature_content=signatureContent)
        '''

    def addFiles(self, release):
        releaseName = release.version
        for file in self.files:
            self.addFile(release, file['name'] % releaseName,
                    file['description'], file['content_type'],
                    file['file_type'])

    def closeBugs(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone.name)
        if patch >= 90:
            print 'Not closing bugs for pre-release version'
            return
        for task in milestone.searchTasks(status='Fix Committed'):
            print 'Marking %s as fix released.' % task.title
            task.transitionToStatus(status='Fix Released')

    def makeRelease(self, milestone_name, changelog):
        series = self.launchpad.load('https://api.staging.launchpad.net/beta/igotu2gpx/' +
                self.seriesFromMilestone(milestone_name))
        milestone = None
        for m in series.active_milestones:
            if m.name == milestone_name:
                milestone = m
                break
        if milestone is None:
            print 'Creating milestone'
            milestone = series.newMilestone(
                    name=milestone_name,
                    summary=self.versionDescription(milestone_name))

        if milestone.release_link is not None:
            release = milestone.release
        else:
            print 'Creating release'
            release = milestone.createProductRelease(
                    date_released=datetime.now(),
                    release_notes=self.announcement % (self.firstLine(milestone_name),
                        '', self.repository(milestone_name)),
                    changelog=changelog)

        self.addFiles(release)
        self.closeBugs(milestone)

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], 'h', ['help'])
        except getopt.error, msg:
            raise Usage(msg)
        for o, a in opts:
            if o in ('-h', '--help'):
                print __doc__ % argv[0]
                return 0
        if len(args) < 2:
            raise Usage('need at least a milestone and a changelog as parameter')

        makeRelease = MakeRelease()
        makeRelease.makeRelease(args[0], args[1])

    except Usage, err:
        print >>sys.stderr, err.msg
        print >>sys.stderr, 'for help use --help'
        return 1

if __name__ == '__main__':
    sys.exit(main())
