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


#include "genobj.hxx"


// Calculate the global bounding sphere from all the input points.
// Center is the average of the points.
static void calc_gbs( const trinode_list& nodelist, Point3D *center, 
		      double *radius )
{
    double x = 0;
    double y = 0;
    double z = 0;

    double dist_squared;
    double radius_squared = 0;
    
    const_trinode_list_iterator current = nodelist.begin();
    const_trinode_list_iterator last = nodelist.end();

    for ( ; current != last; ++current ) {
	x += current->x();
	y += current->y();
	z += current->z();
    }

    x /= nodelist.size();
    y /= nodelist.size();
    z /= nodelist.size();

    *center = Point3D(x, y, z);

    current = nodelist.begin();
    for ( ; current != last; ++current ) {
        dist_squared = center->distance3Dsquared(*current);
	if ( dist_squared > radius_squared ) {
            radius_squared = dist_squared;
        }
    }

    *radius = sqrt(radius_squared);
}


// generate the flight gear format from the triangulation
int fgGenOutput( const FGTriangle& t ) {
    Point3D gbs;
    double gradius;

    FGTriNodes trinodes = t.get_out_nodes();
    trinode_list nodelist = trinodes.get_node_list();

    calc_gbs( nodelist, &gbs, &gradius );
    cout << "center = " << gbs << " radius = " << gradius << endl;
    return 1;
}


// $Log$
// Revision 1.1  1999/03/22 23:51:51  curt
// Initial revision.
//
