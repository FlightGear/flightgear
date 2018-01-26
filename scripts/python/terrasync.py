#!/usr/bin/env python3
# -*- coding: utf-8 -*-
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
#
# terrasync.py - synchronize terrascenery data to your local disk
# needs dnspython (pip install dnspython)

import urllib, os, hashlib
from urllib.parse import urlparse
from http.client import HTTPConnection, _CS_IDLE, HTTPException
from os import listdir
from os.path import isfile, isdir, join
import re
import argparse
import shutil
import time


# *****************************************************************************
# *                             Custom exceptions                             *
# *****************************************************************************

# Generic exception class for terrasync.py, to be subclassed for each specific
# kind exception.
class TerraSyncPyException(Exception):
    def __init__(self, message=None, *, mayCapitalizeMsg=True):
        """Initialize a TerraSyncPyException instance.

        Except in cases where 'message' starts with a proper noun or
        something like that, its first character should be given in
        lower case. Automated treatments of this exception may print the
        message with its first character changed to upper case, unless
        'mayCapitalizeMsg' is False. In other words, if the case of the
        first character of 'message' must not be changed under any
        circumstances, set 'mayCapitalizeMsg' to False.

        """
        self.message = message
        self.mayCapitalizeMsg = mayCapitalizeMsg

    def __str__(self):
        return self.completeMessage()

    def __repr__(self):
        return "{}.{}({!r})".format(__name__, type(self).__name__, self.message)

    # Typically overridden by subclasses with a custom constructor
    def detail(self):
        return self.message

    def completeMessage(self):
        if self.message:
            return "{shortDesc}: {detail}".format(
                shortDesc=self.ExceptionShortDescription,
                detail=self.detail())
        else:
            return self.ExceptionShortDescription

    ExceptionShortDescription = "terrasync.py generic exception"


class UserError(TerraSyncPyException):
     """Exception raised when the program is used in an incorrect way."""
     ExceptionShortDescription = "User error"

class NetworkError(TerraSyncPyException):
     """Exception raised when getting a network error even after retrying."""
     ExceptionShortDescription = "Network error"


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

#################################################################################################################################

def hash_of_file(fname):
    hash = hashlib.sha1()

    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash.update(chunk)

    return hash.hexdigest()

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

class TerraSync:

    def __init__(self, url, target, quick, removeOrphan, downloadBoundaries):
        self.setUrl(url).setTarget(target)
        self.quick = quick
        self.removeOrphan = removeOrphan
        self.httpGetter = None
        self.downloadBoundaries = downloadBoundaries

    def setUrl(self, url):
        self.url = url.rstrip('/').strip()
        return self

    def setTarget(self, target):
        self.target = target.rstrip('/').strip()
        return self

    def start(self):
        self.httpGetter = HTTPGetter(self.url)
        self.updateDirectory("", "", None )

    def updateFile(self, serverPath, localPath, fileHash ):
        localFullPath = join(self.target, localPath)
        if fileHash != None and hash_of_file(localFullPath) == fileHash:
            #print("hash of file matches, not downloading")
            return

        print("Downloading '{}'".format(serverPath))

        request = HTTPDownloadRequest(self, serverPath, localFullPath )
        self.httpGetter.get(request)


    def updateDirectory(self, serverPath, localPath, dirIndexHash):
        print("Processing '{}'...".format(serverPath))

        if serverPath:
            serverFolderName = os.path.basename(serverPath)
            coordinate = parse_terrasync_coordinate(serverFolderName)
            if coordinate and not self.downloadBoundaries.is_coordinate_inside_boundaries(coordinate):
                return

        localFullPath = join(self.target, localPath)
        if not os.path.exists( localFullPath ):
          os.makedirs( localFullPath )

        localDirIndex = join(localFullPath, ".dirindex")
        if dirIndexHash != None and  hash_of_file(localDirIndex) == dirIndexHash:
            # print("hash of dirindex matches, not downloading")
            if not self.quick:
                self.handleDirindexFile( localDirIndex )
        else:
            request = HTTPDownloadRequest(self,
                                          serverPath + "/.dirindex",
                                          localDirIndex,
                                          self.handleDirindexRequest)
            self.httpGetter.get(request)

    def handleDirindexRequest(self, dirindexRequest):
        self.handleDirindexFile(dirindexRequest.dst)

    def handleDirindexFile(self, dirindexFile):
        dirIndex = DirIndex(dirindexFile)
        serverFiles = []
        serverDirs = []

        for file in dirIndex.getFiles():
            f = file['name']
            h = file['hash']
            self.updateFile("/" + dirIndex.getPath() + "/" + f,
                            join(dirIndex.getPath(), f),
                            h)
            serverFiles.append(f)

        for subdir in dirIndex.getDirectories():
            d = subdir['name']
            h = subdir['hash']
            self.updateDirectory("/" + dirIndex.getPath() + "/" + d,
                                 join(dirIndex.getPath(), d),
                                 h)
            serverDirs.append(d)

        if self.removeOrphan:
            localFullPath = join(self.target, dirIndex.getPath())
            localFiles = [ f for f in listdir(localFullPath)
                           if isfile(join(localFullPath, f)) ]
            for f in localFiles:
                if f != ".dirindex" and not f in serverFiles:
                    #print("removing orphan file", join(localFullPath,f) )
                    os.remove( join(localFullPath,f) )
            localDirs = [ f for f in listdir(localFullPath)
                          if isdir(join(localFullPath, f)) ]
            for f in localDirs:
                if not f in serverDirs:
                    #print ("removing orphan dir",f)
                    removeDirectoryTree(self.target, join(localFullPath, f))

#################################################################################################################################


parser = argparse.ArgumentParser()
parser.add_argument("-u", "--url", dest="url", metavar="URL",
        default="http://flightgear.sourceforge.net/scenery", help="Server URL [default: %(default)s]")
parser.add_argument("-t", "--target", dest="target", metavar="DIR",
        default=".", help="Directory to store the files [default: current directory]")
parser.add_argument("-q", "--quick", dest="quick", action="store_true",
        default=False, help="Quick")
parser.add_argument("-r", "--remove-orphan", dest="removeOrphan", action="store_true",
        default=False, help="Remove old scenery files")

parser.add_argument("--top", dest="top", type=int,
        default=90, help="Maximum latitude to include in download [default: %(default)d]")
parser.add_argument("--bottom", dest="bottom", type=int,
        default=-90, help="Minimum latitude to include in download [default: %(default)d]")
parser.add_argument("--left", dest="left", type=int,
        default=-180, help="Minimum longitude to include in download [default: %(default)d]")
parser.add_argument("--right", dest="right", type=int,
        default=180, help="Maximum longitude to include in download [default: %(default)d]")

args = parser.parse_args()

terraSync = TerraSync(args.url, args.target, args.quick, args.removeOrphan,
        DownloadBoundaries(args.top, args.left, args.bottom, args.right))

terraSync.start()
