#!/usr/bin/python

'''Igotu2gpx launchpad helper library

Usage: %s command

commands:
    create <version>|snapshot  start a new release
    dist                       create tarball and Ubuntu source packages
    ppa                        upload source packages to ppa
    upload                     upload tarball and archives to launchpad
'''

from datetime import datetime
import getopt
import glob
import shutil
import os
import os.path
import re
import subprocess
import sys
import urlparse
import xdg.BaseDirectory

from launchpadlib.launchpad import Launchpad, STAGING_SERVICE_ROOT, EDGE_SERVICE_ROOT
from launchpadlib.credentials import Credentials

class LaunchpadHelper:
    def __init__(self):
        self.configDir = xdg.BaseDirectory.save_config_path('mh21.de',
                'igotu2gpx-launchpad-tools')
        self.versionFile = os.path.join(self.configDir, 'version.txt')
        self.changelogFile = os.path.join(self.configDir, 'changelog.txt')
        self.releaseNotesFile = os.path.join(self.configDir, 'releasenotes.txt')
        self.releaseExportFile = os.path.join(self.configDir, 'release-data.txt')
        self.launchpad = None
        self.targets = ['hardy', 'intrepid', 'jaunty', 'karmic', 'lucid'];
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

    def joinVersion(self, details):
        return '%s.%s.%s' % (details[0], details[1], '+'.join([str(x) for x in
            details[2:]]))

    def seriesFromMilestone(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            minor = minor + 1
        return '%s.%s' % (major, minor)

    ''' Normalize the version signature (no patch etc.)'''
    def versionName(self, milestone):
        return self.joinVersion(self.splitVersion(milestone))

    def versionDescription(self, milestone):
        details = self.splitVersion(milestone)
        (major, minor, patch) = details[0:3]
        if patch >= 90:
            finalVersion = '%s.%s.%s' % (major, minor + 1, 0)
            if patch == 90:
                versionDescription = 'a beta version'
            elif patch >= 91:
                rcNames = ['first', 'second', 'third', 'fourth', 'fifth',
                        'sixth']
                versionDescription = ('the %s release candidate' %
                        rcNames[patch - 91])
            versionDescription = '%s of igotu2gpx %s' % (versionDescription, finalVersion)
            if patch == 90:
                details = self.splitVersion(milestone)
                versionDescription = '%s (%s)' % (versionDescription,
                        milestone)
            return versionDescription
        return 'igot2gpx %s.%s.%s' % (major, minor, patch)

    def repository(self, milestone):
        return 'https://launchpad.net/~igotu2gpx/+archive/' + self.ppa(milestone)

    def ppa(self, milestone):
        (major, minor, patch) = self.splitVersion(milestone)
        if patch >= 90:
            return 'rc-archive'
        else:
            return 'ppa'

    def releasedString(self, milestone):
        return ('%s has been released.' %
                self.versionDescription(milestone).capitalize())

    def url(self, milestone):
        return('https://launchpad.net/igotu2gpx/%s/%s' %
                (self.seriesFromMilestone(milestone), milestone))

    def login(self):
        if self.launchpad is not None:
            return
        cachedir = os.path.expanduser('~/.cache/launchpadlib')
        creddir = os.path.expanduser('~/.config/launchpadlib')
        credfile = creddir + '/credentials.txt'
        root = EDGE_SERVICE_ROOT
        if not os.path.exists(cachedir):
            os.makedirs(cachedir, 0700)
        try:
            credentials = Credentials()
            credentials.load(open(credfile))
            self.launchpad = Launchpad(credentials, root, cachedir)
        except:
            self.launchpad = Launchpad.get_token_and_login('igotu2gpx-launchpad-tools',
                    root, cachedir)
            if not os.path.exists(creddir):
                os.makedirs(creddir, 0700)
            self.launchpad.credentials.save(open(credfile, "w", 0600))
        self.project = self.launchpad.projects['igotu2gpx']

    def exportReleases(self):
        self.login()
        result = ''
        for release in self.project.releases:
            try:
                releaseVersion = release.version
                (major, minor, patch) = self.splitVersion(releaseVersion)
            except:
                # ignore 0.2.2rc1
                continue
            # TODO make sure this is a ascii file, use ini escape sequences
            result = result + (
                    '[' + releaseVersion + ']\n' +
                    'version=' + self.versionName(releaseVersion) + '\n' +
                    'name=' + self.versionDescription(releaseVersion).capitalize() + '\n' +
                    'series=' + self.seriesFromMilestone(releaseVersion) + '\n' +
                    'url=' + self.url(releaseVersion) + '\n' +
                    'repository=' + self.repository(releaseVersion) + '\n')
            if patch >= 90:
                result = result + 'status=devel\n'
            else:
                result = result + 'status=stable\n'
        return result

    def addFile(self, release, filename, description, content_type, file_type):
        self.login()
        print 'Adding ' + filename
        with open(filename, 'r') as f: content = f.read()
        signatureName = filename + '.asc'
        signatureContent = subprocess.Popen(['gnome-gpg', '--armor', '--sign',
            '--detach-sign'], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE).communicate(content)[0]
        uploadedFile = release.add_file(content_type=content_type,
                description=description,
                filename=filename,
                file_content=content,
                file_type=file_type,
                signature_filename=signatureName,
                signature_content=signatureContent)

    def addFiles(self, release):
        self.login()
        releaseName = release.version
        existingFiles = []
        for file in release.files:
            existingFiles.append(os.path.basename(urlparse.urlparse(file.self_link).path))
        for file in self.files:
            if file['name'] % releaseName not in existingFiles:
                self.addFile(release, file['name'] % releaseName,
                        file['description'], file['content_type'],
                        file['file_type'])

    def closeBugs(self, milestone):
        self.login()
        (major, minor, patch) = self.splitVersion(milestone.name)
        if patch >= 90:
            print 'Not closing bugs for pre-release version'
            return
        for task in milestone.searchTasks(status='Fix Committed'):
            print 'Marking %s as fix released.' % task.title
            task.transitionToStatus(status='Fix Released')

    def createCommand(self, version):
        lastVersion = subprocess.Popen(['sed', '-n',
            's/igotu2gpx (\\([^-~]*\\).*/\\1/p;T;q', 'debian/changelog'],
            stdout=subprocess.PIPE).communicate()[0].strip()
        details = self.splitVersion(lastVersion)
        if version == 'snapshot':
            details = details[0:2] + [90,
                    'bzr' + datetime.now().strftime('%Y%m%d%H%M')]
        else:
            details = self.splitVersion(version)
        newVersion = self.joinVersion(details)
        print 'New Version: ', newVersion

        with open(self.versionFile, 'w') as f: f.write(newVersion)

        changelogTemplate = '  * Changes since version %s:\n%s' % (lastVersion,
                subprocess.Popen("COLUMNS=1000 "
                "bzr log --line -r tag:igotu2gpx-%s..-1 | head -n -1 | "
                "sed 's/^.*[0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} /    - /'" %
                lastVersion, shell=True,
                stdout=subprocess.PIPE).communicate()[0])

        with open(self.changelogFile, 'w') as f: f.write(changelogTemplate)

        with open(self.releaseNotesFile, 'w') as f: f.write('''%s

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
requests welcome!''')

        print('The following files have been created:')
        print('  %s - version' % self.versionFile)
        print('  %s - changelog' % self.changelogFile)
        print('  %s - release notes' % self.releaseNotesFile)
        print('Edit the files as necessary and continue with dist.')

    def distCommand(self):
        with open(self.versionFile, 'r') as f:
            version = f.read()
        with open(self.changelogFile, 'r') as f:
            changelog = f.read()

        tarballs = [
            '../build-area/igotu2gpx_%s.orig.tar.gz' % version,
            '../build-area/igotu2gpx-%s.tar.gz' % version,
            '../igotu2gpx_%s.orig.tar.gz' % version,
            'igotu2gpx_%s.orig.tar.gz' % version,
            ]
        for tarball in tarballs:
            try:
                os.remove(tarball)
            except:
                pass

        if subprocess.call(['bzr', 'revert']) != 0:
            sys.exit('Could not revert to previous version')

        # New version, changelog, release commit
        with open('src/igotu/global.h', 'r') as f:
            globalh = f.read()
        globalh = re.sub(r'(#define\s+IGOTU_VERSION_STR\s).*', r'\1"%s"' %
                version, globalh)
        with open('src/igotu/global.h', 'w') as f:
            f.write(globalh)

        if subprocess.call(['dch', '-v', '%s-0' % version, '-D',
            'UNRELEASED', 'YYY#PLACEHOLDER#YYY']) != 0:
            sys.exit('Could not modify changelog')
        with open('debian/changelog', 'r') as f:
            debianchangelog = f.read()
        debianchangelog = re.sub('YYY#PLACEHOLDER#YYY', 'Released %s.\n%s' %
            (self.versionDescription(version), changelog), debianchangelog)
        with open('debian/changelog', 'w') as f:
            f.write(debianchangelog)

        if subprocess.call(['bzr', 'commit', '-m', 'Released %s.' %
            self.versionDescription(version)]) != 0:
            sys.exit('Could not commit release')
        if subprocess.call(['bzr', 'tag', 'igotu2gpx-%s' % version]) != 0:
            sys.exit('Could not tag release')

        # Translations
        cwd = os.getcwd()
        pwd = os.path.join(cwd, '..')
        branch = os.path.basename(cwd)
        translationsbranch = branch + '-translations'
        translations = os.path.join(pwd, translationsbranch)
        if not os.path.isdir(translations):
            if subprocess.call(['bzr', 'branch', 'lp:~igotu2gpx/igotu2gpx/' +
                translationsbranch], cwd=pwd) != 0:
                sys.exit('Could not branch translations')
        else:
            if subprocess.call(['bzr', 'pull'], cwd=translations) != 0:
                sys.exit('Could not update translations')

        for file in glob.glob(os.path.join(translations, 'translations', '*.po')):
            shutil.copy(file, 'translations')
        if subprocess.call(['make', 'qmake']) != 0:
            sys.exit('Could not update makefiles')

        if subprocess.call(['make', 'porelease']) != 0:
            sys.exit('Could not create qt translation files')

        for file in glob.glob(os.path.join('translations', '*_*.ts')):
            if subprocess.call(['bzr', 'add', file]) != 0:
                sys.exit('Could not add translations to bazaar')

        tarball = '-sa'
        for dist in self.targets:
            if subprocess.call(['dch', '-v', '%s-1~%s1' % (version, dist), '-D',
                dist, 'Released %s for %s.' % (self.versionDescription(version),
                    dist)]) != 0:
                sys.exit('Could not modify changelog')

            if subprocess.call(['bzr', 'builddeb', '--builder',
                'debuild %s -S -pgnome-gpg -sgpg' % tarball]) != 0:
                sys.exit('Could not build debian source packages')

            if subprocess.call(['bzr', 'revert', 'debian/changelog']) != 0:
                sys.exit('Could not revert to previous version')

            tarball = '-sd'

        os.rename('../igotu2gpx_%s.orig.tar.gz' % version,
                'igotu2gpx-%s.tar.gz' % version)

        for extension in ['*.po', '*_*.ts']:
            for file in glob.glob(os.path.join('translations', extension)):
                os.remove(file)

        if subprocess.call(['bzr', 'revert']) != 0:
            sys.exit('Could not revert to previous version')

        print('To build ubuntu packages, use ppa.')
        print('The tarball can be found at %s' % ('igotu2gpx-%s.tar.gz' % version))
        print('Compile the tarball on Windows and Mac OS X and place the resulting')
        print('zip and dmg file next to it. Then continue with upload.')

    def ppaCommand(self):
        with open(self.versionFile, 'r') as f:
            version = f.read()

        for dist in self.targets:
            if subprocess.call(['dput', 'ppa:mh21/ppa', os.path.join('..',
                'build-area', 'igotu2gpx_%s-1~%s1_source.changes' % (version,
                    dist))]) != 0:
                sys.exit('Could not upload to ppa')

    def uploadCommand(self):
        with open(self.versionFile, 'r') as f:
            version = f.read()
        with open(self.changelogFile, 'r') as f:
            changelog = f.read()
        with open(self.releaseNotesFile, 'r') as f:
            releaseNotes = f.read()
        self.login()

        series = None
        seriesName = self.seriesFromMilestone(version)
        for s in self.project.series:
            if s.name == seriesName:
                series = s
                break
        if series is None:
            sys.exit('Unable to load series %s' % seriesName)

        milestone = None
        for m in series.active_milestones:
            if m.name == version:
                milestone = m
                break
        if milestone is None:
            print 'Creating milestone'
            milestone = series.newMilestone(
                    name=version,
                    summary=self.versionDescription(version))

        if milestone.release_link is not None:
            release = milestone.release
        else:
            print 'Creating release'
            release = milestone.createProductRelease(
                    date_released=datetime.now(),
                    release_notes=releaseNotes %
                    (self.releasedString(version),
                        '', self.repository(version)),
                    changelog=changelog)
            milestone.is_active = False
            milestone.lp_save()

        self.addFiles(release)

        ppa = self.launchpad.people['mh21'].ppas[0]
        team = self.launchpad.people['igotu2gpx']
        for dist in self.targets:
            binaries = ppa.getPublishedBinaries(
                    binary_name='igotu2gpx',
                    version='%s-1~%s1' % (version, dist),
                    status='Published',
                    exact_match=True,
                    )
            binaryCount = 0
            for binary in binaries:
                binaryCount = binaryCount + 1
            if binaryCount < 3:
                sys.exit('Not enough published binaries found')
            team.getPPAByName(name=self.ppa(version)).syncSource(
                    from_archive=ppa,
                    include_binaries=True,
                    source_name='igotu2gpx',
                    version='%s-1~%s1' % (version, dist),
                    to_pocket='Release')

        self.closeBugs(milestone)

        with open(self.releaseExportFile, 'w') as f: f.write(self.exportReleases())
        if subprocess.call(['scp', self.releaseExportFile,
            'mh21.de:www/igotu2gpx']) != 0:
            sys.exit('Could not upload release information')
        print('Please create a release announcement on launchpad, post to'
                ' igotu2gpx-users and a-trip.com/forum')

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.gnu_getopt(argv[1:], 'h', ['help'])
        except getopt.error, msg:
            raise Usage(msg)
        for o, a in opts:
            if o in ('-h', '--help'):
                print __doc__ % argv[0]
                return 0

        if len(args) < 1:
            raise Usage('Please specify a command')

        launchpadHelper = LaunchpadHelper()

        if args[0] == 'create':
            if len(args) < 2:
                raise Usage('Need a version or "snapshot"')
            launchpadHelper.createCommand(args[1])
        elif args[0] == 'dist':
            launchpadHelper.distCommand()
        elif args[0] == 'ppa':
            launchpadHelper.ppaCommand()
        elif args[0] == 'upload':
            launchpadHelper.uploadCommand()
        else:
            raise Usage('Unknown command %s' % args[0])

    except Usage, err:
        print >>sys.stderr, err.msg
        print >>sys.stderr, 'for help use --help'
        return 1

if __name__ == '__main__':
    sys.exit(main())
