// -*- coding: utf-8 -*-
//
// exceptions.hxx --- Exception classes for the FlightGear add-on infrastructure
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

#ifndef FG_ADDON_EXCEPTIONS_HXX
#define FG_ADDON_EXCEPTIONS_HXX

#include <string>

#include <simgear/structure/exception.hxx>

namespace flightgear
{

namespace addons
{

namespace errors
{

class error : public sg_exception
{
public:
  explicit error(const std::string& message,
                 const std::string& origin = std::string());
  explicit error(const char* message, const char* origin = nullptr);
};

class error_loading_config_file : public error
{ using error::error; /* inherit all constructors */ };

class no_metadata_file_found : public error
{ using error::error; };

class error_loading_metadata_file : public error
{ using error::error; };

class error_loading_menubar_items_file : public error
{ using error::error; };

class duplicate_registration_attempt : public error
{ using error::error; };

class fg_version_too_old : public error
{ using error::error; };

class fg_version_too_recent : public error
{ using error::error; };

class invalid_resource_path : public error
{ using error::error; };

class unable_to_create_addon_storage_dir : public error
{ using error::error; };

} // of namespace errors

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_EXCEPTIONS_HXX
