// fixnode.cxx -- traverse the node file and fix the elevation of all the new
//                interpolated points.
//
// Written by Curtis Olson, started November 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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



#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#include <Misc/fgstream.hxx>

#include "fixnode.hxx"
#include "triload.hxx"


// load the node information
void load_nodes(const string& filename, container& node_list) {
    fgPoint3d node;
    int dim, junk1, junk2;
    int i, nodecount;

    cout << "Loading node file:  " + filename + " ...\n";

    fg_gzifstream in( filename );
    if ( !in ) {
	cout << "Cannot open " + filename + "\n";
	// exit immediately assuming an airport file for this tile
	// doesn't exist.
	exit(-1);
    }

    // Read header line
    in.stream() >> nodecount >> dim >> junk1 >> junk2;
    cout << "    Expecting " << nodecount << " nodes\n";

    // start with an empty list :-)
    node_list.erase( node_list.begin(), node_list.end() );

    in.eat_comments();
    while ( ! in.eof() ) {
	in.stream() >> junk1 >> node.x >> node.y >> node.z >> junk2;
	in.eat_comments();
	node_list.push_back(node);
    }
}


// fix the node elevations
void fix_nodes( const string& filename, fgDEM& dem, container& node_list )
{
    string toname;
    FILE *fd;
    int i;

    cout << "Fixing up node elevations\n";

    iterator current;
    iterator last = node_list.end();
    for ( current = node_list.begin() ; current != last ; ++current ) {
	// printf("Current: %d %.2f %.2f %.2f\n", i, nodes[i][0],
	//        nodes[i][1], nodes[i][2]);

	(*current).z = 
	    dem.interpolate_altitude( (*current).x, (*current).y );

	// printf("Fixed: %d %.2f %.2f %.2f\n", i, nodes[i][0],
	//        nodes[i][1], nodes[i][2]);
    }


    toname = filename + ".orig";
    cout << "Moving " + filename + " to " + toname + "\n";
    rename( filename.c_str(), toname.c_str() );

    cout << "Saving new node file: " + filename + "\n";

    fd = fopen(filename.c_str(), "w");

    fprintf( fd, "%d 2 1 0\n", node_list.size() );

    i = 1;
    for ( current = node_list.begin() ; current != last ; ++current ) {
	fprintf( fd, "%d %.2f %.2f %.2f 0\n", i,
		 (*current).x, (*current).y, (*current).z );
	++i;
    }

    fclose(fd);
}


// $Log$
// Revision 1.4  1998/09/19 20:43:52  curt
// C++-ified and STL-ified the code.  Combined triload.* and fixnode.* into
// a single file.
//
// Revision 1.3  1998/07/22 21:46:40  curt
// Fixed a bug that was triggering a seg fault.
//
// Revision 1.2  1998/04/14 02:26:03  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.1  1998/04/08 23:05:56  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.5  1998/03/19 02:50:19  curt
// Updated to support -lDEM class.
//
// Revision 1.4  1998/03/03 16:00:57  curt
// More c++ compile tweaks.
//
// Revision 1.3  1998/01/09 23:03:08  curt
// Restructured to split 1deg x 1deg dem's into 64 subsections.
//
// Revision 1.2  1997/12/02 13:12:07  curt
// Updated to fix every node.
//
// Revision 1.1  1997/11/27 00:17:33  curt
// Initial revision.
//

