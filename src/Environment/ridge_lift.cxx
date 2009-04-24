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
#include <Scenery/scenery.hxx>
#include <string>
#include <math.h>

using std::string;

#include "ridge_lift.hxx"

//constructor
FGRidgeLift::FGRidgeLift ()
{	
	dist_probe_m[0] = 0.0; // in meters
	dist_probe_m[1] = 250.0;
	dist_probe_m[2] = 750.0;
	dist_probe_m[3] = 2000.0;
	dist_probe_m[4] = -100.0;

	BOUNDARY1_m = 40.0; // in meters
	BOUNDARY2_m = 130.0; 

	PI = 4.0*atan(1.0); // pi
	deg2rad = (PI/180.0);
	rad2deg = (180.0/PI);
	strength = 0.0;
	timer = 0.0;
	scanned = false;

	earth_rad_ft=20899773.07;
        for( int i = 0; i < sizeof(probe_elev_m)/sizeof(probe_elev_m[0]); i++ )
          probe_elev_m[i] = 0.0;
}

//destructor
FGRidgeLift::~FGRidgeLift()
{

}

void FGRidgeLift::init(void)
{
	_ridge_lift_fps_node =
			fgGetNode("/environment/ridge-lift-fps"
			, true);
	_surface_wind_from_deg_node =
			fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg"
			, true);
	_surface_wind_speed_node =
			fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt"
			, true);
	_earth_radius_node =
			fgGetNode("/position/sea-level-radius-ft"
			, true);
	_user_longitude_node =
			fgGetNode("/position/longitude-deg"
			, true);
	_user_latitude_node =
			fgGetNode("/position/latitude-deg"
			, true);
	_user_altitude_ft_node =
			fgGetNode("/position/altitude-ft"
			, true);
	_user_altitude_agl_ft_node =
			fgGetNode("/position/altitude-agl-ft"
			, true);
}

void FGRidgeLift::bind() {

	fgTie("/environment/ridge-lift/probe-elev-m[0]", this,
		&FGRidgeLift::get_probe_elev_m_0); // read-only
	fgTie("/environment/ridge-lift/probe-elev-m[1]", this,
		&FGRidgeLift::get_probe_elev_m_1); // read-only
	fgTie("/environment/ridge-lift/probe-elev-m[2]", this,
		&FGRidgeLift::get_probe_elev_m_2); // read-only
	fgTie("/environment/ridge-lift/probe-elev-m[3]", this,
		&FGRidgeLift::get_probe_elev_m_3); // read-only
	fgTie("/environment/ridge-lift/probe-elev-m[4]", this,
		&FGRidgeLift::get_probe_elev_m_4); // read-only

	fgTie("/environment/ridge-lift/probe-lat-deg[0]", this,
		&FGRidgeLift::get_probe_lat_0); // read-only
	fgTie("/environment/ridge-lift/probe-lat-deg[1]", this,
		&FGRidgeLift::get_probe_lat_1); // read-only
	fgTie("/environment/ridge-lift/probe-lat-deg[2]", this,
		&FGRidgeLift::get_probe_lat_2); // read-only
	fgTie("/environment/ridge-lift/probe-lat-deg[3]", this,
		&FGRidgeLift::get_probe_lat_3); // read-only
	fgTie("/environment/ridge-lift/probe-lat-deg[4]", this,
		&FGRidgeLift::get_probe_lat_4); // read-only

	fgTie("/environment/ridge-lift/probe-lon-deg[0]", this,
		&FGRidgeLift::get_probe_lon_0); // read-only
	fgTie("/environment/ridge-lift/probe-lon-deg[1]", this,
		&FGRidgeLift::get_probe_lon_1); // read-only
	fgTie("/environment/ridge-lift/probe-lon-deg[2]", this,
		&FGRidgeLift::get_probe_lon_2); // read-only
	fgTie("/environment/ridge-lift/probe-lon-deg[3]", this,
		&FGRidgeLift::get_probe_lon_3); // read-only
	fgTie("/environment/ridge-lift/probe-lon-deg[4]", this,
		&FGRidgeLift::get_probe_lon_4); // read-only

	fgTie("/environment/ridge-lift/slope[0]", this,
		&FGRidgeLift::get_slope_0); // read-only
	fgTie("/environment/ridge-lift/slope[1]", this,
		&FGRidgeLift::get_slope_1); // read-only
	fgTie("/environment/ridge-lift/slope[2]", this,
		&FGRidgeLift::get_slope_2); // read-only
	fgTie("/environment/ridge-lift/slope[3]", this,
		&FGRidgeLift::get_slope_3); // read-only
	
}

void FGRidgeLift::unbind() {

	fgUntie("/environment/ridge-lift/probe-elev-m[0]");
	fgUntie("/environment/ridge-lift/probe-elev-m[1]");
	fgUntie("/environment/ridge-lift/probe-elev-m[2]");
	fgUntie("/environment/ridge-lift/probe-elev-m[3]");
	fgUntie("/environment/ridge-lift/probe-elev-m[4]");

	fgUntie("/environment/ridge-lift/probe-lat-deg[0]");
	fgUntie("/environment/ridge-lift/probe-lat-deg[1]");
	fgUntie("/environment/ridge-lift/probe-lat-deg[2]");
	fgUntie("/environment/ridge-lift/probe-lat-deg[3]");
	fgUntie("/environment/ridge-lift/probe-lat-deg[4]");

	fgUntie("/environment/ridge-lift/probe-lon-deg[0]");
	fgUntie("/environment/ridge-lift/probe-lon-deg[1]");
	fgUntie("/environment/ridge-lift/probe-lon-deg[2]");
	fgUntie("/environment/ridge-lift/probe-lon-deg[3]");
	fgUntie("/environment/ridge-lift/probe-lon-deg[4]");

	fgUntie("/environment/ridge-lift/slope[0]");
	fgUntie("/environment/ridge-lift/slope[1]");
	fgUntie("/environment/ridge-lift/slope[2]");
	fgUntie("/environment/ridge-lift/slope[3]");
	
}

void FGRidgeLift::update(double dt) {
	Run(dt);
}

double FGRidgeLift::sign(double x) {
    if (x == 0.0)
        return x;
    else
        return x/fabs(x);
}

void FGRidgeLift::Run(double dt) {

	// copy values 

	user_latitude_deg  = _user_latitude_node->getDoubleValue();
	user_longitude_deg = _user_longitude_node->getDoubleValue();
	//user_altitude_ft  = _user_altitude_ft_node->getDoubleValue();
	
	if ( ( _earth_radius_node->getDoubleValue() ) > 1.0 ) {
	earth_rad_ft =_earth_radius_node->getDoubleValue(); }
	else {	earth_rad_ft=20899773.07; }

	//earth_rad_m = earth_rad_ft * 0.3048 ;
	earth_rad_m = earth_rad_ft * SG_FEET_TO_METER ;
	
	//get the windspeed at ground level

	double ground_wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
	double ground_wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();
	//double ground_wind_speed_mps = ground_wind_speed_kts / SG_METER_TO_FEET;
	double ground_wind_speed_mps = ground_wind_speed_kts / 3.2808399;

	//double ground_wind_from_rad = (user_longitude_deg < 0.0)  ? 
	//	PI*( ground_wind_from_deg/180.0) +PI : PI*( ground_wind_from_deg/180.0);
	double ground_wind_from_rad = PI*( ground_wind_from_deg/180.0);

	// Placing the probes
	
	for (int i = 0; i <= 4; i++)
	{
			probe_lat_rad[i] = asin(sin(deg2rad*user_latitude_deg)*cos(dist_probe_m[i]/earth_rad_m)
				+cos(deg2rad*user_latitude_deg)*sin(dist_probe_m[i]/earth_rad_m)*cos(ground_wind_from_rad));
		if (probe_lat_rad[i] == 0.0) {
			probe_lon_rad[i] = (deg2rad*user_latitude_deg);      // probe on a pole	
		}
		else {
			probe_lon_rad[i] = fmod((deg2rad*user_longitude_deg+asin(sin(ground_wind_from_rad)
						*sin(dist_probe_m[i]/earth_rad_m)/cos(probe_lat_rad[i]))+PI)
					,(2.0*PI))-PI;
		}
		probe_lat_deg[i]= rad2deg*probe_lat_rad[i];
		probe_lon_deg[i]= rad2deg*probe_lon_rad[i];
	}
	
	// ground elevations
	// every second
	
	timer -= dt;
	if (timer <= 0.0 ) {
		for (int i = 0; i <= 4; i++)
		{
			if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(
			    SGGeod::fromRad(probe_lon_rad[i],probe_lat_rad[i]), 20000), alt, 0))
			{
				if ( alt > 0.1 ) { probe_elev_m[i] =  alt; } else { probe_elev_m[i] = 0.1 ;};
			}
			else { probe_elev_m[i] = 0.1;};
		}
		timer = 1.0;
		
	}

	// slopes
	
	double adj_slope[5];
	
	slope[0] = (probe_elev_m[0] - probe_elev_m[1]) / dist_probe_m[1];
	slope[1] = (probe_elev_m[1] - probe_elev_m[2]) / dist_probe_m[2];
	slope[2] = (probe_elev_m[2] - probe_elev_m[3]) / dist_probe_m[3];
	slope[3] = (probe_elev_m[4] - probe_elev_m[0]) / -dist_probe_m[4];
	
	for (int i = 0; i <= 4; i++)
	{	
		adj_slope[i] = sin(atan(5.0 * pow ( (fabs(slope[i])),1.7) ) ) *sign(slope[i]);
	}
	
	//adjustment
	
	adj_slope[0] = 0.2 * adj_slope[0];
	adj_slope[1] = 0.2 * adj_slope[1];
	if ( adj_slope [2] < 0.0 )
	{
		adj_slope[2] = 0.5 * adj_slope[2];
	}
	else 
	{
		adj_slope[2] = 0.0 ;
	}
	
	if ( ( adj_slope [0] >= 0.0 ) && ( adj_slope [3] < 0.0 ) )
	{
		adj_slope[3] = 0.0;
	}
	else 
	{
		adj_slope[3] = 0.2 * adj_slope[3];
	}
	
	double lift_factor = adj_slope[0]+adj_slope[1]+adj_slope[2]+adj_slope[3];
	
	//user altitude above ground
	
	user_altitude_agl_ft = _user_altitude_agl_ft_node->getDoubleValue();
	user_altitude_agl_m = ( user_altitude_agl_ft / SG_METER_TO_FEET );
	
	//boundaries
	double agl_factor;

	if (lift_factor < 0.0) // in the sink
	{
		double highest_probe_temp= max ( probe_elev_m[1], probe_elev_m[2] );
		double highest_probe_downwind_m= max ( highest_probe_temp, probe_elev_m[3] );
		BOUNDARY2_m = highest_probe_downwind_m - probe_elev_m[0];
	}
	else // in the lift
	{
		BOUNDARY2_m = 130.0;
	}

	if ( user_altitude_agl_m < BOUNDARY1_m )
	{
		agl_factor = 0.5+0.5*user_altitude_agl_m /BOUNDARY1_m ;
	}
	else if ( user_altitude_agl_m < BOUNDARY2_m )
	{
		agl_factor = 1.0;
	}
	else
	{
		agl_factor =  exp(-(2 + 2 * probe_elev_m[0] / 4000) * 
				(user_altitude_agl_m - BOUNDARY2_m) / max(probe_elev_m[0],200.0));
	}
	
	double lift_mps = lift_factor* ground_wind_speed_mps * agl_factor;
	
	//the updraft, finally, in ft per second
	strength = lift_mps * SG_METER_TO_FEET ;
//  	if(isnan(strength)) strength=0; 
 	 _ridge_lift_fps_node->setDoubleValue( strength );
}






