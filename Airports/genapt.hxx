//
// getapt.hxx -- generate airport scenery from the given definition file
//
// Written by Curtis Olson, started September 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#ifndef _GENAPT_HXX
#define _GENAPT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <string>        // Standard C++ string library
#include <set>
#include "Include/fg_stl_config.h"

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include <Scenery/tile.hxx>


// maximum size of airport perimeter structure, even for complex
// airports such as KORD this number is typically not very big.
#define MAX_PERIMETER 20

// name of the material to use for the airport base
#define APT_BASE_MATERIAL "grass"


// Load a .apt file and register the GL fragments with the
// corresponding tile
int
fgAptGenerate(const string& path, fgTILE *tile);


#endif /* _AIRPORTS_HXX */


// $Log$
// Revision 1.1  1998/09/14 02:14:01  curt
// Initial revision of genapt.[ch]xx for generating airport scenery.
//
//
