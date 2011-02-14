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

#include <simgear/props/tiedpropertylist.hxx>

class FGRidgeLift : public SGSubsystem {
public:

	FGRidgeLift();
	~FGRidgeLift();		
	
	virtual void bind();
	virtual void unbind();
	virtual void update(double dt);
	virtual void init();

	inline double getStrength() const { return strength; };

 	inline double get_probe_elev_m( int index ) const { return probe_elev_m[index]; };
 	inline double get_probe_lat_deg( int index ) const { return probe_lat_deg[index]; };
	inline double get_probe_lon_deg( int index ) const { return probe_lon_deg[index]; };
 	inline double get_slope( int index ) const { return slope[index]; };

private:
	static const double dist_probe_m[5];

	double strength;
	double timer;

	double probe_lat_deg[5];
	double probe_lon_deg[5];
	double probe_elev_m[5];

	double slope[4];

	double lift_factor;

	SGPropertyNode_ptr _enabled_node;
	SGPropertyNode_ptr _ridge_lift_fps_node;

	SGPropertyNode_ptr _surface_wind_from_deg_node;
	SGPropertyNode_ptr _surface_wind_speed_node;

	SGPropertyNode_ptr _user_altitude_agl_ft_node;
	SGPropertyNode_ptr _user_longitude_node;
	SGPropertyNode_ptr _user_latitude_node;
	SGPropertyNode_ptr _ground_elev_node;

	simgear::TiedPropertyList _tiedProperties;
};

#endif  // _FG_RidgeLift_HXX
