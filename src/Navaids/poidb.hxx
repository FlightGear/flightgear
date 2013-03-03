// poidb.cxx -- points of interest management routines
//
// Written by Christian Schmitt, March 2013
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


#ifndef _FG_POIDB_HXX
#define _FG_POIDB_HXX


#include <simgear/compiler.h>


// forward decls
class SGPath;

namespace flightgear
{

// load and initialize the POI database
bool poiDBInit(const SGPath& path);

} // of namespace flightgear

#endif // _FG_NAVDB_HXX
