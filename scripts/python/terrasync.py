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
from http.client import HTTPConnection, _CS_IDLE
from os import listdir
from os.path import isfile, join

#################################################################################################################################
class HTTPGetCallback:
    def __init__(self, src, callback):
        self.callback = callback
        self.src = src
        self.result = None

class HTTPGetter:
    def __init__(self, baseUrl, maxPending=10):
        self.baseUrl = baseUrl
        self.parsedBaseUrl = urlparse(baseUrl)
        self.maxPending = maxPending
        self.requests = []
        self.pendingRequests = []
        self.httpConnection = HTTPConnection(self.parsedBaseUrl.netloc)
        self.httpRequestHeaders = headers = {'Host':self.parsedBaseUrl.netloc,'Content-Length':0,'Connection':'Keep-Alive','User-Agent':'FlightGear terrasync.py'}

    def doGet(self, httpGetCallback):
        conn = self.httpConnection
        request = httpGetCallback
        self.httpConnection.request("GET", self.parsedBaseUrl.path + request.src, None, self.httpRequestHeaders)
        httpGetCallback.result = self.httpConnection.getresponse()
        httpGetCallback.callback()

    def get(self, httpGetCallback):

        try:
            self.doGet(httpGetCallback)
        except:
            # try to reconnect once
            #print("reconnect")
            self.httpConnection.close()
            self.httpConnection.connect()
            self.doGet(httpGetCallback)


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

    def callback(self):
        with open(self.dst, 'wb') as f:
            f.write(self.result.read())

        if self.mycallback != None:
            self.mycallback(self)

#################################################################################################################################

def hash_of_file(fname):
    if not os.path.exists( fname ):
      return None

    hash = hashlib.sha1()
    try:
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash.update(chunk)
    except:
        pass

    return hash.hexdigest()

#################################################################################################################################
class TerraSync:

    def __init__(self, url="http://flightgear.sourceforge.net/scenery", target=".", quick=False, removeOrphan=False):
        self.setUrl(url).setTarget(target)
        self.quick = quick
        self.removeOrphan = removeOrphan
        self.httpGetter = None

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

        print("downloading ", serverPath )

        request = HTTPDownloadRequest(self, serverPath, localFullPath )
        self.httpGetter.get(request)


    def updateDirectory(self, serverPath, localPath, dirIndexHash):
        print("processing ", serverPath)

        localFullPath = join(self.target, localPath)
        if not os.path.exists( localFullPath ):
          os.makedirs( localFullPath )

        localDirIndex = join(localFullPath, ".dirindex")
        if dirIndexHash != None and  hash_of_file(localDirIndex) == dirIndexHash:
            # print("hash of dirindex matches, not downloading")
            if not self.quick:
                self.handleDirindexFile( localDirIndex )
        else:
            request = HTTPDownloadRequest(self, serverPath + "/.dirindex", localDirIndex, self.handleDirindexRequest )
            self.httpGetter.get(request)

    def handleDirindexRequest(self, dirindexRequest):
        self.handleDirindexFile(dirindexRequest.dst)

    def handleDirindexFile(self, dirindexFile):
        dirIndex = DirIndex(dirindexFile)
        serverFiles = []

        for file in dirIndex.getFiles():
            f = file['name']
            h = file['hash']
            self.updateFile( "/" + dirIndex.getPath() + "/" + f, join(dirIndex.getPath(),f), h )
            serverFiles.append(f)

        for subdir in dirIndex.getDirectories():
            d = subdir['name']
            h = subdir['hash']
            self.updateDirectory( "/" + dirIndex.getPath() + "/" + d, join(dirIndex.getPath(),d), h )

        if self.removeOrphan:
            localFullPath = join(self.target, dirIndex.getPath())
            localFiles = [f for f in listdir(localFullPath) if isfile(join(localFullPath, f))]
            for f in localFiles:
                if f != ".dirindex" and not f in serverFiles:
                    #print("removing orphan", join(localFullPath,f) )
                    os.remove( join(localFullPath,f) )


    def isReady(self):
        return self.httpGetter and self.httpGetter.isReady()
        return False

    def update(self):
        if self.httpGetter:
            self.httpGetter.update()

#################################################################################################################################
import getopt, sys
try:
  opts, args = getopt.getopt(sys.argv[1:], "u:t:qr", [ "url=", "target=", "quick", "remove-orphan" ])

except getopt.GetoptError:
  print("terrasync.py [--url=http://some.server.org/scenery] [--target=/some/path] [-q|--quick] [-r|--remove-orphan]")
  sys.exit(2)

terraSync = TerraSync()
for opt, arg in opts:
  if opt in("-u", "--url"):
    terraSync.url = arg

  elif opt in ("-t", "--target"):
    terraSync.target = arg

  elif opt in ("-q", "--quick"):
    terraSync.quick = True

  elif opt in ("-r", "--remove-orphan"):
    terraSync.removeOrphan = True

terraSync.start()
