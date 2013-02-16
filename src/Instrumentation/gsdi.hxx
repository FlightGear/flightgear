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

#ifndef __INSTRUMENTS_GSDI_HXX
#define __INSTRUMENTS_GSDI_HXX 1

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Input properties:
 *
 * /instrumentation/gsdi/serviceable
 * /orientation/heading-deg
 * /velocities/uBody-fps
 * /velocities/vBody-fps
 * /environment/wind-from-heading-deg
 * /environment/wind-speed-kt
 *
 * Output properties:
 *
 * /instrumentation/gsdi/drift-u-kt
 * /instrumentation/gsdi/drift-v-kt
 * /instrumentation/gsdi/drift-speed-kt
 * /instrumentation/gsdi/drift-angle-deg
 */
class GSDI : public SGSubsystem
{
public:
	GSDI(SGPropertyNode *node);
	virtual ~GSDI();

	virtual void init();
	virtual void update(double dt);

private:
  std::string _name;
	unsigned int _num;

	SGPropertyNode_ptr _serviceableN;
	SGPropertyNode_ptr _headingN;
	SGPropertyNode_ptr _ubodyN;
	SGPropertyNode_ptr _vbodyN;
	SGPropertyNode_ptr _wind_speedN;
	SGPropertyNode_ptr _wind_dirN;
	SGPropertyNode_ptr _drift_uN;
	SGPropertyNode_ptr _drift_vN;
	SGPropertyNode_ptr _drift_speedN;
	SGPropertyNode_ptr _drift_angleN;
};

#endif // _INSTRUMENTS_GSDI_HXX
