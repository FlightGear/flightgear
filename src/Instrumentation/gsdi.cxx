// gsdi.cxx - Ground Speed Drift Angle Indicator (known as GSDI or GSDA)
// Written by Melchior FRANZ, started 2006.
//
// Copyright (C) 2006  Melchior FRANZ - mfranz#aon:at
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/sg_inlines.h>
#include <simgear/constants.h>

#include <Main/fg_props.hxx>
#include "gsdi.hxx"


/*
 * Failures or inaccuracies are currently not modeled due to lack of data.
 * The Doppler based GSDI should output unreliable data with increasing
 * pitch, roll, vertical acceleration and altitude-agl.
 */


GSDI::GSDI(SGPropertyNode *node) :
	_name(node->getStringValue("name", "gsdi")),
	_num(node->getIntValue("number", 0))
{
}


GSDI::~GSDI()
{
}


void GSDI::init()
{
  std::string branch;
	branch = "/instrumentation/" + _name;
	SGPropertyNode *n = fgGetNode(branch.c_str(), _num, true);
	_serviceableN = n->getNode("serviceable", true);

	// input
	_headingN = fgGetNode("/orientation/heading-deg", true);
	_ubodyN = fgGetNode("/velocities/uBody-fps", true);
	_vbodyN = fgGetNode("/velocities/vBody-fps", true);
	_wind_dirN = fgGetNode("/environment/wind-from-heading-deg", true);
	_wind_speedN = fgGetNode("/environment/wind-speed-kt", true);

	// output
	_drift_uN = n->getNode("drift-u-kt", true);
	_drift_vN = n->getNode("drift-v-kt", true);
	_drift_speedN = n->getNode("drift-speed-kt", true);
	_drift_angleN = n->getNode("drift-angle-deg", true);
}


void GSDI::update(double /*delta_time_sec*/)
{
	if (!_serviceableN->getBoolValue())
		return;

	double wind_speed = _wind_speedN->getDoubleValue();
	double rel_wind_dir = (_headingN->getDoubleValue() - _wind_dirN->getDoubleValue())
			* SGD_DEGREES_TO_RADIANS;
	double wind_u = wind_speed * cos(rel_wind_dir);
	double wind_v = wind_speed * sin(rel_wind_dir);

	double u = _ubodyN->getDoubleValue() * SG_FPS_TO_KT - wind_u;
	double v = _vbodyN->getDoubleValue() * SG_FPS_TO_KT + wind_v;

	double speed = sqrt(u * u + v * v);
	double angle = atan2(v, u) * SGD_RADIANS_TO_DEGREES;

	_drift_uN->setDoubleValue(u);
	_drift_vN->setDoubleValue(v);
	_drift_speedN->setDoubleValue(speed);
	_drift_angleN->setDoubleValue(angle);
}

// end of gsdi.cxx
