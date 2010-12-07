// navdb.hxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVDB_HXX
#define _FG_NAVDB_HXX


#include <simgear/compiler.h>

class FGNavList;
class FGTACANList;

// load and initialize the navigational databases
bool fgNavDBInit( FGNavList *navlist, FGNavList *loclist, FGNavList *gslist,
                  FGNavList *dmelist,
                  FGNavList *tacanlist, FGNavList *carrierlist,
                  FGTACANList *channellist );


/**
 * Return the property node corresponding to the runway ILS installation,
 * from the Airports/I/C/A/ICAO.ils.xml file (loading it if necessary)
 * returns NULL is no ILS data is defined for the runway.
 */
SGPropertyNode* ilsDataForRunwayAndNavaid(FGRunway* aRunway, const std::string& aNavIdent);

/**
 * Helper to map a nav.data name (eg 'KBWI 33R GS') into a FGRunway reference.
 * returns NULL, and complains loudly, if the airport/runway is not found.
 */
FGRunway* getRunwayFromName(const std::string& aName);

#endif // _FG_NAVDB_HXX
