# -*- coding: utf-8 -*-

# dirindex.py --- Class used to parse .dirindex files
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

"""Parser for .dirindex files."""

from .exceptions import InvalidDirIndexFile
from .virtual_path import VirtualPath


class DirIndex:
    """Parser for .dirindex files."""

    def __init__(self, dirIndexFile):
        self.directories = []
        self.files = []
        self.tarballs = []
        self.version = 0
        self.path = None        # will be a VirtualPath instance when set

        # readFrom() stores the raw contents of the .dirindex file in this
        # attribute. This is useful for troubleshooting.
        self._rawContents = None

        with open(dirIndexFile, "r", encoding="ascii") as f:
            self.readFrom(f)

        self._sanityCheck()

    @classmethod
    def checkForBackslashOrLeadingSlash(cls, line, path):
        if '\\' in path or path.startswith('/'):
            raise InvalidDirIndexFile(
                r"invalid '\' or leading '/' in path field from line {!r}"
                .format(line))

    @classmethod
    def checkForSlashBackslashOrDoubleColon(cls, line, name):
        if '/' in name or '\\' in name:
            raise InvalidDirIndexFile(
                r"invalid '\' or '/' in name field from line {!r}"
                .format(line))

        if name == "..":
            raise InvalidDirIndexFile(
                r"invalid name field equal to '..' in line {!r}".format(line))

    def readFrom(self, readable):
        self._rawContents = readable.read()

        for line in self._rawContents.split('\n'):
            line = line.strip()
            if line.startswith('#'):
                continue

            tokens = line.split(':')
            if len(tokens) == 0:
                continue
            elif tokens[0] == "version":
                self.version = int(tokens[1])
            elif tokens[0] == "path":
                self.checkForBackslashOrLeadingSlash(line, tokens[1])
                # This is relative to the repository root
                self.path = VirtualPath(tokens[1])

                if ".." in self.path.parts:
                    raise InvalidDirIndexFile(
                        "'..' component found in 'path' entry {!r}"
                        .format(self.path))
            elif tokens[0] == "d":
                self.checkForSlashBackslashOrDoubleColon(line, tokens[1])
                self.directories.append({'name': tokens[1], 'hash': tokens[2]})
            elif tokens[0] == "f":
                self.checkForSlashBackslashOrDoubleColon(line, tokens[1])
                self.files.append({'name': tokens[1],
                                   'hash': tokens[2], 'size': int(tokens[3])})
            elif tokens[0] == "t":
                self.checkForSlashBackslashOrDoubleColon(line, tokens[1])
                self.tarballs.append({'name': tokens[1], 'hash': tokens[2],
                                      'size': int(tokens[3])})

    def _sanityCheck(self):
        if self.path is None:
            assert self._rawContents is not None

            firstLines = self._rawContents.split('\n')[:5]
            raise InvalidDirIndexFile(
                "no 'path' field found; the first lines of this .dirindex file "
                "follow:\n\n" + '\n'.join(firstLines))
