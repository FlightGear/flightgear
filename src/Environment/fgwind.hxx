// fgwind.hxx -- routines to create variable winds
//
// Written by Torsten Dreyer
//
// Copyright (C) 2009 Torsten Dreyer - Torsten _at_ t3r*de
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
// $Id$
#ifndef _FGWIND_HXX
#define _FGWIND_HXX

#include <simgear/structure/SGReferenced.hxx>

////////////////////////////////////////////////////////////////////////
// A Wind Modulator interface, generates gusts and wind direction changes
////////////////////////////////////////////////////////////////////////
class FGWindModulator : public SGReferenced {
public:
	FGWindModulator();
	virtual ~FGWindModulator();
	virtual void update( double dt ) = 0;
	double get_direction_offset_norm() const { return direction_offset_norm; }
	double get_magnitude_factor_norm() const { return magnitude_factor_norm; }
protected:
	double direction_offset_norm;
	double magnitude_factor_norm;
};


////////////////////////////////////////////////////////////////////////
// A Basic Wind Modulator, implementation of FGWindModulator
// direction and magnitude variations are based on simple sin functions
////////////////////////////////////////////////////////////////////////
class FGBasicWindModulator : public FGWindModulator {
public:
	FGBasicWindModulator();
	virtual ~FGBasicWindModulator();
	virtual void update( double dt );
	void set_direction_period( double _direction_period ) { direction_period = _direction_period; }
	double get_direction_period() const { return direction_period; }
	void set_speed_period( double _speed_period ) { speed_period = _speed_period; }
	double get_speed_period() const{ return speed_period; }
private:
	double elapsed;
	double direction_period;
	double speed_period;
};

#endif
