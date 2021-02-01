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
import re
import shutil
import ssl
import sys
import time
import urllib

from urllib.parse import urlparse, urljoin
from http.client import HTTPConnection, HTTPSConnection, HTTPException
from os import listdir
from os.path import isfile, isdir, join
from base64 import b64encode

from . import dirindex
from .exceptions import UserError, NetworkError, RepoDataError, \
                        InvalidDirIndexFile, UnsupportedURLScheme
from .virtual_path import VirtualPath


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

    if not os.path.isfile(join(absPath, ".dirindex")):
        raise UserError("refusing to recursively delete '{}' because "
                        "it does not contain a .dirindex file".format(absPath))
    elif _removeDirectoryTree_dangerous_cre.match(absPath):
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


# *****************************************************************************
# *                          Network-related classes                          *
# *****************************************************************************

class HTTPGetCallback:
    def __init__(self, src, callback):
        """Initialize an HTTPGetCallback instance.

        src      -- a VirtualPath instance (corresponding to the path on
                    the server for which a GET request is to be issued)
        callback -- a function taking two parameters: the URL (string)
                    and an http.client.HTTPResponse instance. When
                    invoked, the callback return value will be returned
                    by HTTPGetter.get().

        """
        if callback is not None:
            self.callback = callback
        self.src = src

class HTTPGetter:
    def __init__(self, baseUrl, maxPending=10, auth=""):
        self.baseUrl = baseUrl
        self.parsedBaseUrl = urlparse(baseUrl)
        self.maxPending = maxPending
        self.requests = []
        self.pendingRequests = []

        if self.parsedBaseUrl.scheme == "http":
            self.httpConnection = HTTPConnection(self.parsedBaseUrl.netloc)
        elif self.parsedBaseUrl.scheme == "https":
            context = ssl.create_default_context()
            self.httpConnection = HTTPSConnection(self.parsedBaseUrl.netloc,
                                                  context=context)
        else:
            raise UnsupportedURLScheme(self.parsedBaseUrl.scheme)

        self.httpRequestHeaders = headers = {'Host':self.parsedBaseUrl.netloc,'Content-Length':0,'Connection':'Keep-Alive','User-Agent':'FlightGear terrasync.py'}
        if( auth and not auth.isspace() ):
            self.httpRequestHeaders['Authorization'] = 'Basic %s' % b64encode(auth.encode("utf-8")).decode("ascii")

    def assemblePath(self, httpGetCallback):
        """Return the path-on-server for the file to download.

        Example: '/scenery/Airports/N/E/4/.dirindex'

        """
        assert not self.parsedBaseUrl.path.endswith('/'), \
            repr(self.parsedBaseUrl)
        return self.parsedBaseUrl.path + str(httpGetCallback.src)

    def assembleUrl(self, httpGetCallback):
        """Return the URL of the file to download."""
        baseUrl = self.parsedBaseUrl.geturl()
        assert not baseUrl.endswith('/'), repr(baseUrl)

        return urljoin(baseUrl + '/', httpGetCallback.src.asRelative())

    def doGet(self, httpGetCallback):
        time.sleep(1.25)    # throttle the rate

        pathOnServer = self.assemblePath(httpGetCallback)
        self.httpConnection.request("GET", pathOnServer, None,
                                    self.httpRequestHeaders)
        httpResponse = self.httpConnection.getresponse()

        # 'httpResponse' is an http.client.HTTPResponse instance
        return httpGetCallback.callback(self.assembleUrl(httpGetCallback),
                                        httpResponse)

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


class HTTPDownloadRequest(HTTPGetCallback):
    def __init__(self, src, dst, callback=None):
        """Initialize an HTTPDownloadRequest instance.

        src       -- a VirtualPath instance (corresponding to the path
                     on the server for which a GET request is to be
                     issued)
        dst       -- file path (or whatever open() accepts) where the
                     downloaded data is to be stored
        callback  -- a function that will be called if the download is
                     successful, or None if no such callback is desired.
                     The function must take one parameter: when invoked,
                     it will be passed this HTTPDownloadRequest
                     instance. Its return value is ignored.

        """
        HTTPGetCallback.__init__(self, src, None)
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

        if self.mycallback is not None:
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

        src -- VirtualPath instance for the resource on the server
               (presumably a file)

        """
        HTTPGetCallback.__init__(self, src, None)

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
            # right may be less than left when wrapping across the antimeridian
            if not (left >= 0 and right < 0):
                raise ValueError("right cannot be less than left")

        if top > 90 or bottom < -90:
            raise ValueError("top and bottom must be a valid latitude")
        if left < -180 or right >= 180:
            raise ValueError("left and right must be a valid longitude")
        self.top = top
        self.left = left
        self.bottom = bottom
        self.right = right

    def is_coordinate_inside_boundaries(self, coordinate, isOuterBucket):
        bigTileBottom = coordinate.lat
        bigTileTop = bigTileBottom + (10 if isOuterBucket else 1)
        bigTileLeft = coordinate.lon
        bigTileRight = bigTileLeft + (10 if isOuterBucket else 1)

        # if the two regions do not overlap then we are done
        if bigTileTop <= self.bottom or bigTileBottom > self.top:
            return False
        if bigTileRight <= self.left or bigTileLeft > self.right:
            # check for spanning across the antimeridian
            if self.left >= 0 and self.right < 0:
                # determine which side we are on and check of region overlap
                if bigTileLeft >= 0:
                    if bigTileRight <= self.left:
                        return False
                elif bigTileLeft > self.right:
                    return False
            else:
                return False

        # at least a partial overlap exists, so more processing will be needed
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

    def addMissingDirIndex(self, directoryVirtualPath):
        self.dirsWithMissingIndex.add(directoryVirtualPath)

    def addDirIndexWithMismatchingHash(self, directoryVirtualPath):
        self.dirsWithMismatchingDirIndexHash.add(directoryVirtualPath)

    def addMissingFile(self, virtualPath):
        self.missingFiles.add(virtualPath)

    def addFileWithMismatchingHash(self, virtualPath):
        self.filesWithMismatchingHash.add(virtualPath)

    def addSkippedDueToBoundaries(self, virtualPath):
        self.dirsSkippedDueToBoundaries.add(virtualPath)

    def addOrphanFile(self, virtualPath):
        self.orphanFiles.add(virtualPath)

    def addOrphanDir(self, virtualPath):
        self.orphanDirs.add(virtualPath)

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
                l.extend( ("  " + str(f) for f in sorted(setOfFilesOrDirs)) )
                l.append('')    # ensure a blank line follows the list
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

    # 'path': VirtualPath instance for a file or directory
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
                 downloadBoundaries, auth):
        self.mode = self.Mode[mode]
        self.doReport = doReport
        self.setUrl(url).setTarget(target)
        self.auth = auth
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

    def start(self, virtualSubdir=VirtualPath('/')):
        """Start the 'sync' or 'check' process.

        The 'virtualSubdir' argument must be a VirtualPath instance and
        allows one to start the 'sync' or 'check' process in a chosen
        subdirectory of the TerraSync repository, instead of at its
        root.

        """
        # Remove the leading '/' from 'virtualSubdir' and convert to native
        # separators ('/' or '\' depending on the platform).
        localSubdir = os.path.normpath(virtualSubdir.asRelative())
        if localSubdir == ".":  # just ugly, but it wouldn't hurt
            localSubdir = ""

        assert not os.path.isabs(localSubdir), repr(localSubdir)
        self.httpGetter = HTTPGetter(baseUrl=self.url,auth=self.auth)

        # Get the hash of the .dirindex file for 'virtualSubdir'
        try:
            request = HTTPSocketRequest(virtualSubdir / ".dirindex")
            with self.httpGetter.get(request) as fileLike:
                dirIndexHash = computeHash(fileLike)
        except HTTPException as exc:
            raise NetworkError("for the root .dirindex file: {errMsg}"
                               .format(errMsg=exc)) from exc

        # Process the chosen part of the repository (recursive)
        self.processDirectoryEntry(virtualSubdir, localSubdir, dirIndexHash)

        return self.report

    def processFileEntry(self, virtualPath, localPath, fileHash):
        """Process a file entry from a .dirindex file."""
        localFullPath = join(self.target, localPath)
        failedCheckReason = None

        if not os.path.isfile(localFullPath):
            self.report.addMissingFile(virtualPath)
            failedCheckReason = FailedCheckReason.missingNormalFile
        elif hashForFile(localFullPath) != fileHash:
            self.report.addFileWithMismatchingHash(virtualPath)
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

            print("Downloading '{}'".format(virtualPath))
            request = HTTPDownloadRequest(virtualPath, localFullPath)
            self.httpGetter.get(request)
        else:
            self.abortCheckMode(failedCheckReason, virtualPath)

    def processDirectoryEntry(self, virtualPath, localPath, dirIndexHash):
        """Process a directory entry from a .dirindex file."""
        print("Processing '{}'...".format(virtualPath))
        isOuterBucket = True if len(virtualPath.parts) <= 3 else False

        coord = parse_terrasync_coordinate(virtualPath.name)

        if (coord and
            not self.downloadBoundaries.is_coordinate_inside_boundaries(coord, isOuterBucket)):
            self.report.addSkippedDueToBoundaries(virtualPath)
            return

        localFullPath = join(self.target, localPath)
        localDirIndex = join(localFullPath, ".dirindex")
        failedCheckReason = None

        if not os.path.isfile(localDirIndex):
            failedCheckReason = FailedCheckReason.missingDirIndexFile
            self.report.addMissingDirIndex(virtualPath)
        elif hashForFile(localDirIndex) != dirIndexHash:
            failedCheckReason = FailedCheckReason.mismatchingHashForDirIndexFile
            self.report.addDirIndexWithMismatchingHash(virtualPath)

        if failedCheckReason is None:
            if not self.quick:
                self.handleDirindexFile(localDirIndex)
        elif self.inSyncMode():
            if os.path.isfile(localFullPath):
                os.unlink(localFullPath) # file on server became a directory
            if not os.path.exists(localFullPath):
                os.makedirs(localFullPath)

            request = HTTPDownloadRequest(virtualPath / ".dirindex",
                                          localDirIndex,
                                          self.handleDirindexRequest)
            self.httpGetter.get(request)
        else:
            self.abortCheckMode(failedCheckReason, virtualPath / ".dirindex")

    def handleDirindexRequest(self, dirindexRequest):
        self.handleDirindexFile(dirindexRequest.dst)

    def handleDirindexFile(self, dirindexFile):
        dirIndex = dirindex.DirIndex(dirindexFile)
        virtualBase = dirIndex.path             # VirtualPath instance
        relativeBase = virtualBase.asRelative() # string, doesn't start with '/'
        serverFiles = []
        serverDirs = []

        for file in dirIndex.files:
            f = file['name']
            self.processFileEntry(virtualBase / f,
                                  join(relativeBase, f),
                                  file['hash'])
            serverFiles.append(f)

        for subdir in dirIndex.directories:
            d = subdir['name']
            self.processDirectoryEntry(virtualBase / d,
                                       join(relativeBase, d),
                                       subdir['hash'])
            serverDirs.append(d)

        for tarball in dirIndex.tarballs:
            # Tarballs are handled the same as normal files.
            f = tarball['name']
            self.processFileEntry(virtualBase / f,
                                  join(relativeBase, f),
                                  tarball['hash'])
            serverFiles.append(f)

        localFullPath = join(self.target, relativeBase)
        localFiles = [ f for f in listdir(localFullPath)
                       if isfile(join(localFullPath, f)) ]

        for f in localFiles:
            if f != ".dirindex" and f not in serverFiles:
                virtualPath = virtualBase / f
                self.report.addOrphanFile(virtualPath)

                if self.inSyncMode():
                    if self.removeOrphan:
                        os.remove(join(self.target, virtualPath.asRelative()))
                else:
                    self.abortCheckMode(FailedCheckReason.orphanFile,
                                        virtualPath)

        localDirs = [ f for f in listdir(localFullPath)
                      if isdir(join(localFullPath, f)) ]

        for d in localDirs:
            if d not in serverDirs:
                virtualPath = virtualBase / d
                self.report.addOrphanDir(virtualPath)

                if self.inSyncMode():
                    if self.removeOrphan:
                        removeDirectoryTree(self.target,
                                            join(self.target,
                                                 virtualPath.asRelative()))
                else:
                    self.abortCheckMode(FailedCheckReason.orphanDirectory,
                                        virtualPath)

    # 'reason' is a member of the FailedCheckReason enum
    def abortCheckMode(self, reason, fileOrDirVirtualPath):
        assert self.mode == self.Mode.check, repr(self.mode)

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

    parser.add_argument("-a", "--auth", dest="auth", metavar="user:password",
                        default="", help="""\
      authentication credentials for basic auth [default: empty, no authentication]""")

    parser.add_argument("-t", "--target", dest="target", metavar="DIR",
                        default=".", help="""\
      directory where to store the files [default: the current directory]""")

    parser.add_argument("--only-subdir", dest="onlySubdir", metavar="SUBDIR",
                        default="", help="""\
      restrict processing to this subdirectory of the TerraSync repository. Use
      a path relative to the repository root, for instance 'Models/Residential'
      [default: process the whole repository]""")

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
    parser.add_argument("--right", dest="right", type=int, default=179,
                        help="""\
      maximum longitude to include in download [default: %(default)d]""")

    args = parser.parse_args()

    # Perform consistency checks on the arguments
    if args.mode == "check" and args.removeOrphan:
        print("{prg}: 'check' mode is read-only and thus doesn't make sense "
              "with\noption --remove-orphan (-r)".format(prg=PROGNAME),
              file=sys.stderr)
        sys.exit(ExitStatus.ERROR.value)

    # Replace backslashes with forward slashes, remove leading and trailing
    # slashes, collapse consecutive slashes. Yes, this implies that we tolerate
    # leading slashes for --only-subdir (which makes sense because virtual
    # paths are printed like that by this program, therefore it is natural for
    # users to copy & paste such paths in order to use them for --only-subdir).
    args.virtualSubdir = VirtualPath(args.onlySubdir.replace('\\', '/'))

    # Be nice to our user in case the path starts with '\', 'C:\', etc.
    if os.path.isabs(args.virtualSubdir.asRelative()):
        print("{prg}: option --only-subdir expects a *relative* path, but got "
              "'{subdir}'".format(prg=PROGNAME, subdir=args.onlySubdir),
              file=sys.stderr)
        sys.exit(ExitStatus.ERROR.value)

    return args


def main():
    args = parseCommandLine()
    terraSync = TerraSync(args.mode, args.report, args.url, args.target,
                          args.quick, args.removeOrphan,
                          DownloadBoundaries(args.top, args.left, args.bottom,
                                             args.right),args.auth)
    report = terraSync.start(args.virtualSubdir)

    if args.report:
        report.printReport()

    sys.exit(ExitStatus.SUCCESS.value)
