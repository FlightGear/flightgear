//*************************************************************************
// LaRCsim.hxx -- interface to the "LaRCsim" flight model
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
//*************************************************************************/


#ifndef _LARCSIM_HXX
#define _LARCSIM_HXX


#include "IO360.hxx"
#include "flight.hxx"


#define USE_NEW_ENGINE_CODE 1


class FGLaRCsim: public FGInterface {

#ifdef USE_NEW_ENGINE_CODE
    FGEngine eng;
#endif

public:

    // copy FDM state to LaRCsim structures
    int copy_to_LaRCsim();

    // copy FDM state from LaRCsim structures
    int copy_from_LaRCsim();

    // reset flight params to a specific position 
    int init( double dt );

    // update position based on inputs, positions, velocities, etc.
    int update( int multiloop );
};


#endif // _LARCSIM_HXX


