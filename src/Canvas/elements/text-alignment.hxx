// Mapping between strings and osg text alignment flags
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef ENUM_MAPPING
# error "Only include with ENUM_MAPPING defined!"
#endif

ENUM_MAPPING(LEFT_TOP,      "left-top")
ENUM_MAPPING(LEFT_CENTER,   "left-center")
ENUM_MAPPING(LEFT_BOTTOM,   "left-bottom")

ENUM_MAPPING(CENTER_TOP,    "center-top")
ENUM_MAPPING(CENTER_CENTER, "center-center")
ENUM_MAPPING(CENTER_BOTTOM, "center-bottom")

ENUM_MAPPING(RIGHT_TOP,     "right-top")
ENUM_MAPPING(RIGHT_CENTER,  "right-center")
ENUM_MAPPING(RIGHT_BOTTOM,  "right-bottom")

ENUM_MAPPING(LEFT_BASE_LINE,    "left-baseline")
ENUM_MAPPING(CENTER_BASE_LINE,  "center-baseline")
ENUM_MAPPING(RIGHT_BASE_LINE,   "right-baseline")

ENUM_MAPPING(LEFT_BOTTOM_BASE_LINE,     "left-bottom-baseline")
ENUM_MAPPING(CENTER_BOTTOM_BASE_LINE,   "center-bottom-baseline")
ENUM_MAPPING(RIGHT_BOTTOM_BASE_LINE,    "right-bottom-baseline")
