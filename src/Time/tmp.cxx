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

#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#include <FDM/flight.hxx>
#include <Main/options.hxx>

#include "light.hxx"
#include "moonpos.hxx"
#include "sunpos.hxx"
#include "tmp.hxx"


FGMagVar::FGMagVar() {
}

FGMagVar::~FGMagVar() {
}


void FGMagVar::update( double lon, double lat, double alt_m, double jd ) {
    // Calculate local magnetic variation
    double field[6];
    // cout << "alt_m = " << alt_m << endl;
    magvar = SGMagVar( lat, lon, alt_m / 1000.0, (long)jd, field );
    magdip = atan(field[5]/sqrt(field[3]*field[3]+field[4]*field[4]));
}

FGMagVar cur_magvar;


// periodic time updater wrapper
void fgUpdateLocalTime() {

    SGTime::cur_time_params->updateLocal( cur_fdm_state->get_Longitude(),
					  cur_fdm_state->get_Latitude(),
					  current_options.get_fg_root() );
}


// update sky and lighting parameters
void fgUpdateSkyAndLightingParams() {
    fgUpdateSunPos();
    fgUpdateMoonPos();
    cur_light_params.Update();
}
