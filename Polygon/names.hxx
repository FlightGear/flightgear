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
// (Log is kept at end of this file)
 

#ifndef _NAMES_HXX
#define _NAMES_HXX


// libgfc.a includes need this bit o' strangeness
#if defined ( linux )
#  define _LINUX_
#endif
#include <gfc/gadt_polygon.h>
#include <gfc/gdbf.h>
#undef E
#undef DEG_TO_RAD
#undef RAD_TO_DEG


// Posible shape file types
enum AreaType {
    AirportKeepArea = 0,
    AirportIgnoreArea = 1,
    MarshArea = 2,
    OceanArea = 3,
    LakeArea = 4,
    DryLakeArea = 5,
    IntLakeArea = 6,
    ReservoirArea = 7,
    IntReservoirArea = 8,
    StreamArea = 9,
    CanalArea = 10,
    GlacierArea = 11,
    VoidArea = 9997,
    NullArea = 9998,
    UnknownArea = 9999
};


// return the type of the shapefile record
AreaType get_area_type(GDBFile *dbf, int rec);

// return text form of area name
string get_area_name( AreaType area );


#endif // _NAMES_HXX


// $Log$
// Revision 1.1  1999/02/25 21:30:24  curt
// Initial revision.
//
// Revision 1.1  1999/02/23 01:29:05  curt
// Additional progress.
//
