// names.cxx -- process shapefiles names
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
 
#include <Include/compiler.h>

#include STL_STRING

#include "names.hxx"


// return area type from text name
AreaType get_area_type( string area ) {
    if ( area == "Default" ) {
	return DefaultArea;
    } else if ( area == "AirportKeep" ) {
	return AirportKeepArea;
    } else if ( area == "AirportIgnore" ) {
	return AirportIgnoreArea;
    } else if ( (area == "Swamp or Marsh")
		|| (area == "Marsh") ) {
	return MarshArea;
    } else if ( (area == "Bay  Estuary or Ocean")
                || (area == "Ocean") ) {
	return OceanArea;
    } else if ( area == "Lake" ) {
	return LakeArea;
    } else if ( (area == "Lake   Dry")
		|| (area == "DryLake") ) {
	return DryLakeArea;
    } else if ( (area == "Lake   Intermittent")
		|| (area == "IntermittentLake") ) {
	return IntLakeArea;
    } else if ( area == "Reservoir" ) {
	return ReservoirArea;
    } else if ( (area == "Reservoir   Intermittent")
		|| (area == "IntermittentReservoir") ) {
	return IntReservoirArea;
    } else if ( area == "Stream" ) {
	return StreamArea;
    } else if ( area == "Canal" ) {
	return CanalArea;
    } else if ( area == "Glacier" ) {
	return GlacierArea;
    } else if ( area == "Void Area" ) {
	return VoidArea;
    } else if ( area == "Null" ) {
	return NullArea;
    } else {
	cout << "unknown area = '" << area << "'" << endl;
	// cout << "area = " << area << endl;
	// for ( int i = 0; i < area.length(); i++ ) {
	//  cout << i << ") " << (int)area[i] << endl;
	// }
	return UnknownArea;
    }
}


// return text from of area name
string get_area_name( AreaType area ) {
    if ( area == DefaultArea ) {
	return "Default";
    } else if ( area == AirportKeepArea ) {
	return "AirportKeep";
    } else if ( area == AirportIgnoreArea ) {
	return "AirportIgnore";
    } else if ( area == MarshArea ) {
	return "Marsh";
    } else if ( area == OceanArea ) {
	return "Ocean";
    } else if ( area == LakeArea ) {
	return "Lake";
    } else if ( area == DryLakeArea ) {
	return "DryLake";
    } else if ( area == IntLakeArea ) {
	return "IntermittentLake";
    } else if ( area == ReservoirArea ) {
	return "Reservoir";
    } else if ( area == IntReservoirArea ) {
	return "IntermittentReservoir";
    } else if ( area == StreamArea ) {
	return "Stream";
    } else if ( area == CanalArea ) {
	return "Canal";
    } else if ( area == GlacierArea ) {
	return "Glacier";
    } else if ( area == VoidArea ) {
	return "VoidArea";
    } else if ( area == NullArea ) {
	return "Null";
    } else {
	cout << "unknown area code = " << (int)area << endl;
	return "Unknown";
    }
}


// $Log$
// Revision 1.7  1999/04/01 13:52:13  curt
// Version 0.6.0
// Shape name tweak.
// Removing tool: FixNode
//
// Revision 1.6  1999/03/27 05:31:24  curt
// Make 0 the default area type since this corresponds well with the conventions
//   used by the triangulator.
//
// Revision 1.5  1999/03/22 23:49:29  curt
// Moved AreaType get_shapefile_type(GDBFile *dbf, int rec) to where it
// belongs in ShapeFile/
//
// Revision 1.4  1999/03/13 18:47:04  curt
// Removed an unused variable.
//
// Revision 1.3  1999/03/02 01:03:58  curt
// Added more reverse lookup support.
//
// Revision 1.2  1999/03/01 15:35:52  curt
// Generalized the routines a bit to make them more useful.
//
// Revision 1.1  1999/02/25 21:30:24  curt
// Initial revision.
//
// Revision 1.1  1999/02/23 01:29:05  curt
// Additional progress.
//
