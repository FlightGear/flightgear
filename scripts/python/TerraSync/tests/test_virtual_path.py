#! /usr/bin/env python3
# -*- coding: utf-8 -*-

# test_virtual_path.py --- Test module for terrasync.virtual_path
# Copyright (C) 2018  Florent Rougon
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

import collections
import unittest

from terrasync.virtual_path import VirtualPath, MutableVirtualPath

# Hook doctest-based tests into the unittest test discovery mechanism
import doctest
import terrasync.virtual_path

def load_tests(loader, tests, ignore):
    # Tell unittest to run doctests from terrasync.virtual_path
    tests.addTests(doctest.DocTestSuite(terrasync.virtual_path))
    return tests


class VirtualPathCommonTests:
    """Common tests to run for both VirtualPath and MutableVirtualPath.

    The tests inside this class must exercice the class (VirtualPath or
    MutableVirtualPath) stored in the 'cls' class attribute. They must
    work for both VirtualPath and MutableVirtualPath, otherwise they
    don't belong here!

    """

    def test_normalizeStringPath(self):
        self.assertEqual(self.cls.normalizeStringPath("/"), "/")
        self.assertEqual(self.cls.normalizeStringPath(""), "/")
        self.assertEqual(
            self.cls.normalizeStringPath("/abc/Def ijk//l Mn///op/q/rst/"),
            "/abc/Def ijk/l Mn/op/q/rst")
        self.assertEqual(self.cls.normalizeStringPath("abc/def"), "/abc/def")
        self.assertEqual(self.cls.normalizeStringPath("/abc/def"), "/abc/def")
        self.assertEqual(self.cls.normalizeStringPath("//abc/def"),
                         "/abc/def")
        self.assertEqual(self.cls.normalizeStringPath("///abc/def"),
                         "/abc/def")
        self.assertEqual(self.cls.normalizeStringPath("/abc//def"),
                         "/abc/def")

    # Unless the implementation of VirtualPath.__init__() has changed
    # meanwhile, the following function must be essentially the same as
    # test_normalizeStringPath().
    def test_constructor_and_str(self):
        p = self.cls("/")
        self.assertEqual(str(p), "/")

        p = self.cls("")
        self.assertEqual(str(p), "/")

        p = self.cls("/abc/Def ijk//l Mn///op/q/rst/")
        self.assertEqual(str(p), "/abc/Def ijk/l Mn/op/q/rst")

        p = self.cls("abc/def")
        self.assertEqual(str(p), "/abc/def")

        p = self.cls("/abc/def")
        self.assertEqual(str(p), "/abc/def")

        p = self.cls("//abc/def")
        self.assertEqual(str(p), "/abc/def")

        p = self.cls("///abc/def")
        self.assertEqual(str(p), "/abc/def")

        p = self.cls("/abc//def")
        self.assertEqual(str(p), "/abc/def")

    def test_asPosix (self):
        self.assertEqual(self.cls("").asPosix(), "/")
        self.assertEqual(self.cls("/").asPosix(), "/")
        self.assertEqual(self.cls("/abc//def").asPosix(), "/abc/def")
        self.assertEqual(self.cls("/abc//def/").asPosix(), "/abc/def")
        self.assertEqual(self.cls("//abc//def//").asPosix(), "/abc/def")
        self.assertEqual(self.cls("////abc//def//").asPosix(), "/abc/def")

    def test_samePath(self):
        self.assertTrue(self.cls("").samePath(self.cls("")))
        self.assertTrue(self.cls("").samePath(self.cls("/")))
        self.assertTrue(self.cls("/").samePath(self.cls("")))
        self.assertTrue(self.cls("/").samePath(self.cls("/")))

        self.assertTrue(
            self.cls("/abc/def").samePath(self.cls("/abc/def")))
        self.assertTrue(
            self.cls("/abc//def").samePath(self.cls("/abc/def")))
        self.assertTrue(
            self.cls("/abc/def/").samePath(self.cls("/abc/def")))

    def test_comparisons(self):
        self.assertEqual(self.cls("/abc/def"), self.cls("/abc/def"))
        self.assertEqual(self.cls("/abc//def"), self.cls("/abc/def"))
        self.assertEqual(self.cls("/abc/def/"), self.cls("/abc/def"))

        self.assertNotEqual(self.cls("/abc/dEf"), self.cls("/abc/def"))
        self.assertNotEqual(self.cls("/abc/def "), self.cls("/abc/def"))

        self.assertLessEqual(self.cls("/foo/bar"), self.cls("/foo/bar"))
        self.assertLessEqual(self.cls("/foo/bar"), self.cls("/foo/bbr"))
        self.assertLess(self.cls("/foo/bar"), self.cls("/foo/bbr"))

        self.assertGreaterEqual(self.cls("/foo/bar"), self.cls("/foo/bar"))
        self.assertGreaterEqual(self.cls("/foo/bbr"), self.cls("/foo/bar"))
        self.assertGreater(self.cls("/foo/bbr"), self.cls("/foo/bar"))

    def test_truedivOperators(self):
        """
        Test operators used to add paths components to a VirtualPath instance."""
        p = self.cls("/foo/bar/baz/quux/zoot")
        self.assertEqual(p, self.cls("/") / "foo" / "bar" / "baz/quux/zoot")
        self.assertEqual(p, self.cls("/foo") / "bar" / "baz/quux/zoot")
        self.assertEqual(p, self.cls("/foo/bar") / "baz/quux/zoot")

    def test_joinpath(self):
        p = self.cls("/foo/bar/baz/quux/zoot")
        self.assertEqual(
            p,
            self.cls("/foo").joinpath("bar", "baz", "quux/zoot"))

    def test_nameAttribute(self):
        self.assertEqual(self.cls("/").name, "")

        p = self.cls("/foo/bar/baz/quux/zoot")
        self.assertEqual(p.name, "zoot")

    def test_partsAttribute(self):
        self.assertEqual(self.cls("/").parts, ("/",))

        p = self.cls("/foo/bar/baz/quux/zoot")
        self.assertEqual(p.parts, ("/", "foo", "bar", "baz", "quux", "zoot"))

    def test_parentsAttribute(self):
        def pathify(*args):
            return tuple( (self.cls(s) for s in args) )

        p = self.cls("/")
        self.assertEqual(tuple(p.parents), pathify()) # empty tuple

        p = self.cls("/foo")
        self.assertEqual(tuple(p.parents), pathify("/"))

        p = self.cls("/foo/bar")
        self.assertEqual(tuple(p.parents), pathify("/foo", "/"))

        p = self.cls("/foo/bar/baz")
        self.assertEqual(tuple(p.parents), pathify("/foo/bar", "/foo", "/"))

    def test_parentAttribute(self):
        def pathify(s):
            return self.cls(s)

        p = self.cls("/")
        self.assertEqual(p.parent, pathify("/"))

        p = self.cls("/foo")
        self.assertEqual(p.parent, pathify("/"))

        p = self.cls("/foo/bar")
        self.assertEqual(p.parent, pathify("/foo"))

        p = self.cls("/foo/bar/baz")
        self.assertEqual(p.parent, pathify("/foo/bar"))

    def test_suffixAttribute(self):
        p = self.cls("/")
        self.assertEqual(p.suffix, '')

        p = self.cls("/foo/bar/baz.py")
        self.assertEqual(p.suffix, '.py')

        p = self.cls("/foo/bar/baz.py.bla")
        self.assertEqual(p.suffix, '.bla')

        p = self.cls("/foo/bar/baz")
        self.assertEqual(p.suffix, '')

    def test_suffixesAttribute(self):
        p = self.cls("/")
        self.assertEqual(p.suffixes, [])

        p = self.cls("/foo/bar/baz.py")
        self.assertEqual(p.suffixes, ['.py'])

        p = self.cls("/foo/bar/baz.py.bla")
        self.assertEqual(p.suffixes, ['.py', '.bla'])

        p = self.cls("/foo/bar/baz")
        self.assertEqual(p.suffixes, [])

    def test_stemAttribute(self):
        p = self.cls("/")
        self.assertEqual(p.stem, '')

        p = self.cls("/foo/bar/baz.py")
        self.assertEqual(p.stem, 'baz')

        p = self.cls("/foo/bar/baz.py.bla")
        self.assertEqual(p.stem, 'baz.py')

    def test_asRelative(self):
        self.assertEqual(self.cls("/").asRelative(), "")
        self.assertEqual(self.cls("/foo/bar/baz/quux/zoot").asRelative(),
                         "foo/bar/baz/quux/zoot")

    def test_relativeTo(self):
        self.assertEqual(self.cls("").relativeTo(""), "")
        self.assertEqual(self.cls("").relativeTo("/"), "")
        self.assertEqual(self.cls("/").relativeTo("/"), "")
        self.assertEqual(self.cls("/").relativeTo(""), "")

        p = self.cls("/foo/bar/baz/quux/zoot")

        self.assertEqual(p.relativeTo(""), "foo/bar/baz/quux/zoot")
        self.assertEqual(p.relativeTo("/"), "foo/bar/baz/quux/zoot")

        self.assertEqual(p.relativeTo("foo"), "bar/baz/quux/zoot")
        self.assertEqual(p.relativeTo("foo/"), "bar/baz/quux/zoot")
        self.assertEqual(p.relativeTo("/foo"), "bar/baz/quux/zoot")
        self.assertEqual(p.relativeTo("/foo/"), "bar/baz/quux/zoot")

        self.assertEqual(p.relativeTo("foo/bar/baz"), "quux/zoot")
        self.assertEqual(p.relativeTo("foo/bar/baz/"), "quux/zoot")
        self.assertEqual(p.relativeTo("/foo/bar/baz"), "quux/zoot")
        self.assertEqual(p.relativeTo("/foo/bar/baz/"), "quux/zoot")

        with self.assertRaises(ValueError):
            p.relativeTo("/foo/ba")

        with self.assertRaises(ValueError):
            p.relativeTo("/foo/balloon")

    def test_withName(self):
        p = self.cls("/foo/bar/baz/quux/zoot")

        self.assertEqual(p.withName(""),
                         VirtualPath("/foo/bar/baz/quux"))
        self.assertEqual(p.withName("pouet"),
                         VirtualPath("/foo/bar/baz/quux/pouet"))
        self.assertEqual(p.withName("pouet/zdong"),
                         VirtualPath("/foo/bar/baz/quux/pouet/zdong"))

        # The self.cls object has no 'name' (referring to the 'name' property)
        with self.assertRaises(ValueError):
            self.cls("").withName("foobar")

        with self.assertRaises(ValueError):
            self.cls("/").withName("foobar")

    def test_withSuffix(self):
        p = self.cls("/foo/bar/baz.tar.gz")
        self.assertEqual(p.withSuffix(".bz2"),
                         VirtualPath("/foo/bar/baz.tar.bz2"))
        p = self.cls("/foo/bar/baz")
        self.assertEqual(p.withSuffix(".tar.xz"),
                         VirtualPath("/foo/bar/baz.tar.xz"))

        # The self.cls object has no 'name' (referring to the 'name' property)
        with self.assertRaises(ValueError):
            self.cls("/foo/bar/baz.tar.gz").withSuffix("no-leading-dot")

        with self.assertRaises(ValueError):
            # The root virtual path ('/') can't be used for this
            self.cls("/").withSuffix(".foobar")


class TestVirtualPath(unittest.TestCase, VirtualPathCommonTests):
    """Tests for the VirtualPath class.

    These are the tests using the common infrastructure from
    VirtualPathCommonTests.

    """

    cls = VirtualPath

class TestVirtualPathSpecific(unittest.TestCase):
    """Tests specific to the VirtualPath class."""

    def test_isHashableType(self):
        p = VirtualPath("/foo")
        self.assertTrue(isinstance(p, collections.abc.Hashable))

    def test_insideSet(self):
        l1 = [ VirtualPath("/foo/bar"),
               VirtualPath("/foo/baz") ]
        l2 = l1 + [ VirtualPath("/foo/bar") ] # l2 has a duplicate element

        # Sets allow one to ignore duplicate elements when comparing
        self.assertEqual(set(l1), set(l2))
        self.assertEqual(frozenset(l1), frozenset(l2))


class TestMutableVirtualPath(unittest.TestCase, VirtualPathCommonTests):
    """Tests for the MutableVirtualPath class.

    These are the tests using the common infrastructure from
    VirtualPathCommonTests.

    """

    cls = MutableVirtualPath

class TestMutableVirtualPathSpecific(unittest.TestCase):
    """Tests specific to the MutableVirtualPath class."""

    def test_mixedComparisons(self):
        self.assertTrue(
            VirtualPath("/abc/def").samePath(MutableVirtualPath("/abc/def")))
        self.assertTrue(
            VirtualPath("/abc//def").samePath(MutableVirtualPath("/abc/def")))
        self.assertTrue(
            VirtualPath("/abc/def/").samePath(MutableVirtualPath("/abc/def")))

        self.assertTrue(
            MutableVirtualPath("/abc/def").samePath(VirtualPath("/abc/def")))
        self.assertTrue(
            MutableVirtualPath("/abc//def").samePath(VirtualPath("/abc/def")))
        self.assertTrue(
            MutableVirtualPath("/abc/def/").samePath(VirtualPath("/abc/def")))

    def test_inPlacePathConcatenation(self):
        p = VirtualPath("/foo/bar/baz/quux/zoot")

        q = MutableVirtualPath("/foo")
        q /= "bar"
        q /= "baz/quux/zoot"

        self.assertTrue(p.samePath(q))

    def test_isNotHashableType(self):
        p = MutableVirtualPath("/foo")
        self.assertFalse(isinstance(p, collections.abc.Hashable))
