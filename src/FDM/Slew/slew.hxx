// slew.hxx -- the "slew" flight model
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


#ifndef _SLEW_HXX
#define _SLEW_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


// reset flight params to a specific position 
void fgSlewInit(double pos_x, double pos_y, double pos_z, double heading);

// update position based on inputs, positions, velocities, etc.
void fgSlewUpdate( void );


#endif // _SLEW_HXX


