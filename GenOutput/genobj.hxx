// genobj.hxx -- Generate the flight gear "obj" file format from the
//               triangle output
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _GENOBJ_HXX
#define _GENOBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Triangulate/triangle.hxx>


// generate the flight gear format from the triangulation
int fgGenOutput( const FGTriangle& t );


#endif // _GENOBJ_HXX


// $Log$
// Revision 1.1  1999/03/22 23:51:51  curt
// Initial revision.
//
