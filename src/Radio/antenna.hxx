// antenna.hxx -- FGRadioAntenna: class to represent antenna properties
//
// Written by Adrian Musceac, started December 2011.
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

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <Main/fg_props.hxx>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>

class FGRadioAntenna
{
private:
	void _load_antenna_pattern();
	int _mirror_y;
	int _mirror_z;
	double _heading;
	struct AntennaGain {
		double azimuth;
		double elevation_angle;
		double gain;
	};
	
	typedef std::vector<AntennaGain> AntennaPattern;
	AntennaPattern _pattern;
	
public:
	
	FGRadioAntenna();
    ~FGRadioAntenna();
	double calculate_gain(double azimuth, double theta);
	
	
};
