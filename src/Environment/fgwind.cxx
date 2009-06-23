// fgwind.cxx -- routines to create variable winds
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
#include "fgwind.hxx"
#include <math.h>
#include <stdio.h>

FGWindModulator::FGWindModulator() : 
	direction_offset_norm(0.0),
	magnitude_factor_norm(1.0)
{
}

FGWindModulator::~FGWindModulator()
{
}

FGBasicWindModulator::FGBasicWindModulator() :
	elapsed(0.0),
	direction_period(17),
	speed_period(1)
{
}

FGBasicWindModulator::~FGBasicWindModulator()
{
}

void FGBasicWindModulator::update( double dt)
{
	elapsed += dt;
	double t = elapsed/direction_period;
	
	direction_offset_norm = (sin(t)*sin(2*t)+sin(t/3)) / 1.75;

	t = elapsed/speed_period;
	magnitude_factor_norm = sin(t)* sin(5*direction_offset_norm*direction_offset_norm);;
	magnitude_factor_norm = magnitude_factor_norm < 0 ? 0 : magnitude_factor_norm;
}
