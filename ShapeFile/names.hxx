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
    MarshArea = 0,
    OceanArea = 1,
    LakeArea = 2,
    DryLakeArea = 3,
    IntLakeArea = 4,
    ReservoirArea = 5,
    IntReservoirArea = 6,
    StreamArea = 7,
    CanalArea = 8,
    GlacierArea = 9,
    VoidArea = 9997,
    NullArea = 9998,
    UnknownArea = 9999
};


// return the type of the shapefile record
AreaType get_area_type(GDBFile *dbf, int rec);


#endif // _NAMES_HXX


// $Log$
// Revision 1.1  1999/02/23 01:29:05  curt
// Additional progress.
//
