// dme.cxx -- class to manage an instance of the DME
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Navaids/navlist.hxx>

#include "dme.hxx"

#include <string>
SG_USING_STD(string);


/**
 * Boy, this is ugly!  Make the VOR range vary by altitude difference.
 */
static double kludgeRange ( double stationElev, double aircraftElev,
			    double nominalRange)
{
				// Assume that the nominal range (usually
				// 50nm) applies at a 5,000 ft difference.
				// Just a wild guess!
  double factor = (aircraftElev - stationElev)*SG_METER_TO_FEET / 5000.0;
  double range = fabs(nominalRange * factor);

				// Clamp the range to keep it sane; for
				// now, never less than 25% or more than
				// 500% of nominal range.
  if (range < nominalRange/4.0) {
    range = nominalRange/4.0;
  } else if (range > nominalRange*5.0) {
    range = nominalRange*5.0;
  }

  return range;
}


// Constructor
FGDME::FGDME() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    bus_power(fgGetNode("/systems/electrical/outputs/dme", true)),
    navcom1_bus_power(fgGetNode("/systems/electrical/outputs/navcom[0]", true)),
    navcom2_bus_power(fgGetNode("/systems/electrical/outputs/navcom[1]", true)),
    navcom1_power_btn(fgGetNode("/radios/comm[0]/inputs/power-btn", true)),
    navcom2_power_btn(fgGetNode("/radios/comm[1]/inputs/power-btn", true)),
    navcom1_freq(fgGetNode("/radios/nav[0]/frequencies/selected-mhz", true)),
    navcom2_freq(fgGetNode("/radios/nav[1]/frequencies/selected-mhz", true)),
    need_update(true),
    freq(0.0),
    bias(0.0),
    dist(0.0),
    prev_dist(0.0),
    spd(0.0),
    ete(0.0)
{
    last_time.stamp();
}


// Destructor
FGDME::~FGDME() 
{
}


void
FGDME::init ()
{
}

void
FGDME::bind ()
{

				// User inputs
    fgTie("/radios/dme/frequencies/selected-khz", this,
	  &FGDME::get_freq, &FGDME::set_freq);

				// Radio outputs
    fgTie("/radios/dme/in-range", this, &FGDME::get_inrange);

    fgTie("/radios/dme/distance-nm", this, &FGDME::get_dist);

    fgTie("/radios/dme/speed-kt", this, &FGDME::get_spd);

    fgTie("/radios/dme/ete-min", this, &FGDME::get_ete);
}

void
FGDME::unbind ()
{
    fgUntie("/radios/dme/frequencies/selected-khz");

				// Radio outputs
    fgUntie("/radios/dme/in-range");
    fgUntie("/radios/dme/distance-nm");
    fgUntie("/radios/dme/speed-kt");
    fgUntie("/radios/dme/ete-min");
}


// Update the various nav values based on position and valid tuned in navs
void 
FGDME::update(double dt) 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    need_update = false;

    Point3D aircraft = sgGeodToCart( Point3D( lon, lat, elev ) );
    Point3D station;

    if ( valid && has_power() ) {
	station = Point3D( x, y, z );
	dist = aircraft.distance3D( station ) * SG_METER_TO_NM;
	effective_range = kludgeRange(elev, elev, range);
	if (dist < effective_range * SG_NM_TO_METER) {
            inrange = true;
	} else if (dist < 2 * effective_range * SG_NM_TO_METER) {
            inrange = sg_random() <
                (2 * effective_range * SG_NM_TO_METER - dist) /
                (effective_range * SG_NM_TO_METER);
	} else {
            inrange = false;
	}
	if ( inrange ) {
            SGTimeStamp current_time;
            station = Point3D( x, y, z );
            dist = aircraft.distance3D( station ) * SG_METER_TO_NM;
            cout << "dist = " << dist << endl;
            dist -= bias;
            cout << "  bias = " << bias << endl;
            cout << "    dist = " << dist << endl;

            current_time.stamp();
            long dMs = (current_time - last_time) / 1000;
				// Update every second
            if (dMs >= 1000) {
                double dDist = dist - prev_dist;
                spd = fabs((dDist/dMs) * 3600000);
				// FIXME: the panel should be able to
				// handle this!!!
                if (spd > 999.0)
                    spd = 999.0;
                ete = fabs((dist/spd) * 60.0);
				// FIXME: the panel should be able to
				// handle this!!!
                if (ete > 99.0)
                    ete = 99.0;
                prev_dist = dist;
                last_time.stamp();
            }
	}
    } else {
        inrange = false;
        dist = 0.0;
        prev_dist = 0.0;
        spd = 0.0;
        ete = 0.0;
    }
}


// Update current nav/adf radio stations based on current postition
void FGDME::search() 
{
    double lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

				// FIXME: the panel should handle this
				// don't worry about overhead for now,
				// since this is handled only periodically
    switch_pos = fgGetInt("/radios/dme/switch-position", 2);
    if ( switch_pos == 1 && has_power() && navcom1_on() ) {
        if ( freq != navcom1_freq->getDoubleValue() ) {
            freq = navcom1_freq->getDoubleValue();
            need_update = true;
        }
    } else if ( switch_pos == 3 && has_power() && navcom2_on() ) {
        if ( freq != navcom2_freq->getDoubleValue() ) {
            freq = navcom2_freq->getDoubleValue();
            need_update = true;
        }
    } else if ( switch_pos == 2 && has_power() ) {
        // no-op
    } else {
        freq = 0;
        inrange = false;
    }

    FGNavRecord *dme
        = globals->get_dmelist()->findByFreq( freq, lon, lat, elev );

    if ( dme != NULL ) {
        valid = true;
        lon = dme->get_lon();
        lat = dme->get_lat();
        elev = dme->get_elev_ft();
        bias = dme->get_multiuse();
        range = FG_ILS_DEFAULT_RANGE;
        effective_range = kludgeRange(elev, elev, range);
        x = dme->get_x();
        y = dme->get_y();
        z = dme->get_z();
    } else {
        valid = false;
        dist = 0;
    }
}
