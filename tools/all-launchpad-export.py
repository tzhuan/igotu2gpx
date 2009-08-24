#!/usr/bin/python

'''Igotu2gpx releases export script.

Usage: %s'''

import os
import sys
import getopt
import subprocess
from datetime import datetime

from launchpadlib.launchpad import Launchpad, STAGING_SERVICE_ROOT, EDGE_SERVICE_ROOT
from launchpadlib.credentials import Credentials

class ReleaseExport:
    def __init__(self):
        self.launchpad = self.login()
        self.project = self.launchpad.projects['igotu2gpx']

    def splitVersion(self, milestone):
        try:
            (major, minor, patch) = milestone.split('.')
        except ValueError:
            (major, minor) = milestone.split('.')
            patch = '0'
        details = patch.split('+')
        patch = details[0]
        details = details[1:]
        return [int(major), int(minor), int(patch)] + details

    def seriesFromMilestone(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            minor = minor + 1
        return '%s.%s' % (major, minor)

    def versionName(self, milestone):
        details = self.splitVersion(milestone)
        return '%s.%s.%s' % (details[0], details[1], '+'.join([str(x) for x in
            details[2:]]))

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

    def login(self):
        cachedir = os.path.expanduser('~/.cache/launchpadlib')
        creddir = os.path.expanduser('~/.config/launchpadlib')
        credfile = creddir + '/credentials.txt'
        root = EDGE_SERVICE_ROOT
        if not os.path.exists(cachedir):
            os.makedirs(cachedir, 0700)
        try:
            credentials = Credentials()
            credentials.load(open(credfile))
            launchpad = Launchpad(credentials, root, cachedir)
        except:
            launchpad = Launchpad.get_token_and_login('igotu2gpx-releases-export-script',
                    root, cachedir)
            if not os.path.exists(creddir):
                os.makedirs(creddir, 0700)
            launchpad.credentials.save(open(credfile, "w", 0600))
        return launchpad

    def export(self):
        for release in self.project.releases:
            try:
                releaseVersion = release.version
                (major, minor, patch) = self.splitVersion(releaseVersion)
            except:
                # ignore 0.2.2rc1
                continue
            print '[' + releaseVersion + ']'
            print 'version=' + self.versionName(releaseVersion)
            print 'name=' + self.versionDescription(releaseVersion)
            print 'series=' + self.seriesFromMilestone(releaseVersion)
            print 'repository=' + self.repository(releaseVersion)
            if patch >= 90:
                print 'status=devel'
            else:
                print 'status=stable'

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

        releaseExport = ReleaseExport()
        releaseExport.export()

    except Usage, err:
        print >>sys.stderr, err.msg
        print >>sys.stderr, 'for help use --help'
        return 1

if __name__ == '__main__':
    sys.exit(main())
