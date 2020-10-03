#! /usr/bin/env python3
# -*- coding: utf-8 -*-

# test_dirindex.py --- Test module for terrasync.dirindex
# Copyright (C) 2020  Florent Rougon
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

# In order to exercise all tests, run the following command from the parent
# directory (you may omit the 'discover' argument):
#
#   python3 -m unittest discover

"""Test module for terrasync.dirindex"""

import os
import unittest
from terrasync.dirindex import DirIndex
from terrasync.exceptions import InvalidDirIndexFile
from terrasync.virtual_path import VirtualPath


baseDir = os.path.dirname(__file__)

def testData(*args):
    return os.path.join(baseDir, "data", "dirindex", *args)


directories_in_sample_dirindex_1 = [
    {'name': 'Airports', 'hash': '8a93b5d8a2b04d2fb8de4ef58ad02f9e8819d314'},
    {'name': 'Models', 'hash': 'bee221c9d2621dc9b69cd9e0ad7dd0605f6ea928'},
    {'name': 'Objects', 'hash': '10ae32c986470fa55b56b8eefbc6ed565cce0642'},
    {'name': 'Terrain', 'hash': 'e934024dc0f959f9a433e47c646d256630052c2e'},
    {'name': 'Buildings', 'hash': '19060725efc2a301fa6844991e2922d42d8de5e2'},
    {'name': 'Pylons', 'hash': '378b3dd58ce3058f2992b70aa5ecf8947a4d7f9e'},
    {'name': 'Roads', 'hash': '89f8f10406041948368c76c0a2e794d45ac536b7'}]

files_in_sample_dirindex_1 = [
    {'name': 'some file',
     'hash': '4cbf3d1746a1249bff7809e4b079dd80cfce594c',
     'size': 123},
    {'name': 'other file',
     'hash': '62726252f7183eef31001c1c565e149f3c4527b9',
     'size': 4567},
    {'name': 'third file',
     'hash': '303adcc1747d8dc438096307189881e987e9bb61',
     'size': 89012}]

tarballs_in_sample_dirindex_1 = [
    {'name': 'Airports_archive.tgz',
     'hash': 'b63a067d82824f158d6bde66f9e76654274277fe',
     'size': 1234567}]


class TestDirIndex(unittest.TestCase):
    """Unit tests for the DirIndex class."""

    def test_constructor(self):
        d = DirIndex(testData("good", "sample_dirindex_1"))
        self.assertEqual(d.version, 1)
        self.assertEqual(d.path, VirtualPath("some/path"))
        self.assertEqual(d.directories, directories_in_sample_dirindex_1)
        self.assertEqual(d.files, files_in_sample_dirindex_1)
        self.assertEqual(d.tarballs, tarballs_in_sample_dirindex_1)

        stems = ("path_starts_with_slash",
                 "path_contains_a_backslash",
                 "dotdot_in_path",
                 "slash_in_directory_name",
                 "slash_in_file_name",
                 "slash_in_tarball_name",
                 "backslash_in_directory_name",
                 "backslash_in_file_name",
                 "backslash_in_tarball_name",
                 "directory_name_is_double_colon",
                 "file_name_is_double_colon",
                 "tarball_name_is_double_colon",)
        for stem in stems:
            with self.assertRaises(InvalidDirIndexFile):
                DirIndex(testData("bad", "bad_dirindex_" + stem))

        with self.assertRaises(UnicodeDecodeError):
            d = DirIndex(testData("bad", "bad_dirindex_encoding"))
