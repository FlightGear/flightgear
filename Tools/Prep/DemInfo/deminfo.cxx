// deminfo.cxx -- main loop
//
// Written by Curtis Olson, started June 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <DEM/dem.hxx>


// static float dem_data[DEM_SIZE_1][DEM_SIZE_1];
// static float output_data[DEM_SIZE_1][DEM_SIZE_1];


int main(int argc, char **argv) {
    // DEM data
    FGDem dem;
    string filename;
    double error;
    int i, j;

    if ( argc != 2 ) {
	printf("Usage: %s <file.dem>\n", argv[0]);
	exit(-1);
    }

    // set input dem file name
    filename = argv[1];

    dem.open(filename);

    if ( dem.read_a_record() ) {
	cout << "Results = " << filename << "  "
	     << dem.get_originx() / 3600.0 << " "
	     << dem.get_originy() / 3600.0 << "\n";
    } else {
	cout << "Error parsing DEM file.\n";
    }

    dem.close();

    return(0);
}


