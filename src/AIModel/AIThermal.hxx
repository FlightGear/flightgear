// FGAIThermal - FGAIBase-derived class creates an AI thermal
//
// Original by Written by David Culp
//
// An attempt to refine the thermal shape and behaviour by WooT 2009
//
// Copyright (C) 2009 Patrice Poly ( WooT )
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

#ifndef _FG_AIThermal_HXX
#define _FG_AIThermal_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"

#include <string>
using std::string;


class FGAIThermal : public FGAIBase {

public:
	FGAIThermal();
	~FGAIThermal();

	void readFromScenario(SGPropertyNode* scFileNode);

	virtual bool init(bool search_in_AI_path=false);
	virtual void bind();
	virtual void update(double dt);

	inline void setMaxStrength( double s ) { max_strength = s; };
	inline void setDiameter( double d ) { diameter = d; };
	inline void setHeight( double h ) { height = h; };
	inline void setMaxUpdraft( double lift ) { v_up_max = lift; };
	inline void setMinUpdraft( double sink ) { v_up_min = sink; };
	inline void setR_up_frac( double r ) { r_up_frac = r; };

	inline double getStrength() const { return strength; };
	inline double getDiameter() const { return diameter; };
	inline double getHeight() const { return height; };
	inline double getV_up_max() const { return v_up_max; };
	inline double getV_up_min() const { return v_up_min; };
	inline double getR_up_frac() const { return r_up_frac; };

	virtual const char* getTypeString(void) const { return "thermal"; }
	void getGroundElev(double dt);

private:
	void Run(double dt);
	double get_strength_fac(double alt_frac);
	double max_strength;
	double strength;
	double diameter;
	double height;
	double factor;
	double alt_rel;
	double alt;
	double v_up_max;
	double v_up_min;
	double r_up_frac;
	double cycle_timer;
	double dt_count;
	double time;
	double xx;
	double ground_elev_ft; // ground level in ft
	double altitude_agl_ft; // altitude above ground in feet
	bool do_agl_calc;
	bool is_forming;
	bool is_formed;
	bool is_dying;
	bool is_dead;
	SGPropertyNode_ptr _surface_wind_from_deg_node;
	SGPropertyNode_ptr _surface_wind_speed_node;
	SGPropertyNode_ptr _aloft_wind_from_deg_node;
	SGPropertyNode_ptr _aloft_wind_speed_node;

};

#endif  // _FG_AIThermal_HXX
