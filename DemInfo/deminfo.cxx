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
// (Log is kept at end of this file)
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DEM/dem.hxx>


// static float dem_data[DEM_SIZE_1][DEM_SIZE_1];
// static float output_data[DEM_SIZE_1][DEM_SIZE_1];


int main(int argc, char **argv) {
    // DEM data
    fgDEM dem;
    char fg_root[256];
    char filename[256];
    double error;
    int i, j;

    if ( argc != 2 ) {
	printf("Usage: %s <file.dem>\n", argv[0]);
	exit(-1);
    }

    // set input dem file name
    strcpy(filename, argv[1]);

    dem.open(filename);

    if ( dem.read_a_record() ) {
	printf("Results = %s  %.1f %.1f\n", 
	       filename, 
	       dem.info_originx() / 3600.0,
	       dem.info_originy() / 3600.0 ) ;
    } else {
	printf("Error parsing DEM file.\n");
    }

    dem.close();

    return(0);
}


// $Log$
// Revision 1.1  1998/06/04 19:18:05  curt
// Initial revision.
//
