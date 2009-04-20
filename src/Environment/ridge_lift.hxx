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


#ifndef _FG_RidgeLift_HXX
#define _FG_RidgeLift_HXX

#ifdef HAVE_CONFIG
#  include <config.h>
#endif


#include <string>
using std::string;


class FGRidgeLift : public SGSubsystem {
public:

	FGRidgeLift();
	~FGRidgeLift();		
	
	virtual void bind();
	virtual void unbind();
	virtual void update(double dt);
	virtual void init();

	inline double getStrength() const { return strength; };

 	inline double get_probe_elev_m_0() const { return probe_elev_m[0]; };
 	inline double get_probe_elev_m_1() const { return probe_elev_m[1]; };
 	inline double get_probe_elev_m_2() const { return probe_elev_m[2]; };
 	inline double get_probe_elev_m_3() const { return probe_elev_m[3]; };
 	inline double get_probe_elev_m_4() const { return probe_elev_m[4]; };

 	inline double get_probe_lat_0() const { return probe_lat_deg[0]; };
 	inline double get_probe_lat_1() const { return probe_lat_deg[1]; };
 	inline double get_probe_lat_2() const { return probe_lat_deg[2]; };
 	inline double get_probe_lat_3() const { return probe_lat_deg[3]; };
 	inline double get_probe_lat_4() const { return probe_lat_deg[4]; };

	inline double get_probe_lon_0() const { return probe_lon_deg[0]; };
 	inline double get_probe_lon_1() const { return probe_lon_deg[1]; };
 	inline double get_probe_lon_2() const { return probe_lon_deg[2]; };
 	inline double get_probe_lon_3() const { return probe_lon_deg[3]; };
 	inline double get_probe_lon_4() const { return probe_lon_deg[4]; };

 	inline double get_slope_0() const { return slope[0]; };
 	inline double get_slope_1() const { return slope[1]; };
 	inline double get_slope_2() const { return slope[2]; };
 	inline double get_slope_3() const { return slope[3]; };

private:
	//void init();
	void Run(double dt);

	double dist_probe_m[5];
	double BOUNDARY1_m;
	double BOUNDARY2_m;
	double PI; // pi
	double deg2rad;
	double rad2deg;

	bool scanned;

	double strength;
	double timer;

	double probe_lat_rad[5];
	double probe_lon_rad[5];

	double probe_lat_deg[5];
	double probe_lon_deg[5];

	double alt;	
	double probe_elev_m[5];

	double slope[4];
	double earth_rad_ft;
	double earth_rad_m;
	double user_latitude_deg;
	double user_longitude_deg;
	//double user_altitude;
	double user_altitude_agl_ft;
	double user_altitude_agl_m;

	double sign(double x);

	SGPropertyNode_ptr _ridge_lift_fps_node;

	SGPropertyNode_ptr _surface_wind_from_deg_node;
	SGPropertyNode_ptr _surface_wind_speed_node;

	SGPropertyNode_ptr _user_altitude_ft_node;
	SGPropertyNode_ptr _user_altitude_agl_ft_node;
	SGPropertyNode_ptr _earth_radius_node;
	SGPropertyNode_ptr _user_longitude_node;
	SGPropertyNode_ptr _user_latitude_node;

};

#endif  // _FG_RidgeLift_HXX