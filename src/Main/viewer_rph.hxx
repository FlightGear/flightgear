// viewer_rph.hxx -- class for managing a Roll/Pitch/Heading viewer in
//                   the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _VIEWER_RPH_HXX
#define _VIEWER_RPH_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include "viewer.hxx"


// Define a structure containing view information
class FGViewerRPH: public FGViewer {

private:

    // view orientation (roll, pitch, heading)
    sgVec3 rph;

    // up vector for the view (usually point straight up through the
    // top of the aircraft
    sgVec3 view_up;

    // the vector pointing straight out the nose of the aircraft
    sgVec3 view_forward;

    // Transformation matrix for the view direction offset relative to
    // the AIRCRAFT matrix
    sgMat4 VIEW_OFFSET;

    // sg versions of our friendly matrices
    sgMat4 LOCAL, TRANS, LARC_TO_SSG;

    // Update the view volume, position, and orientation
    void update();

public:

    // Constructor
    FGViewerRPH( void );

    // Destructor
    ~FGViewerRPH( void );

    // Initialize a view class
    void init( void );

    //////////////////////////////////////////////////////////////////////
    // setter functions
    //////////////////////////////////////////////////////////////////////
    inline void set_rph( double r, double p, double h ) {
	// data should be in radians
	set_dirty();
	sgSetVec3( rph, r, p, h );
    }

    //////////////////////////////////////////////////////////////////////
    // accessor functions
    //////////////////////////////////////////////////////////////////////
    inline float *get_rph() { return rph; }
    inline float *get_view_forward() { return view_forward; }
    inline float *get_view_up() { return view_up; }

    //////////////////////////////////////////////////////////////////////
    // derived values accessor functions
    //////////////////////////////////////////////////////////////////////
};


#endif // _VIEWER_RPH_HXX
