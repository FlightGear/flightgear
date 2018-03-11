// -*- coding: utf-8 -*-
//
// addon_fwd.hxx --- Forward declarations for the FlightGear add-on
//                   infrastructure
// Copyright (C) 2017 Florent Rougon
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

#ifndef FG_ADDON_FWD_HXX
#define FG_ADDON_FWD_HXX

#include <simgear/structure/SGSharedPtr.hxx>

namespace flightgear
{

namespace addons
{

class Addon;
class AddonManager;
class AddonVersion;
class AddonVersionSuffix;
class ResourceProvider;

enum class UrlType;
class QualifiedUrl;

enum class ContactType;
class Contact;
class Author;
class Maintainer;

using AddonRef = SGSharedPtr<Addon>;
using AddonVersionRef = SGSharedPtr<AddonVersion>;
using ContactRef = SGSharedPtr<Contact>;
using AuthorRef = SGSharedPtr<Author>;
using MaintainerRef = SGSharedPtr<Maintainer>;

namespace errors
{

class error;
class error_loading_config_file;
class no_metadata_file_found;
class error_loading_metadata_file;
class error_loading_menubar_items_file;
class duplicate_registration_attempt;
class fg_version_too_old;
class fg_version_too_recent;
class invalid_resource_path;
class unable_to_create_addon_storage_dir;

} // of namespace errors

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_FWD_HXX
