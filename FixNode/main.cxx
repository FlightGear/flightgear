// main.cxx -- read in a .node file and fix the z values of the interpolated 
//             points
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
//


#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#include <DEM/dem.hxx>

#include "fixnode.hxx"
#include "triload.hxx"


// Storage for the original DEM data which is used to interpolate z values
fgDEM dem;

// Node list
static double nodes[MAX_NODES][3];


// find all the matching files in the specified directory and fix them
void process_files(const string& root_path) {
    DIR *d;
    struct dirent *de;
    string file_path;
    char *ptr;
    int len;

    if ( (d = opendir( root_path.c_str() )) == NULL ) {
        cout << "cannot open directory " + root_path + "\n";
	exit(-1);
    }

    while ( (de = readdir(d)) != NULL ) {
	len = strlen(de->d_name);
	if ( len > 7 ) {
	    ptr = de->d_name;
	    ptr += (len - 7);
	    // printf("--> %s \n", ptr);

	    if ( strcmp(ptr, ".1.node") == 0 ) {
		file_path =  root_path + "/" + de->d_name;
		cout << "File = " + file_path + "\n";

		// load the input data files
		triload(file_path.c_str(), nodes);

		fixnodes(file_path.c_str(), &dem, nodes);
	    }
	}
    }
}


// main
int main(int argc, char **argv) {
    string demfile, root_path;

    if ( argc != 3 ) {
	printf("Usage %s demfile root_path\n", argv[0]);
	exit(-1);
    }

    cout << "Starting fixnode\n";

    demfile = argv[1];
    root_path = argv[2];

    // load the corresponding dem file so we can interpolate elev values
    dem.open(demfile);
    dem.parse();
    dem.close();

    // process all the *.1.node files in the specified directory
    process_files(root_path);

    return(0);
}


// $Log$
// Revision 1.6  1998/09/19 18:01:27  curt
// Support for changes to libDEM.a
//
// Revision 1.5  1998/07/22 21:46:41  curt
// Fixed a bug that was triggering a seg fault.
//
// Revision 1.4  1998/06/27 16:55:24  curt
// Changed include order for <sys/types.h>
//
// Revision 1.3  1998/04/26 05:02:06  curt
// Added #ifdef HAVE_STDLIB_H
//
// Revision 1.2  1998/04/14 02:26:04  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.1  1998/04/08 23:05:57  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.6  1998/04/06 21:09:44  curt
// Additional win32 support.
// Fixed a bad bug in dem file parsing that was causing the output to be
// flipped about x = y.
//
// Revision 1.5  1998/03/19 02:50:20  curt
// Updated to support -lDEM class.
//
// Revision 1.4  1998/03/03 16:00:58  curt
// More c++ compile tweaks.
//
// Revision 1.3  1998/01/09 23:03:08  curt
// Restructured to split 1deg x 1deg dem's into 64 subsections.
//
// Revision 1.2  1997/12/02 13:12:07  curt
// Updated to fix every node.
//
// Revision 1.1  1997/11/27 00:17:34  curt
// Initial revision.
//
//
