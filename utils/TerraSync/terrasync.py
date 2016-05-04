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
#
# this is a terrible stub, please improve

import os 
import hashlib
import urllib.request

dirindex = ".dirindex"
DIRINDEXVERSION = 1

#TODO: use DNS to resolve list of mirrors
# - lookup terrasync.flightgear.org, type=NAPTR, service="ws20", flags="U"
# - sort by order,preference ascending
# - pick entries with lowest order and preference
# - randomly pick one of those
# - use regexp fields URL
URL="http://flightgear.sourceforge.net/scenery"

########################################################################

def fn_hash_of_file(fname):
    hash = hashlib.sha1()
    try:
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash.update(chunk)
    except:
        pass

    return hash.hexdigest()

########################################################################
def do_download_file( _url, _path, _localfile, _hash ):
  if os.path.exists( _localfile ):
    h = fn_hash_of_file(_localfile)
    if h == _hash:
      print("hash match for ", _localfile)
      return

  r = urllib.request.urlopen( _url + _path )
  f = open(_localfile, 'wb')
  f.write( r.read() )
  f.close()
  print("downloaded ", _localfile, " from ", _url + _path )

########################################################################
def do_terrasync( _url, _path, _localdir ):
  url = _url + _path
  print("syncing ",url)

  if not os.path.exists( _localdir ):
    os.makedirs( _localdir )

  for line in urllib.request.urlopen(url + "/.dirindex").readlines():
    tokens = line.decode("utf-8").rstrip().split(':')
    if( len(tokens) == 0 ):
      continue

    # TODO: check version number, should be equal to DIRINDEXVERSION
    #       otherwise complain and terminate
    if( tokens[0] == "version" ):
      continue

    if( tokens[0] == "path" ):
      continue

    if( tokens[0] == "d" ):
      do_terrasync( url,  "/" + tokens[1], os.path.join(_localdir,tokens[1] ) )

    if( tokens[0] == "f" ):
      do_download_file( url, "/" + tokens[1], os.path.join(_localdir,tokens[1]), tokens[2] )

########################################################################

# TODO: parse command line args
# --url=automatic for automatic detection of server URL
# --url=http://flightgear.sourceforge.net/scenery to use explicit server
# --path=/Models sync only the /Models path from the server
# --destination=/some/path write files to destination instead of pwd
# TODO: sanitize user provided data

do_terrasync( URL, "", "." )

########################################################################
