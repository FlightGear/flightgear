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


#include <Include/compiler.h>

#include <vector>

#include <Bucket/newbucket.hxx>
#include <Math/point3d.hxx>
#include <Misc/fgstream.hxx>

#include <Main/construct_types.hxx>

FG_USING_STD(vector);


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
    // float (*out_data)[ARRAY_SIZE_1];

    // output nodes
    point_list corner_list;
    point_list node_list;

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
    int parse( FGBucket& b );

    // Use least squares to fit a simpler data set to dem data.
    // Return the number of fitted nodes
    int fit( double error );

    // add a node to the output corner node list
    void add_corner_node( int i, int j, double val );

    // add a node to the output fitted node list
    void add_fit_node( int i, int j, double val );

    // return the current altitude based on grid data.  We should
    // rewrite this to interpolate exact values, but for now this is
    // good enough
    double interpolate_altitude( double lon, double lat ) const;

    // Informational methods
    inline double get_originx() const { return originx; }
    inline double get_originy() const { return originy; }
    inline int get_cols() const { return cols; }
    inline int get_rows() const { return rows; }
    inline double get_col_step() const { return col_step; }
    inline double get_row_step() const { return row_step; }

    inline point_list get_corner_node_list() const { return corner_list; }
    inline point_list get_fit_node_list() const { return node_list; }
};


#endif // _ARRAY_HXX


// $Log$
// Revision 1.6  1999/04/05 02:15:24  curt
// Make dem fitting more robust in cases when no dem file available.
//
// Revision 1.5  1999/03/29 13:11:02  curt
// Shuffled stl type names a bit.
// Began adding support for tri-fanning (or maybe other arrangments too.)
//
// Revision 1.4  1999/03/27 05:20:14  curt
// Handle corner nodes separately from the rest of the fitted nodes.
// Fixed some "const" related warnings.
//
// Revision 1.3  1999/03/20 20:32:52  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.2  1999/03/13 23:50:27  curt
// Tweaked output formatting a bit.
//
// Revision 1.1  1999/03/13 18:45:02  curt
// Initial revision. (derived from libDEM.a code.)
//
