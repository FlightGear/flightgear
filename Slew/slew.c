/**************************************************************************
 * slew.c -- the "slew" flight model
 *
 * Written by Curtis Olson, started May 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <math.h>

#include "slew.h"
#include "../flight.h"
#include "../../Aircraft/aircraft.h"
#include "../../Controls/controls.h"
#include "../../constants.h"


#ifndef M_PI                                    
#define M_PI        3.14159265358979323846      /* pi */
#endif                                                           

#ifndef PI2                                               
#define PI2  (M_PI + M_PI)                      
#endif        


/* reset flight params to a specific position */
void fgSlewInit(double pos_x, double pos_y, double pos_z, double heading) {
    struct FLIGHT *f;

    f = &current_aircraft.flight;

    /*    f->pos_x = pos_x;
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

    f->Psi = heading; */
}


/* update position based on inputs, positions, velocities, etc. */
void fgSlewUpdate() {
    struct FLIGHT *f;
    struct CONTROLS *c;

    f = &current_aircraft.flight;
    c = &current_aircraft.controls;

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


/* $Log$
/* Revision 1.6  1997/08/27 03:30:11  curt
/* Changed naming scheme of basic shared structures.
/*
 * Revision 1.5  1997/07/19 22:35:06  curt
 * Moved fiddled with PI to avoid compiler warnings.
 *
 * Revision 1.4  1997/06/21 17:12:51  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.3  1997/05/29 22:40:00  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.2  1997/05/29 12:30:19  curt
 * Some initial mods to work better in a timer environment.
 *
 * Revision 1.1  1997/05/29 02:29:42  curt
 * Moved to their own directory.
 *
 * Revision 1.2  1997/05/23 15:40:37  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:04:45  curt
 * Initial revision.
 *
 */
