// simulates ridge lift
//
// Written by Patrice Poly
// Copyright (C) 2009 Patrice Poly - p.polypa@gmail.com
//
//
// Entirely based  on the paper : 
// http://carrier.csi.cam.ac.uk/forsterlewis/soaring/sim/fsx/dev/sim_probe/sim_probe_paper.html
// by Ian Forster-Lewis, University of Cambridge, 26th December 2007
//
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>


using std::string;

#include "ridge_lift.hxx"

static string CreateIndexedPropertyName(string Property, int index)
{
	std::stringstream str;
	str << index;
	string tmp;
	str >> tmp;
	return Property + "[" + tmp + "]";
}

static inline double sign(double x) {
	return x == 0 ? 0 : x > 0 ? 1.0 : -1.0;
}

static const	double BOUNDARY1_m = 40.0;

//constructor
FGRidgeLift::FGRidgeLift ()
{	
	dist_probe_m[0] = 0.0; // in meters
	dist_probe_m[1] = 250.0;
	dist_probe_m[2] = 750.0;
	dist_probe_m[3] = 2000.0;
	dist_probe_m[4] = -100.0;

	strength = 0.0;
	timer = 0.0;
	for( int i = 0; i < 5; i++ )
		probe_elev_m[i] = probe_lat_deg[i] = probe_lon_deg[i] = 0.0;

	for( int i = 0; i < 4; i++ )
		slope[i] = 0.0;
}

//destructor
FGRidgeLift::~FGRidgeLift()
{

}

void FGRidgeLift::init(void)
{
	_enabled_node = fgGetNode( "/environment/ridge-lift/enabled", false );

	_ridge_lift_fps_node = fgGetNode("/environment/ridge-lift-fps", true);
	_surface_wind_from_deg_node =
			fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg"
			, true);
	_surface_wind_speed_node =
			fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt"
			, true);
	_earth_radius_node = fgGetNode("/position/sea-level-radius-ft", true);
	_user_longitude_node = fgGetNode("/position/longitude-deg", true);
	_user_latitude_node = fgGetNode("/position/latitude-deg", true);
	_user_altitude_ft_node = fgGetNode("/position/altitude-ft", true);
	_user_altitude_agl_ft_node = fgGetNode("/position/altitude-agl-ft", true);
}

void FGRidgeLift::bind() {
	string prop;

	for( int i = 0; i < 5; i++ ) {
		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-elev-m", i );
		fgTie( prop.c_str(), this, i, &FGRidgeLift::get_probe_elev_m); // read-only

		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-lat-deg", i );
		fgTie( prop.c_str(), this, i, &FGRidgeLift::get_probe_lat_deg); // read-only

		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-lon-deg", i );
		fgTie( prop.c_str(), this, i, &FGRidgeLift::get_probe_lon_deg); // read-only
	}

	for( int i = 0; i < 4; i++ ) {
		prop = CreateIndexedPropertyName("/environment/ridge-lift/slope", i );
		fgTie( prop.c_str(), this, i, &FGRidgeLift::get_slope); // read-only
	}
}

void FGRidgeLift::unbind() {
	string prop;

	for( int i = 0; i < 5; i++ ) {

		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-elev-m", i );
		fgUntie( prop.c_str() );

		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-lat-deg", i );
		fgUntie( prop.c_str() );

		prop = CreateIndexedPropertyName("/environment/ridge-lift/probe-lon-deg", i );
		fgUntie( prop.c_str() );
	}

	for( int i = 0; i < 4; i++ ) {
		prop = CreateIndexedPropertyName("/environment/ridge-lift/slope", i );
		fgUntie( prop.c_str() );
	}
}

void FGRidgeLift::update(double dt) {

	if( _enabled_node && false == _enabled_node->getBoolValue() ) {
		if( strength != 0.0 ) {
			strength = 0.0;
			_ridge_lift_fps_node->setDoubleValue( 0 );
		}
		return;
	}

	//get the windspeed at ground level
	double ground_wind_from_rad = _surface_wind_from_deg_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	double ground_wind_speed_mps = _surface_wind_speed_node->getDoubleValue() * SG_NM_TO_METER / 3600;

	timer -= dt;
	if (timer <= 0.0 ) {
		// copy values 

		double user_latitude_rad = _user_latitude_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
		double user_longitude_rad = _user_longitude_node->getDoubleValue() * SG_DEGREES_TO_RADIANS;
	
		double earth_rad_m = _earth_radius_node->getDoubleValue() * SG_FEET_TO_METER;
		if( earth_rad_m < SG_EPSILON )
				earth_rad_m = SG_EARTH_RAD * 1000;

		// Placing the probes
	
		for (int i = 0; i < sizeof(probe_lat_rad)/sizeof(probe_lat_rad[0]); i++) {
			double probe_radius_ratio = dist_probe_m[i]/earth_rad_m;
			double sin_probe_radius_ratio = sin(probe_radius_ratio);

			probe_lat_rad[i] = asin(sin(user_latitude_rad)*cos(probe_radius_ratio)
					+cos(user_latitude_rad)*sin_probe_radius_ratio*cos(ground_wind_from_rad));
			if (probe_lat_rad[i] < SG_EPSILON ) {
				probe_lon_rad[i] = user_latitude_rad; // probe on a pole	
			} else {
				probe_lon_rad[i] = fmod((user_longitude_rad+asin(sin(ground_wind_from_rad)
							*sin_probe_radius_ratio/cos(probe_lat_rad[i]))+SG_PI)
						,SGD_2PI)-SG_PI;
			}
			probe_lat_deg[i]= probe_lat_rad[i] * SG_RADIANS_TO_DEGREES;
			probe_lon_deg[i]= probe_lon_rad[i] * SG_RADIANS_TO_DEGREES;
		}
	
		for (int i = 0; i < sizeof(probe_elev_m)/sizeof(probe_elev_m[0]); i++) {
			double elevation = 0;
			if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(
				SGGeod::fromRad(probe_lon_rad[i],probe_lat_rad[i]), 20000), elevation, 0)) {
				if ( elevation > 0.1 ) { 
					probe_elev_m[i] = elevation; 
				} else { 
					probe_elev_m[i] = 0.1 ;
				}
			} else { 
				probe_elev_m[i] = 0.1;
			}
		}

		// slopes
		double adj_slope[4];
		slope[0] = (probe_elev_m[0] - probe_elev_m[1]) / dist_probe_m[1];
		slope[1] = (probe_elev_m[1] - probe_elev_m[2]) / dist_probe_m[2];
		slope[2] = (probe_elev_m[2] - probe_elev_m[3]) / dist_probe_m[3];
		slope[3] = (probe_elev_m[4] - probe_elev_m[0]) / -dist_probe_m[4];
	
		for (int i = 0; i < sizeof(slope)/sizeof(slope[0]); i++)
			adj_slope[i] = sin(atan(5.0 * pow ( (fabs(slope[i])),1.7) ) ) *sign(slope[i]);
	
		//adjustment
		adj_slope[0] *= 0.2;
		adj_slope[1] *= 0.2;
		if ( adj_slope [2] < 0.0 ) {
			adj_slope[2] *= 0.5;
		} else {
			adj_slope[2] = 0.0 ;
		}
	
		if ( ( adj_slope [0] >= 0.0 ) && ( adj_slope [3] < 0.0 ) ) {
			adj_slope[3] = 0.0;
		} else {
			adj_slope[3] *= 0.2;
		}
		lift_factor = adj_slope[0]+adj_slope[1]+adj_slope[2]+adj_slope[3];
	
		// restart the timer
		timer = 1.0;
	}
	
	//user altitude above ground
	double user_altitude_agl_m = _user_altitude_agl_ft_node->getDoubleValue() * SG_FEET_TO_METER;
	
	//boundaries
	double boundary2_m = 130.0; // in the lift
	if (lift_factor < 0.0) { // in the sink
		double highest_probe_temp= max ( probe_elev_m[1], probe_elev_m[2] );
		double highest_probe_downwind_m= max ( highest_probe_temp, probe_elev_m[3] );
		boundary2_m = highest_probe_downwind_m - probe_elev_m[0];
	}

	double agl_factor;
	if ( user_altitude_agl_m < BOUNDARY1_m ) {
		agl_factor = 0.5+0.5*user_altitude_agl_m /BOUNDARY1_m ;
	} else if ( user_altitude_agl_m < boundary2_m ) {
		agl_factor = 1.0;
	} else {
		agl_factor = exp(-(2 + probe_elev_m[0] / 2000) * 
				(user_altitude_agl_m - boundary2_m) / max(probe_elev_m[0],200.0));
	}
	
	double lift_mps = lift_factor* ground_wind_speed_mps * agl_factor;
	
	//the updraft, finally, in ft per second
	strength = fgGetLowPass( strength, lift_mps * SG_METER_TO_FEET, dt );
	_ridge_lift_fps_node->setDoubleValue( strength );
}
