// -*- Mode: C++ -*-
//
// dem.h -- DEM management class
//
// Written by Curtis Olson, started March 1998.
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


#ifndef _DEM_H
#define _DEM_H


#include <stdio.h>

#include <Scenery/bucketutils.h>


#define DEM_SIZE 1200
#define DEM_SIZE_1 1201


class fgDEM {
    // file pointer for input
    FILE *fd;

    // coordinates (in arc seconds) of south west corner
    double originx, originy;
    
    // number of columns and rows
    int cols, rows;
    
    // Distance between column and row data points (in arc seconds)
    double col_step, row_step;
    
    // the actual mesh data allocated here
    // float dem_data[DEM_SIZE_1][DEM_SIZE_1];
    // float output_data[DEM_SIZE_1][DEM_SIZE_1];

    // Current "A" Record Information
    char dem_description[80], dem_quadrangle[80];
    double dem_x1, dem_y1, dem_x2, dem_y2, dem_x3, dem_y3, dem_x4, dem_y4;
    double dem_z1, dem_z2;
    int dem_resolution, dem_num_profiles;
  
    // Current "B" Record Information
    int prof_col, prof_row;
    int prof_num_cols, prof_num_rows;
    double prof_x1, prof_y1;
    int prof_data;

    // temporary values for the class to use
    char option_name[32];
    int do_data;
    int cur_col, cur_row;

public:

    // Constructor (opens a DEM file)
    fgDEM( void );

    // open a DEM file (use "-" if input is coming from stdin)
    int open ( char *file );

    // close a DEM file
    int close ( void );

    // parse a DEM file
    int parse( float dem_data[DEM_SIZE_1][DEM_SIZE_1] );

    // read and parse DEM "A" record
    void read_a_record( );

    // read and parse DEM "B" record
    void read_b_record( float dem_data[DEM_SIZE_1][DEM_SIZE_1] );

    // Informational methods
    double info_originx( void ) { return(originx); }
    double info_originy( void ) { return(originy); }

    // return the current altitude based on mesh data.  We should
    // rewrite this to interpolate exact values, but for now this is
    // good enough
    double interpolate_altitude( float dem_data[DEM_SIZE_1][DEM_SIZE_1], 
				 double lon, double lat);

    // Use least squares to fit a simpler data set to dem data
    void fit( float dem_data[DEM_SIZE_1][DEM_SIZE_1], 
	      float output_data[DEM_SIZE_1][DEM_SIZE_1], 
	      char *fg_root, double error, struct fgBUCKET *p );

    // Initialize output mesh structure
    void outputmesh_init( float output_data[DEM_SIZE_1][DEM_SIZE_1] );

    // Get the value of a mesh node
    double outputmesh_get_pt( float output_data[DEM_SIZE_1][DEM_SIZE_1],
			      int i, int j );

    // Set the value of a mesh node
    void outputmesh_set_pt( float output_data[DEM_SIZE_1][DEM_SIZE_1],
			    int i, int j, double value );

    // Write out a node file that can be used by the "triangle" program
    void outputmesh_output_nodes( float output_data[DEM_SIZE_1][DEM_SIZE_1],
				  char *fg_root, struct fgBUCKET *p );

    // Destructor
    ~fgDEM( void );
};


#endif // _DEM_H


// $Log$
// Revision 1.2  1998/03/23 20:35:42  curt
// Updated to use FG_EPSILON
//
// Revision 1.1  1998/03/19 02:54:47  curt
// Reorganized into a class lib called fgDEM.
//
// Revision 1.1  1998/03/19 01:46:29  curt
// Initial revision.
//
