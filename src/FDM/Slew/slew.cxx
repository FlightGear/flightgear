// slew.cxx -- the "slew" flight model
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


#include <math.h>

#include "slew.hxx"

#include <FDM/flight.hxx>
#include <Aircraft/aircraft.hxx>
#include <Controls/controls.hxx>
#include <Include/fg_constants.h>


// reset flight params to a specific position
void fgSlewInit(double pos_x, double pos_y, double pos_z, double heading) {
    FGInterface *f;
    
    f = current_aircraft.fdm_state;

    /*
    f->pos_x = pos_x;
    f->pos_y = pos_y;
    f->pos_z = pos_z;

    f->vel_x = 0.0;
    f->vel_y = 0.0;
    f->vel_z = 0.0;

    f->Phi = 0.0;
    f->Theta = 0.0;
    f->Psi = 0.0;

    f->vel_Phi = 0.0;
    f->vel_Theta = 0.0;
    f->vel_Psi = 0.0;

    f->Psi = heading;
    */
}


// update position based on inputs, positions, velocities, etc.
void fgSlewUpdate( void ) {
    FGInterface *f;
    FGControls *c;

    f = current_aircraft.fdm_state;
    c = current_aircraft.controls;

    /* f->Psi += ( c->aileron / 8 );
    if ( f->Psi > FG_2PI ) {
	f->Psi -= FG_2PI;
    } else if ( f->Psi < 0 ) {
	f->Psi += FG_2PI;
    }

    f->vel_x = -c->elev;

    f->pos_x = f->pos_x + (cos(f->Psi) * f->vel_x);
    f->pos_y = f->pos_y + (sin(f->Psi) * f->vel_x); */
}


