// viewer.cxx -- class for managing a viewer in the flightgear world.
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


#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/ssg.h>		// plib include

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/vector.hxx>

#include "globals.hxx"
#include "viewer.hxx"


// Constructor
FGViewer::FGViewer( void )
{
}


#define USE_FAST_VIEWROT
#ifdef USE_FAST_VIEWROT
// VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
// This takes advantage of the fact that VIEWo and VIEW_OFFSET
// only have entries in the upper 3x3 block
// and that LARC_TO_SSG is just a shift of rows   NHV
inline static void fgMakeViewRot( sgMat4 dst, const sgMat4 m1, const sgMat4 m2 )
{
    for ( int j = 0 ; j < 3 ; j++ ) {
	dst[2][j] = m2[0][0] * m1[0][j] +
	    m2[0][1] * m1[1][j] +
	    m2[0][2] * m1[2][j];

	dst[0][j] = m2[1][0] * m1[0][j] +
	    m2[1][1] * m1[1][j] +
	    m2[1][2] * m1[2][j];

	dst[1][j] = m2[2][0] * m1[0][j] +
	    m2[2][1] * m1[1][j] +
	    m2[2][2] * m1[2][j];
    }
    dst[0][3] = 
	dst[1][3] = 
	dst[2][3] = 
	dst[3][0] = 
	dst[3][1] = 
	dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE;
}
#endif


// Initialize a view structure
void FGViewer::init( void ) {
    FG_LOG( FG_VIEW, FG_ALERT, "Shouldn't ever see this" );
    exit(-1);
}


// Update the view parameters
void FGViewer::update() {
    FG_LOG( FG_VIEW, FG_ALERT, "Shouldn't ever see this" );
    exit(-1);
}


// Destructor
FGViewer::~FGViewer( void ) {
}
