#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# main.py --- Main module for terrasync.py
#
# Copyright (C) 2016  Torsten Dreyer
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import argparse
import enum
import hashlib
import os
import pathlib
import re
import shutil
import sys
import time
import urllib

from urllib.parse import urlparse
from http.client import HTTPConnection, _CS_IDLE, HTTPException
from os import listdir
from os.path import isfile, isdir, join

from .exceptions import UserError, NetworkError


PROGNAME = os.path.basename(sys.argv[0])

class ExitStatus(enum.Enum):
    SUCCESS = 0
    # The program exit status is 1 when an exception isn't caught.
    ERROR = 1
    CHECK_MODE_FOUND_MISMATCH = 2


# *****************************************************************************
# *                             Utility functions                             *
# *****************************************************************************

# If a path matches this regexp, we really don't want to delete it recursively
# (“cre” stands for “compiled regexp”).
_removeDirectoryTree_dangerous_cre = re.compile(
    r"""^(/ (home (/ [^/]*)? )? /* |    # for Unix-like systems
          [a-zA-Z]: [\/]*               # for Windows
    )$""", re.VERBOSE)

def removeDirectoryTree(base, whatToRemove):
    """Recursively remove directory 'whatToRemove', with safety checks.

    This function ensures that 'whatToRemove' does not resolve to a
    directory such as /, /home, /home/foobar, C:\, d:\, etc. It is also
    an error if 'whatToRemove' does not literally start with the value
    of 'base' (IOW, this function refuses to erase anything that is not
    under 'base').

    'whatToRemove' is *not* interpreted relatively to 'base' (this would
    be doable, just a different API).

    """
    assert os.path.isdir(base), "Not a directory: {!r}".format(base)
    assert (base and
            whatToRemove.startswith(base) and
            whatToRemove[len(base):].startswith(os.sep)), \
            "Unexpected base path for removeDirectoryTree(): {!r}".format(base)
    absPath = os.path.abspath(whatToRemove)

    if _removeDirectoryTree_dangerous_cre.match(absPath):
        raise UserError("in order to protect your data, refusing to "
                        "recursively delete '{}'".format(absPath))
    else:
        shutil.rmtree(absPath)


def computeHash(fileLike):
    hash = hashlib.sha1()

    for chunk in iter(lambda: fileLike.read(4096), b""):
        hash.update(chunk)

    return hash.hexdigest()


def hashForFile(fname):
    with open(fname, "rb") as f:
        return computeHash(f)


def normalizeVirtualPath(path):
    """Normalized string representation of a virtual path.

    Virtual paths are paths inside the TerraSync repository (be it local
    or remote) using '/' as their separator. The virtual path '/' always
    corresponds to the repository root, regardless of where it is stored
    (hard drive, etc.).

    If the input path (string) doesn't start with a slash ('/'), it is
    considered relative to the root of the TerraSync repository.

    Return a string that always starts with a slash, never contains
    consecutive slashes and only ends with a slash if it is the root
    virtual path ('/').

    """
    if not path.startswith('/'):
        # / is the “virtual root” of the TerraSync repository
        path = '/' + path
    elif path.startswith('//') and not path.startswith('///'):
        # Nasty special case. As allowed (but not mandated!) by POSIX[1],
        # in pathlib.PurePosixPath('//some/path'), no collapsing happens[2].
        # This is only the case for exactly *two* *leading* slashes.
        # [1] http://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap04.html#tag_04_11
        # [2] https://www.python.org/dev/peps/pep-0428/#construction
        path = path[1:]

    return pathlib.PurePosixPath(path).as_posix()


# *****************************************************************************
# *                          Network-related classes                          *
# *****************************************************************************

class HTTPGetCallback:
    def __init__(self, src, callback):
        self.callback = callback
        self.src = src

class HTTPGetter:
    def __init__(self, baseUrl, maxPending=10):
        self.baseUrl = baseUrl
        self.parsedBaseUrl = urlparse(baseUrl)
        self.maxPending = maxPending
        self.requests = []
        self.pendingRequests = []
        self.httpConnection = HTTPConnection(self.parsedBaseUrl.netloc)
        self.httpRequestHeaders = headers = {'Host':self.parsedBaseUrl.netloc,'Content-Length':0,'Connection':'Keep-Alive','User-Agent':'FlightGear terrasync.py'}

    def assembleUrl(self, httpGetCallback):
        return self.parsedBaseUrl.path + httpGetCallback.src

    def doGet(self, httpGetCallback):
        conn = self.httpConnection
        url = self.assembleUrl(httpGetCallback)
        self.httpConnection.request("GET", url, None, self.httpRequestHeaders)
        httpResponse = self.httpConnection.getresponse()

        # 'httpResponse' is an http.client.HTTPResponse instance
        return httpGetCallback.callback(url, httpResponse)

    def get(self, httpGetCallback):
        nbRetries = nbRetriesLeft = 5

        while True:
            try:
                return self.doGet(httpGetCallback)
            except HTTPException as exc:
                if nbRetriesLeft == 0:
                    raise NetworkError(
                        "after {nbRetries} retries for URL {url}: {errMsg}"
                        .format(nbRetries=nbRetries,
                                url=self.assembleUrl(httpGetCallback),
                                errMsg=exc)) from exc

            # Try to reconnect
            self.httpConnection.close()
            time.sleep(1)
            self.httpConnection.connect()
            nbRetriesLeft -= 1


#################################################################################################################################
class DirIndex:

    def __init__(self, dirIndexFile):
        self.d = []
        self.f = []
        self.version = 0
        self.path = ""

        with open(dirIndexFile) as f:
            self.readFrom(f)

    def readFrom(self, readable):
        for line in readable:
            line = line.strip()
            if line.startswith('#'):
                continue

            tokens = line.split(':')
            if len(tokens) == 0:
                continue

            if tokens[0] == "version":
                self.version = int(tokens[1])

            elif tokens[0] == "path":
                self.path = tokens[1]

            elif tokens[0] == "d":
                self.d.append({ 'name': tokens[1], 'hash': tokens[2] })

            elif tokens[0] == "f":
                self.f.append({ 'name': tokens[1], 'hash': tokens[2], 'size': tokens[3] })

    def getVersion(self):
        return self.version

    def getPath(self):
        return self.path

    def getDirectories(self):
        return self.d

    def getFiles(self):
        return self.f

#################################################################################################################################
class HTTPDownloadRequest(HTTPGetCallback):
    def __init__(self, terrasync, src, dst, callback = None ):
        super().__init__(src, self.callback)
        self.terrasync = terrasync
        self.dst = dst
        self.mycallback = callback

    # 'httpResponse' is an http.client.HTTPResponse instance
    def callback(self, url, httpResponse):
        # I suspect this doesn't handle HTTP redirects and things like that. As
        # mentioned at <https://docs.python.org/3/library/http.client.html>,
        # http.client is a low-level interface that should normally not be used
        # directly!
        if httpResponse.status != 200:
            raise NetworkError("HTTP callback got status {status} for URL {url}"
                               .format(status=httpResponse.status, url=url))

        try:
            with open(self.dst, 'wb') as f:
                f.write(httpResponse.read())
        except HTTPException as exc:
            raise NetworkError("for URL {url}: {error}"
                               .format(url=url, error=exc)) from exc

        if self.mycallback != None:
            self.mycallback(self)


class HTTPSocketRequest(HTTPGetCallback):
    """HTTPGetCallback class whose callback returns a file-like object.

    The file-like object returned by the callback, and thus by
    HTTPGetter.get(), is a socket or similar. This allows one to read
    the data obtained from the network without necessarily storing it
    to a file.

    """
    def __init__(self, src):
        """Initialize an HTTPSocketRequest object.

        src -- path to the resource on the server (no protocol, no
               server name, just the path starting with a '/').

        """
        HTTPGetCallback.__init__(self, src, self.callback)

    def callback(self, url, httpResponse):
        # Same comment as for HTTPDownloadRequest.callback()
        if httpResponse.status != 200:
            raise NetworkError("HTTP callback got status {status} for URL {url}"
                               .format(status=httpResponse.status, url=url))

        return httpResponse

#################################################################################################################################

class Coordinate:
    def __init__(self, lat, lon):
        self.lat = lat
        self.lon = lon

class DownloadBoundaries:
    def __init__(self, top, left, bottom, right):
        if top < bottom:
            raise ValueError("top cannot be less than bottom")
        if right < left:
            raise ValueError("right cannot be less than left")

        if top > 90 or bottom < -90:
            raise ValueError("top and bottom must be a valid latitude")
        if left < -180 or right > 180:
            raise ValueError("left and right must be a valid longitude")
        self.top = top
        self.left = left
        self.bottom = bottom
        self.right = right

    def is_coordinate_inside_boundaries(self, coordinate):
        if coordinate.lat < self.bottom or coordinate.lat > self.top:
            return False
        if coordinate.lon < self.left or coordinate.lon > self.right:
            return False
        return True

def parse_terrasync_coordinate(coordinate):
    matches = re.match("(w|e)(\d{3})(n|s)(\d{2})", coordinate)
    if not matches:
        return None
    lon = int(matches.group(2))
    if matches.group(1) == "w":
        lon *= -1
    lat = int(matches.group(4))
    if matches.group(3) == "s":
        lat *= -1
    return Coordinate(lat, lon)


class Report:
    """Gather and format data about the state of a TerraSync mirror."""

    def __init__(self, targetDir):
        self.targetDir = targetDir

        self.dirsWithMissingIndex = set()
        self.dirsWithMismatchingDirIndexHash = set()
        self.missingFiles = set()
        self.filesWithMismatchingHash = set()
        self.dirsSkippedDueToBoundaries = set()

        self.orphanFiles = set()
        self.orphanDirs = set()

    def addMissingDirIndex(self, directoryRelPath):
        self.dirsWithMissingIndex.add(directoryRelPath)

    def addDirIndexWithMismatchingHash(self, directoryRelPath):
        self.dirsWithMismatchingDirIndexHash.add(directoryRelPath)

    def addMissingFile(self, relPath):
        self.missingFiles.add(relPath)

    def addFileWithMismatchingHash(self, relPath):
        self.filesWithMismatchingHash.add(relPath)

    def addSkippedDueToBoundaries(self, relPath):
        self.dirsSkippedDueToBoundaries.add(relPath)

    def addOrphanFile(self, relPath):
        self.orphanFiles.add(relPath)

    def addOrphanDir(self, relPath):
        self.orphanDirs.add(relPath)

    def summaryString(self):
        reportElements = [
            ("Directories with missing index", self.dirsWithMissingIndex),
            ("Directories whose .dirindex file had a mismatching hash",
             self.dirsWithMismatchingDirIndexHash),
            ("Missing files", self.missingFiles),
            ("Files with a mismatching hash", self.filesWithMismatchingHash),
            ("Directories skipped because of the specified boundaries",
             self.dirsSkippedDueToBoundaries),
            ("Orphan files", self.orphanFiles),
            ("Orphan directories", self.orphanDirs)]

        l = []
        for heading, setOfFilesOrDirs in reportElements:
            if setOfFilesOrDirs:
                l.append(heading + ":\n")
                l.extend( ( "  /" + f + '\n' for f in sorted(setOfFilesOrDirs)) )
            else:
                l.append(heading + ": none")

        return '\n'.join(l)

    def printReport(self):
        title = "{prg} report".format(prg=PROGNAME)
        print("\n" + title + '\n' + len(title)*"=", end="\n\n")
        print(self.summaryString())


@enum.unique
class FailedCheckReason(enum.Enum):
    """Reasons that can cause 'check' mode to report a mismatch.

    Note that network errors and things like that do *not* belong here.

    """

    missingDirIndexFile, mismatchingHashForDirIndexFile, \
        missingNormalFile, mismatchingHashForNormalFile, \
        orphanFile, orphanDirectory = range(6)

    # 'path': virtual path to a file or directory
    def explain(self, path):
        if self is FailedCheckReason.missingDirIndexFile:
            res = ".dirindex file '{}' is missing locally".format(path)
        elif self is FailedCheckReason.mismatchingHashForDirIndexFile:
            res = ".dirindex file '{}' doesn't have the hash it " \
                  "should have according to the server".format(path)
        elif self is FailedCheckReason.missingNormalFile:
            res = "file '{}' is present on the server but missing locally" \
                  .format(path)
        elif self is FailedCheckReason.mismatchingHashForNormalFile:
            res = "file '{}' doesn't have the hash given in the " \
                  ".dirindex file of its containing directory".format(path)
        elif self is FailedCheckReason.orphanFile:
            res = "file '{}' was found locally but is not present on the " \
                  "server".format(path)
        elif self is FailedCheckReason.orphanDirectory:
            res = "directory '{}' was found locally but is not present " \
                  "on the server".format(path)
        else:
            assert False, "Unhandled enum value: {!r}".format(self)

        return res


class TerraSync:

    @enum.unique
    class Mode(enum.Enum):
        """Main modes of operation for the TerraSync class."""

        # Using lower case for the member names, because this way
        # enumMember.name is exactly the mode string passed to --mode on the
        # command line (can be useful for messages destined to users).
        check, sync = range(2)

    def __init__(self, mode, doReport, url, target, quick, removeOrphan,
                 downloadBoundaries):
        self.mode = self.Mode[mode]
        self.doReport = doReport
        self.setUrl(url).setTarget(target)
        self.quick = quick
        self.removeOrphan = removeOrphan
        self.httpGetter = None
        self.downloadBoundaries = downloadBoundaries
        # Status of the local repository (as compared to what the server says),
        # before any update we might do to it.
        self.report = Report(self.target)

    def inSyncMode(self):
        return self.mode == self.Mode.sync

    def setUrl(self, url):
        self.url = url.rstrip('/').strip()
        return self

    def setTarget(self, target):
        # Using os.path.abspath() here is safer in case the process later uses
        # os.chdir(), which would change the meaning of the "." directory.
        self.target = os.path.abspath(target)
        return self

    def start(self):
        self.httpGetter = HTTPGetter(self.url)

        # Get the hash of the root .dirindex file
        try:
            request = HTTPSocketRequest("/.dirindex")
            with self.httpGetter.get(request) as fileLike:
                rootDirIndexHash = computeHash(fileLike)
        except HTTPException as exc:
            raise NetworkError("for the root .dirindex file: {errMsg}"
                               .format(errMsg=exc)) from exc

        # Process the root directory of the repository (recursive)
        self.processDirectoryEntry("", "", rootDirIndexHash)
        return self.report

    def processFileEntry(self, serverPath, localPath, fileHash):
        """Process a file entry from a .dirindex file."""
        localFullPath = join(self.target, localPath)
        failedCheckReason = None

        if not os.path.isfile(localFullPath):
            self.report.addMissingFile(localPath)
            failedCheckReason = FailedCheckReason.missingNormalFile
        elif hashForFile(localFullPath) != fileHash:
            self.report.addFileWithMismatchingHash(localPath)
            failedCheckReason = FailedCheckReason.mismatchingHashForNormalFile
        else:
            # The file exists and has the hash mentioned in the .dirindex file
            return

        assert failedCheckReason is not None

        if self.inSyncMode():
            if os.path.isdir(localFullPath):
                # 'localFullPath' is a directory (locally), but on the server
                # it is a file -> remove the dir so that we can store the file.
                removeDirectoryTree(self.target, localFullPath)

            print("Downloading '{}'".format(serverPath))
            request = HTTPDownloadRequest(self, serverPath, localFullPath )
            self.httpGetter.get(request)
        else:
            virtualPath = normalizeVirtualPath(serverPath)
            self.abortCheckMode(failedCheckReason, virtualPath)

    def processDirectoryEntry(self, serverPath, localPath, dirIndexHash):
        """Process a directory entry from a .dirindex file."""
        virtualPath = normalizeVirtualPath(serverPath)
        print("Processing '{}'...".format(virtualPath))

        if serverPath:
            serverFolderName = os.path.basename(serverPath)
            coordinate = parse_terrasync_coordinate(serverFolderName)
            if (coordinate and
                not self.downloadBoundaries.is_coordinate_inside_boundaries(
                    coordinate)):
                self.report.addSkippedDueToBoundaries(localPath)
                return

        localFullPath = join(self.target, localPath)
        localDirIndex = join(localFullPath, ".dirindex")
        failedCheckReason = None

        if not os.path.isfile(localDirIndex):
            failedCheckReason = FailedCheckReason.missingDirIndexFile
            self.report.addMissingDirIndex(localPath)
        elif hashForFile(localDirIndex) != dirIndexHash:
            failedCheckReason = FailedCheckReason.mismatchingHashForDirIndexFile
            self.report.addDirIndexWithMismatchingHash(localPath)

        if failedCheckReason is None:
            if not self.quick:
                self.handleDirindexFile(localDirIndex)
        elif self.inSyncMode():
            if not os.path.exists(localFullPath):
                os.makedirs(localFullPath)

            request = HTTPDownloadRequest(self,
                                          serverPath + "/.dirindex",
                                          localDirIndex,
                                          self.handleDirindexRequest)
            self.httpGetter.get(request)
        else:
            vPath = normalizeVirtualPath(virtualPath + "/.dirindex")
            self.abortCheckMode(failedCheckReason, vPath)

    def handleDirindexRequest(self, dirindexRequest):
        self.handleDirindexFile(dirindexRequest.dst)

    def handleDirindexFile(self, dirindexFile):
        dirIndex = DirIndex(dirindexFile)
        root = "/" + dirIndex.getPath() if dirIndex.getPath() else ""
        serverFiles = []
        serverDirs = []

        for file in dirIndex.getFiles():
            f = file['name']
            self.processFileEntry(root + "/" + f,
                                  join(dirIndex.getPath(), f),
                                  file['hash'])
            serverFiles.append(f)

        for subdir in dirIndex.getDirectories():
            d = subdir['name']
            self.processDirectoryEntry(root + "/" + d,
                                       join(dirIndex.getPath(), d),
                                       subdir['hash'])
            serverDirs.append(d)

        localFullPath = join(self.target, dirIndex.getPath())
        localFiles = [ f for f in listdir(localFullPath)
                       if isfile(join(localFullPath, f)) ]

        for f in localFiles:
            if f != ".dirindex" and f not in serverFiles:
                relPath = dirIndex.getPath() + '/' + f # has no leading '/'
                self.report.addOrphanFile(relPath)

                if self.inSyncMode():
                    if self.removeOrphan:
                        os.remove(join(self.target, relPath))
                else:
                    self.abortCheckMode(FailedCheckReason.orphanFile,
                                        normalizeVirtualPath(relPath))

        localDirs = [ f for f in listdir(localFullPath)
                      if isdir(join(localFullPath, f)) ]

        for d in localDirs:
            if d not in serverDirs:
                relPath = dirIndex.getPath() + '/' + d # has no leading '/'
                self.report.addOrphanDir(relPath)

                if self.inSyncMode():
                    if self.removeOrphan:
                        removeDirectoryTree(self.target,
                                            join(self.target, relPath))
                else:
                    self.abortCheckMode(FailedCheckReason.orphanDirectory,
                                        normalizeVirtualPath(relPath))

    # 'reason' is a member of the FailedCheckReason enum
    def abortCheckMode(self, reason, fileOrDirVirtualPath):
        assert self.mode == self.Mode.check, self.mode

        print("{prg}: exiting from 'check' mode because {explanation}."
              .format(prg=PROGNAME,
                      explanation=reason.explain(fileOrDirVirtualPath)))

        if self.doReport:
            self.report.printReport()

        sys.exit(ExitStatus.CHECK_MODE_FOUND_MISMATCH.value)

#################################################################################################################################

def parseCommandLine():
    parser = argparse.ArgumentParser()

    parser.add_argument("-u", "--url", dest="url", metavar="URL",
                        default="http://flightgear.sourceforge.net/scenery",
                        help="server URL [default: %(default)s]")

    parser.add_argument("-t", "--target", dest="target", metavar="DIR",
                        default=".", help="""\
      directory where to store the files [default: the current directory]""")

    parser.add_argument("-q", "--quick", dest="quick", action="store_true",
                        default=False, help="enable quick mode")

    parser.add_argument("-r", "--remove-orphan", dest="removeOrphan",
                        action="store_true",
                        default=False, help="remove old scenery files")

    parser.add_argument("--mode", default="sync", choices=("check", "sync"),
                        help="""\
      main mode of operation (default: '%(default)s'). In 'sync' mode, contents
      is downloaded from the server to the target directory. On the other hand,
      in 'check' mode, {progname} compares the contents of the target directory
      with the remote repository without writing nor deleting anything on
      disk.""".format(progname=PROGNAME))

    parser.add_argument("--report", dest="report", action="store_true",
                        default=False,
                        help="""\
      before normal exit, print a report of what was found""")

    parser.add_argument("--top", dest="top", type=int, default=90, help="""\
      maximum latitude to include in download [default: %(default)d]""")

    parser.add_argument("--bottom", dest="bottom", type=int, default=-90,
                        help="""\
      minimum latitude to include in download [default: %(default)d]""")

    parser.add_argument("--left", dest="left", type=int, default=-180, help="""\
      minimum longitude to include in download [default: %(default)d]""")
    parser.add_argument("--right", dest="right", type=int, default=180,
                        help="""\
      maximum longitude to include in download [default: %(default)d]""")

    args = parser.parse_args()

    # Perform consistency checks on the arguments
    if args.mode == "check" and args.removeOrphan:
        print("{prg}: 'check' mode is read-only and thus doesn't make sense "
              "with\noption --remove-orphan (-r)".format(prg=PROGNAME),
              file=sys.stderr)
        sys.exit(ExitStatus.ERROR.value)

    return args


def main():
    args = parseCommandLine()
    terraSync = TerraSync(args.mode, args.report, args.url, args.target,
                          args.quick, args.removeOrphan,
                          DownloadBoundaries(args.top, args.left, args.bottom,
                                             args.right))
    report = terraSync.start()

    if args.report:
        report.printReport()

    sys.exit(ExitStatus.SUCCESS.value)
