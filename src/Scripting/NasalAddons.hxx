// -*- coding: utf-8 -*-
//
// NasalAddons.hxx --- Expose add-on classes to Nasal
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

#ifndef FG_ADDON_NASAL_INTERFACE_HXX
#define FG_ADDON_NASAL_INTERFACE_HXX

#include <simgear/nasal/nasal.h>

namespace flightgear
{

namespace addons
{

void initAddonClassesForNasal(naRef globals, naContext c);

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_NASAL_INTERFACE_HXX
