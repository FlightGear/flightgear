// JSBsim.hxx -- interface to the "JSBsim" flight model
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _JSBSIM_HXX
#define _JSBSIM_HXX

#include <FDM/JSBsim/FGFDMExec.h>
#undef MAX_ENGINES

#include <Aircraft/aircraft.hxx>


// reset flight params to a specific position 
int fgJSBsimInit(double dt);

// update position based on inputs, positions, velocities, etc.
int fgJSBsimUpdate(FGInterface& f, int multiloop);

// Convert from the FGInterface struct to the JSBsim generic_ struct
int FGInterface_2_JSBsim (FGInterface& f);

// Convert from the JSBsim generic_ struct to the FGInterface struct
int fgJSBsim_2_FGInterface (FGInterface& f);


#endif // _JSBSIM_HXX


// $Log$
// Revision 1.1  1999/04/05 21:32:45  curt
// Initial revision
//
// Revision 1.2  1999/02/11 21:09:41  curt
// Interface with Jon's submitted JSBsim changes.
//
// Revision 1.1  1999/02/05 21:29:38  curt
// Incorporating Jon S. Berndt's flight model code.
//
