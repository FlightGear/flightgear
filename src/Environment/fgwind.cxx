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
