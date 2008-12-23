// fixlist.hxx -- fix list management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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
//
// $Id$


#ifndef _FG_FIXLIST_HXX
#define _FG_FIXLIST_HXX


#include <simgear/compiler.h>

class FGFix;
class SGPath;

class FGFixList {
public:

    FGFixList();
    ~FGFixList();

    // load the navaids and build the map
    bool init(const SGPath& path);
};


#endif // _FG_FIXLIST_HXX
