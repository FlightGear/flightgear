// array.hxx -- Array management class
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998 - 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _ARRAY_HXX
#define _ARRAY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Bucket/newbucket.hxx>
#include <Misc/fgstream.hxx>


#define ARRAY_SIZE 1200
#define ARRAY_SIZE_1 1201


class FGArray {

private:

    // file pointer for input
    // gzFile fd;
    fg_gzifstream *in;

    // coordinates (in arc seconds) of south west corner
    double originx, originy;
    
    // number of columns and rows
    int cols, rows;
    
    // Distance between column and row data points (in arc seconds)
    double col_step, row_step;
    
    // pointers to the actual grid data allocated here
    float (*in_data)[ARRAY_SIZE_1];
    float (*out_data)[ARRAY_SIZE_1];

    // Current "A" Record Information
    // char dem_description[80], dem_quadrangle[80];
    // double dem_x1, dem_y1, dem_x2, dem_y2, dem_x3, dem_y3, dem_x4, dem_y4;
    // double dem_z1, dem_z2;
    // int dem_resolution, dem_num_profiles;
  
    // Current "B" Record Information
    // int prof_col, prof_row;
    // int prof_num_cols, prof_num_rows;
    // double prof_x1, prof_y1;
    // int prof_data;

    // temporary values for the class to use
    // char option_name[32];
    // int do_data;
    // int cur_col, cur_row;

    // Initialize output mesh structure
    void outputmesh_init( void );

    // Get the value of a mesh node
    double outputmesh_get_pt( int i, int j );

    // Set the value of a mesh node
    void outputmesh_set_pt( int i, int j, double value );

#if 0
    // Write out a node file that can be used by the "triangle" program
    void outputmesh_output_nodes( const string& fg_root, FGBucket& p );
#endif

public:

    // Constructor
    FGArray( void );
    FGArray( const string& file );

    // Destructor
    ~FGArray( void );

    // open an Array file (use "-" if input is coming from stdin)
    int open ( const string& file );

    // close a Array file
    int close();

    // parse a Array file
    int parse();

    // Use least squares to fit a simpler data set to dem data
    void fit( FGBucket& p, double error );

    // return the current altitude based on grid data.  We should
    // rewrite this to interpolate exact values, but for now this is
    // good enough
    double interpolate_altitude( double lon, double lat );

    // Informational methods
    inline double get_originx() const { return originx; }
    inline double get_originy() const { return originy; }
    inline int get_cols() const { return cols; }
    inline int get_rows() const { return rows; }
    inline double get_col_step() const { return col_step; }
    inline double get_row_step() const { return row_step; }
};


#endif // _ARRAY_HXX


// $Log$
// Revision 1.1  1999/03/13 18:45:02  curt
// Initial revision. (derived from libDEM.a code.)
//
