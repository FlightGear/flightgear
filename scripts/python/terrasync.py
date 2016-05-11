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
#

import os 
import hashlib
import urllib.request
from os import listdir
from os.path import isfile, join

dirindex = ".dirindex"
DIRINDEXVERSION = 1

URL="http://flightgear.sourceforge.net/scenery"
# User master repository for now
#URL="automatic"
TARGET="."
QUICK=False
REMOVE_ORPHAN=False

########################################################################

def fn_hash_of_file(fname):
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

########################################################################
def do_download_file( _url, _path, _localfile, _hash, _force ):
  if os.path.exists( _localfile ) and not _force:
    h = fn_hash_of_file(_localfile)
    if h == _hash:
      #print("hash match for ", _localfile)
      return False

  r = urllib.request.urlopen( _url + _path )
  with open(_localfile, 'wb') as f:
    f.write( r.read() )
  #print("downloaded ", _localfile, " from ", _url + _path )
  return True

########################################################################
def do_terrasync( _url, _path, _localdir, _dirIndexHash ):
  url = _url + _path
  print(url)

  if not os.path.exists( _localdir ):
    os.makedirs( _localdir )

  # download and process .dirindex as temporary file
  # rename to .dirindex after successful processing of directory
  # in case of abort, .dirindex.tmp will be removed as orphan
  myDirIndexFile = os.path.join(_localdir, ".dirindex.tmp")

  try:
    if not do_download_file( url, "/.dirindex", myDirIndexFile, _dirIndexHash, QUICK == False ):
      # dirindex hash matches, file not downloaded, skip directory
      return

  except urllib.error.HTTPError as err:
    if err.code == 404 and _path == "":
      # HACK: only the master on SF provides .dirindex for root, fake it if it's missing
      print("Using static root hack.")
      for _sub in ("Models", "Terrain", "Objects", "Airports" ):
        do_terrasync( _url, "/" + _sub, os.path.join(_localdir,_sub), None )
      return

    else:
      raise

  with open(myDirIndexFile, 'r') as myDirIndex:
    serverFiles = []
    for line in myDirIndex:
      tokens = line.rstrip().split(':')
      if( len(tokens) == 0 ):
        continue

      # TODO: check version number, should be equal to DIRINDEXVERSION
      #       otherwise complain and terminate
      if( tokens[0] == "version" ):
        continue

      if( tokens[0] == "path" ):
        continue

      if( tokens[0] == "d" ):
        do_terrasync( url,  "/" + tokens[1], os.path.join(_localdir,tokens[1]), tokens[2] )

      if( tokens[0] == "f" ):
        do_download_file( url, "/" + tokens[1], os.path.join(_localdir,tokens[1]), tokens[2], False )
        serverFiles.append( tokens[1] )

  os.rename( myDirIndexFile, os.path.join(_localdir, ".dirindex" ) )

  localFiles = [f for f in listdir(_localdir) if isfile(join(_localdir, f))]
  for f in localFiles:
    if f != ".dirindex" and not f in serverFiles:
      if REMOVE_ORPHAN:
        os.remove( os.path.join(_localdir,f) )

  #TODO: cleanup orphan files

########################################################################

import getopt, sys, random, re

try:
  opts, args = getopt.getopt(sys.argv[1:], "u:t:qr", [ "url=", "target=", "quick", "remove-orphan" ])

except getopt.GetoptError:
  print("terrasync.py [--url=http://some.server.org/scenery] [--target=/some/path] [-q|--quick] [-r|--remove-orphan]")
  sys.exit(2)

for opt, arg in opts:
  if opt in( "-u", "--url"):
    URL = arg

  elif opt in ( "-t", "--target"):
    TARGET= arg

  elif opt in ("-q", "--quick"):
    QUICK = True

  elif opt in ("-r", "--remove-orphan"):
    REMOVE_ORPHAN = True

# automatic URL lookup from DNS NAPTR
# - lookup terrasync.flightgear.org, type=NAPTR, service="ws20", flags="U"
# - sort by order,preference ascending
# - pick entries with lowest order and preference
# - randomly pick one of those
# - use regexp fields URL
if URL == "automatic":
  import dns.resolver
  dnsResolver = dns.resolver.Resolver()

  order = -1
  preference = -1

  # find lowes preference/order for service 'ws20' and flags 'U'
  dnsAnswer = dnsResolver.query("terrasync.flightgear.org", "NAPTR" )
  for naptr in dnsAnswer:
    if naptr.service != b'ws20' or naptr.flags != b'U':
      continue

    if order == -1 or naptr.order < order:
      order = naptr.order
      preference = naptr.preference

    if order == naptr.order:
      if naptr.preference < preference:
        preference = naptr.preference


  # grab candidats
  candidates = []
  for naptr in dnsAnswer:
    if naptr.service != b'ws20' or naptr.flags != b'U' or naptr.preference != preference or naptr.order != order:
      continue

    candidates.append( naptr.regexp.decode('utf-8') )

  if not candidates:
    print("sorry, no terrascenery URLs found. You may specify one with --url=http://some.url.org/foo")
    sys.exit(3)

  _url  = random.choice(candidates)
  _subst = _url.split(_url[0]) # split string, first character is separator <sep>regex<sep>replacement<sep>
  URL = re.sub(_subst[1], _subst[2], "" ) # apply regex substitude on empty string

print( "terrasyncing from ", URL, "to ", TARGET )
do_terrasync( URL, "", TARGET, None )

########################################################################
