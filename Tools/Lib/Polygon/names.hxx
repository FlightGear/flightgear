// names.hxx -- process shapefiles names
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
 

#ifndef _NAMES_HXX
#define _NAMES_HXX


#include <Include/compiler.h>

#include STL_STRING

FG_USING_STD(string);


// Posible shape file types.  Note the order of these is important and
// defines the priority of these shapes if they should intersect.  The
// smaller the number, the higher the priority.
enum AreaType {
    DefaultArea       = 0,
    AirportKeepArea   = 1,
    AirportIgnoreArea = 2,
    OceanArea         = 3,
    LakeArea          = 4,
    DryLakeArea       = 5,
    IntLakeArea       = 6,
    ReservoirArea     = 7,
    IntReservoirArea  = 8,
    StreamArea        = 9,
    CanalArea         = 10,
    GlacierArea       = 11,
    MarshArea         = 12,
    VoidArea          = 9997,
    NullArea          = 9998,
    UnknownArea       = 9999
};


// return area type from text name
AreaType get_area_type( string area );

// return text form of area name
string get_area_name( AreaType area );


#endif // _NAMES_HXX


