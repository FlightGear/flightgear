// tmp.cxx -- stuff I don't know what to do with at the moment
//
// Written by Curtis Olson, started July 2000.
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/fgpath.hxx>
#include <simgear/magvar/magvar.hxx>

#include <FDM/flight.hxx>
#include <Main/globals.hxx>

#include "light.hxx"
#include "moonpos.hxx"
#include "sunpos.hxx"
#include "tmp.hxx"


// periodic time updater wrapper
void fgUpdateLocalTime() {
    FGPath zone( globals->get_options()->get_fg_root() );
    zone.append( "Timezone" );

    globals->get_time_params()->updateLocal( cur_fdm_state->get_Longitude(),
					     cur_fdm_state->get_Latitude(),
					     zone.str() );
}


// update sky and lighting parameters
void fgUpdateSkyAndLightingParams() {
    fgUpdateSunPos();
    fgUpdateMoonPos();
    cur_light_params.Update();
}
