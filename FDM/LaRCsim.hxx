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
// (Log is kept at end of this file)
//*************************************************************************/


#ifndef _LARCSIM_HXX
#define _LARCSIM_HXX


#include "flight.hxx"


// reset flight params to a specific position 
int fgLaRCsimInit(double dt);

// update position based on inputs, positions, velocities, etc.
int fgLaRCsimUpdate(fgFLIGHT& f, int multiloop);

// Convert from the fgFLIGHT struct to the LaRCsim generic_ struct
int fgFlight_2_LaRCsim (fgFLIGHT& f);

// Convert from the LaRCsim generic_ struct to the fgFLIGHT struct
int fgLaRCsim_2_Flight (fgFLIGHT& f);


#endif // _LARCSIM_HXX


// $Log$
// Revision 1.3  1998/12/03 01:16:38  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.2  1998/10/17 01:34:13  curt
// C++ ifying ...
//
// Revision 1.1  1998/10/17 00:43:58  curt
// Initial revision.
//
