// viewer_lookat.hxx -- class for managing a "look at" viewer in
//                      the flightgear world.
//
// Written by Curtis Olson, started October 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _VIEWER_LOOKAT_HXX
#define _VIEWER_LOOKAT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include "viewer.hxx"


// Define a structure containing view information
class FGViewerLookAt: public FGViewer {

private:

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
    FGViewerLookAt( void );

    // Destructor
    ~FGViewerLookAt( void );

    //////////////////////////////////////////////////////////////////////
    // setter functions
    //////////////////////////////////////////////////////////////////////
    inline void set_view_forward( sgVec3 vf ) {
	set_dirty();
	sgCopyVec3( view_forward, vf );
    }
    inline void set_view_up( sgVec3 vf ) {
	set_dirty();
	sgCopyVec3( view_up, vf );
    }

    //////////////////////////////////////////////////////////////////////
    // accessor functions
    //////////////////////////////////////////////////////////////////////
    inline float *get_view_forward() { return view_forward; }
    inline float *get_view_up() { return view_up; }

    //////////////////////////////////////////////////////////////////////
    // derived values accessor functions
    //////////////////////////////////////////////////////////////////////
};


#endif // _VIEWER_LOOKAT_HXX
