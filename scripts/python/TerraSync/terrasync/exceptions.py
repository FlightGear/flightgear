# -*- coding: utf-8 -*-

# exceptions.py --- Custom exception classes for terrasync.py
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

# Generic exception class for terrasync.py, to be subclassed for each specific
# kind of exception.
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

class UnsupportedURLScheme(TerraSyncPyException):
     """Exception raised when asked to handle an unsupported URL scheme."""
     ExceptionShortDescription = "Unsupported URL scheme"

class RepoDataError(TerraSyncPyException):
     """
     Exception raised when getting invalid data from the TerraSync repository."""
     ExceptionShortDescription = "Invalid data from the TerraSync repository"

class InvalidDirIndexFile(RepoDataError):
     """Exception raised when getting invalid data from a .dirindex file."""
     ExceptionShortDescription = "Invalid .dirindex file"
