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
 
#include <Include/compiler.h>

#include STL_STRING

#include "names.hxx"


// return area type from text name
AreaType get_area_type( string area ) {
    if ( area == "SomeSort" ) {
	return SomeSortOfArea;
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
    } else if ( area == "Default" ) {
	return DefaultArea;
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


