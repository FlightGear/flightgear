// -*- coding: utf-8 -*-
//
// exceptions.cxx --- Exception classes for the FlightGear add-on infrastructure
// Copyright (C) 2017  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <string>

#include <simgear/structure/exception.hxx>

#include "exceptions.hxx"

using std::string;

namespace flightgear
{

namespace addons
{

namespace errors
{

// ***************************************************************************
// *                    Base class for add-on exceptions                     *
// ***************************************************************************

// Prepending a prefix such as "Add-on error: " would be redundant given the
// messages used in, e.g., the Addon class code.
error::error(const string& message, const string& origin)
  : sg_exception(message, origin)
{ }

error::error(const char* message, const char* origin)
  : error(string(message), string(origin))
{ }

} // of namespace errors

} // of namespace addons

} // of namespace flightgear
