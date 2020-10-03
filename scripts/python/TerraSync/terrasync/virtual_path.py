# -*- coding: utf-8 -*-

# virtual_path.py --- Classes used to manipulate slash-separated virtual paths
#
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

"""Module containing the VirtualPath and MutableVirtualPath classes."""

import pathlib


class VirtualPath:
    """Class used to represent virtual paths using the slash separator.

    This class always uses the slash ('/') as the separator between
    components. For terrasync.py, the root path '/' corresponds to the
    repository root, regardless of where it is stored (hard drive,
    remote server, etc.).

    Note: because of this, the class is not supposed to be used directly
          for filesystem accesses, since some root directory or
          protocol://server/root-dir prefix would have to be prepended
          to provide reasonably useful functionality. This is why the
          paths managed by this class are said to be virtual. This also
          implies that even in Python 3.6 or later, this class should
          *not* inherit from os.PathLike.

    Whenever a given feature exists in pathlib.PurePath, this class
    replicates the corresponding pathlib.PurePath API, but using
    mixedCaseStyle instead of underscore_style (the latter being used
    for every method of pathlib.PurePath). Of course, types are adapted:
    for instance, methods of this class often return a VirtualPath
    instance, whereas the corresponding pathlib.PurePath methods would
    return a pathlib.PurePath instance.

    """
    def __init__(self, p):
        # Once this function exits, self._path *must not be changed* anymore
        # (doing so would violate the contract for a hashable object: the
        # hash must not change once the object has been constructed).
        self._path = self.normalizeStringPath(p)
        # This check could of course be skipped if it is found to really affect
        # performance.
        self._check()

    def __str__(self):
        """Return a string representation of the path in self.

        The return value:
          - always starts with a '/';
          - never ends with a '/' except if it is exactly '/' (i.e.,
            the root virtual path).

        """
        return self._path

    def asPosix(self):
        """Return a string representation of the path in self.

        This method returns str(self), it is only present for
        compatibility with pathlib.PurePath.

        """
        return str(self)

    def __repr__(self):
        return "{}.{}({!r})".format(__name__, type(self).__name__, self._path)

    def __lt__(self, other):
        # Allow sorting with instances of VirtualPath, or of any subclass. Note
        # that the == operator (__eq__()) and therefore also != are stricter
        # with respect to typing.
        if isinstance(other, VirtualPath):
            return self._path < other._path
        else:
            return NotImplemented

    def __le__(self, other):
        if isinstance(other, VirtualPath):
            return self._path <= other._path
        else:
            return NotImplemented

    def __eq__(self, other):
        # The types must be the same, therefore a VirtualPath never compares
        # equal to a MutableVirtualPath with the == operator. For such
        # comparisons, use the samePath() method. If __eq__() (and thus
        # necessarily __hash__()) were more lax about typing, adding
        # VirtualPath instances and instances of hashable subclasses of
        # VirtualPath with the same _path to a set or frozenset would lead to
        # unintuitive behavior, since they would all be considered equal.
        return type(self) == type(other) and self._path == other._path

    # intentionally not implemented. Python3 provides a default implementation.
    # def __ne__(self, other):

    def __gt__(self, other):
        if isinstance(other, VirtualPath):
            return self._path > other._path
        else:
            return NotImplemented

    def __ge__(self, other):
        if isinstance(other, VirtualPath):
            return self._path >= other._path
        else:
            return NotImplemented

    def __hash__(self):
        # Be strict about typing, as for __eq__().
        return hash((type(self), self._path))

    def samePath(self, other):
        """Compare the path with another instance, possibly of a subclass.

        other -- instance of VirtualPath, or of a subclass of
                 VirtualPath

        """
        if isinstance(other, VirtualPath):
            return self._path == other._path
        else:
            raise TypeError("{obj!r} is of type {klass}, which is neither "
                            "VirtualPath nor a subclass thereof"
                            .format(obj=other, klass=type(other).__name__))

    def _check(self):
        """Run consistency checks on self."""
        assert (self._path.startswith('/') and not self._path.startswith('//')
                and (self._path == '/' or not self._path.endswith('/'))), \
                repr(self._path)

    @classmethod
    def normalizeStringPath(cls, path):
        """Normalize a string representing a virtual path.

        path -- input path (string)

        Return a string that always starts with a slash, never contains
        consecutive slashes and only ends with a slash if it's the root
        virtual path ('/').

        If 'path' doesn't start with a slash ('/'), it is considered
        relative to the root. This implies that if 'path' is the empty
        string, the return value is '/'.

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

    def __truediv__(self, s):
        """Path concatenation with the '/' operator.

        's' must be a string representing a relative path using the '/'
        separator, for instance "dir/subdir/other-subdir".

        Return a new instance of type(self).

        """
        assert not (s.startswith('/') or s.endswith('/')), repr(s)

        if self._path == '/':
            return type(self)(self._path + s)
        else:
            return type(self)(self._path + '/' + s)

    def joinpath(self, *args):
        """Combine 'self' with each given string argument in turn.

        Each argument should be of the form "foo", "foo/bar",
        "foo/bar/baz", etc. Return the corresponding instance of
        type(self).

        >>> p = VirtualPath("/foo").joinpath("bar", "baz", "quux/zoot")
        >>> str(p)
        '/foo/bar/baz/quux/zoot'

        """
        return self / '/'.join(args)

    @property
    def name(self):
        """Return a string representing the final path component.

        >>> p = VirtualPath("/foo/bar/baz")
        >>> p.name
        'baz'

        """
        pos = self._path.rfind('/')
        assert pos != -1, (pos, self._path)

        return self._path[pos+1:]

    @property
    def parts(self):
        """Return a tuple containing the path’s components.

        >>> p = VirtualPath('/usr/bin/python3')
        >>> p.parts
        ('/', 'usr', 'bin', 'python3')

        """
        if self._path == "/":
            return ('/',)
        else:
            # Skip the leading slash before splitting
            return ('/',) + tuple(self._path[1:].split('/'))

    def generateParents(self):
        """Generator function for the parents of the path.

        See the 'parents' property for details.

        """
        if self._path == '/':
            return

        assert self._path.startswith('/'), repr(self._path)
        prevPos = len(self._path)

        while True:
            pos = self._path.rfind('/', 0, prevPos)

            if pos > 0:
                yield type(self)(self._path[:pos])
                prevPos = pos
            else:
                assert pos == 0, pos
                break

        yield type(self)('/')

    @property
    def parents(self):
        """The path ancestors.

        Return an immutable sequence providing access to the logical
        ancestors of the path.

        >>> p = VirtualPath('/foo/bar/baz')
        >>> len(p.parents)
        3
        >>> p.parents[0]
        terrasync.virtual_path.VirtualPath('/foo/bar')
        >>> p.parents[1]
        terrasync.virtual_path.VirtualPath('/foo')
        >>> p.parents[2]
        terrasync.virtual_path.VirtualPath('/')

        """
        return tuple(self.generateParents())

    @property
    def parent(self):
        """The logical parent of the path.

        >>> p = VirtualPath('/foo/bar/baz')
        >>> p.parent
        terrasync.virtual_path.VirtualPath('/foo/bar')
        >>> q = VirtualPath('/')
        >>> q.parent
        terrasync.virtual_path.VirtualPath('/')

        """
        pos = self._path.rfind('/')
        assert pos >= 0, pos

        if pos == 0:
            return type(self)('/')
        else:
            return type(self)(self._path[:pos])

    @property
    def suffix(self):
        """The extension of the final component, if any.

        >>> VirtualPath('/my/library/setup.py').suffix
        '.py'
        >>> VirtualPath('/my/library.tar.gz').suffix
        '.gz'
        >>> VirtualPath('/my/library').suffix
        ''

        """
        name = self.name
        pos = name.rfind('.')
        return name[pos:] if pos != -1 else ''

    @property
    def suffixes(self):
        """A list of the path’s extensions.

        >>> VirtualPath('/my/library/setup.py').suffixes
        ['.py']
        >>> VirtualPath('/my/library.tar.gz').suffixes
        ['.tar', '.gz']
        >>> VirtualPath('/my/library').suffixes
        []

        """
        name = self.name
        prevPos = len(name)
        l = []

        while True:
            pos = name.rfind('.', 0, prevPos)
            if pos == -1:
                break
            else:
                l.insert(0, name[pos:prevPos])
                prevPos = pos

        return l

    @property
    def stem(self):
        """The final path component, without its suffix.

        >>> VirtualPath('/my/library.tar.gz').stem
        'library.tar'
        >>> VirtualPath('/my/library.tar').stem
        'library'
        >>> VirtualPath('/my/library').stem
        'library'
        >>> VirtualPath('/').stem
        ''

        """
        name = self.name
        pos = name.rfind('.')

        return name if pos == -1 else name[:pos]

    def asRelative(self):
        """Return the virtual path without its leading '/'.

        >>> p = VirtualPath('/usr/bin/python3')
        >>> p.asRelative()
        'usr/bin/python3'

        >>> VirtualPath('').asRelative()
        ''
        >>> VirtualPath('/').asRelative()
        ''

        """
        assert self._path.startswith('/'), repr(self._path)
        return self._path[1:]

    def relativeTo(self, other):
        """Return the portion of this path that follows 'other'.

        The return value is a string. If the operation is impossible,
        ValueError is raised.

        >>> VirtualPath('/etc/passwd').relativeTo('/')
        'etc/passwd'
        >>> VirtualPath('/etc/passwd').relativeTo('/etc')
        'passwd'

        """
        normedOther = self.normalizeStringPath(other)

        if normedOther == '/':
            return self._path[1:]
        elif self._path.startswith(normedOther):
            rest = self._path[len(normedOther):]

            if rest.startswith('/'):
                return rest[1:]

        raise ValueError("{!r} does not start with '{}'".format(self, other))

    def withName(self, newName):
        """Return a new VirtualPath instance with the 'name' part changed.

        If the original path is '/' (which doesn’t have a name in the
        sense of the 'name' property), ValueError is raised.

        >>> p = VirtualPath('/foobar/downloads/pathlib.tar.gz')
        >>> p.withName('setup.py')
        terrasync.virtual_path.VirtualPath('/foobar/downloads/setup.py')

        """
        if self._path == '/':
            raise ValueError("{!r} has an empty name".format(self))
        else:
            pos = self._path.rfind('/')
            assert pos != -1, (pos, self._path)

            if newName.startswith('/'):
                raise ValueError("{!r} starts with a '/'".format(newName))
            elif newName.endswith('/'):
                raise ValueError("{!r} ends with a '/'".format(newName))
            else:
                return VirtualPath(self._path[:pos]) / newName


    def withSuffix(self, newSuffix):
        """Return a new VirtualPath instance with the suffix changed.

        If the original path doesn’t have a suffix, the new suffix is
        appended:

        >>> p = VirtualPath('/foobar/downloads/pathlib.tar.gz')
        >>> p.withSuffix('.bz2')
        terrasync.virtual_path.VirtualPath('/foobar/downloads/pathlib.tar.bz2')
        >>> p = VirtualPath('/foobar/README')
        >>> p.withSuffix('.txt')
        terrasync.virtual_path.VirtualPath('/foobar/README.txt')

        If 'self' is the root virtual path ('/') or 'newSuffix' doesn't
        start with '.', ValueError is raised.

        """
        if not newSuffix.startswith('.'):
            raise ValueError("new suffix {!r} doesn't start with '.'"
                             .format(newSuffix))

        name = self.name
        if not name:
            raise ValueError("{!r} has an empty 'name' part".format(self))

        pos = name.rfind('.')

        if pos == -1:
            return self.withName(name + newSuffix)       # append suffix
        else:
            return self.withName(name[:pos] + newSuffix) # replace suffix


class MutableVirtualPath(VirtualPath):

    """Mutable subclass of VirtualPath.

    Contrary to VirtualPath objects, instances of this class can be
    modified in-place with the /= operator, in order to append path
    components. The price to pay for this advantage is that they can't
    be used as dictionary keys or as elements of a set or frozenset,
    because they are not hashable.

    """

    __hash__ = None             # ensure the type is not hashable

    def _normalize(self):
        self._path = self.normalizeStringPath(self._path)

    def __itruediv__(self, s):
        """Path concatenation with the '/=' operator.

        's' must be a string representing a relative path using the '/'
        separator, for instance "dir/subdir/other-subdir".

        """
        # This check could of course be skipped if it is found to really affect
        # performance.
        self._check()
        assert not (s.startswith('/') or s.endswith('/')), repr(s)

        if self._path == '/':
            self._path += s
        else:
            self._path += '/' + s

        # Collapse multiple slashes, remove trailing '/' except if the whole
        # path is '/', etc.
        self._normalize()

        return self


if __name__ == "__main__":
    # The doctest setup below works, but for full test coverage, use the
    # unittest framework (it is set up to automatically run all doctests from
    # this module!).
    #
    # Hint: 'python3 -m unittest discover' from the TerraSync directory
    #       should do the trick.
    import doctest
    doctest.testmod()
