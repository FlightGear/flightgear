// inst_vertical_speed_indicator.cxx
// -- Instantaneous VSI (emulation calibrated to standard atmosphere).
//
// Started September 2004.
//
// Copyright (C) 2004
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <limits>
#include <simgear/math/interpolater.hxx>

#include "inst_vertical_speed_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


// Altitude based on pressure difference from sea level.
//
// See http://www.pdas.com/programs/atmos.f90, the tables match exactly
// environment.cxx (Standard 1976).
// Example :
// - at 27900 m the level is 20 km :
//   geopotential altitude 27.9 x 6369 / ( 27.9 + 6369) = 27.778 km.
// - deltah = 27.778 - 20 = 7.778 km and tbase 216.65 degK.
// - delta = 5.403295E-2 x exp( -34.163195 x 7.778 / 216.65 ) = 0.016.
// - pressure = (1 - delta ) x 29.92 = 29.44 inhg.
// - to construct the tables, round delta to 3 digits.

// altitude ft, pressure step inHG
static double pressure_data[][2] = {
 { 0.00,     3.33 },	// guess !
 { 2952.76,  3.05 },
 { 5905.51,  2.81 },
 { 8858.27,  2.55 },
 { 11811.02, 2.33 },
 { 14763.78, 2.13 },
 { 17716.54, 1.91 },
 { 20669.29, 1.77 },
 { 23622.05, 1.58 },
 { 26574.80, 1.49 },
 { 29527.56, 1.20 },
 { 32480.31, 1.14 },
 { 35433.07, 1.05 },
 { 38385.83, 0.90 },
 { 41338.58, 0.80 },
 { 44291.34, 0.69 },
 { 47244.09, 0.60 },
 { 50196.85, 0.51 },
 { 53149.61, 0.45 },
 { 56102.36, 0.39 },
 { 59055.12, 0.33 },
 { 62007.87, 0.30 },
 { 64960.63, 0.26 },
 { 67913.39, 0.21 },
 { 70866.14, 0.18 },
 { 73818.90, 0.18 },
 { 76771.65, 0.15 },
 { 79724.41, 0.12 },
 { 82677.17, 0.12 },
 { 85629.92, 0.09 },
 { 88582.68, 0.09 },
 { 91535.43, 0.06 },
 { 94488.19, 0.06 },
 { 97440.94, 0.06 },
 { 100393.70, 0.06 },
 { -1, -1 }
};

// pressure difference inHG, altitude ft
static double altitude_data[][2] = {
 {  0.00, 0.00 },
 {  3.05, 2952.76 },
 {  5.86, 5905.51 },
 {  8.41, 8858.27 },
 { 10.74, 11811.02 },
 { 12.87, 14763.78 },
 { 14.78, 17716.54 },
 { 16.55, 20669.29 },
 { 18.13, 23622.05 },
 { 19.62, 26574.80 },
 { 20.82, 29527.56 },
 { 21.96, 32480.31 },
 { 23.01, 35433.07 },
 { 23.91, 38385.83 },
 { 24.71, 41338.58 },
 { 25.40, 44291.34 },
 { 26.00, 47244.09 },
 { 26.51, 50196.85 },
 { 26.96, 53149.61 },
 { 27.35, 56102.36 },
 { 27.68, 59055.12 },
 { 27.98, 62007.87 },
 { 28.24, 64960.63 },
 { 28.45, 67913.39 },
 { 28.63, 70866.14 },
 { 28.81, 73818.90 },
 { 28.96, 76771.65 },
 { 29.08, 79724.41 },
 { 29.20, 82677.17 },
 { 29.29, 85629.92 },
 { 29.38, 88582.68 },
 { 29.44, 91535.43 },
 { 29.50, 94488.19 },
 { 29.56, 97440.94 },
 { 29.62, 100393.70 },
 { -1, -1 }
};


// SI constants
#define SEA_LEVEL_INHG 29.92

// A higher number means more responsive.
#define RESPONSIVENESS 5.0

// External environment
#define MAX_INHG_PER_S 0.0002


InstVerticalSpeedIndicator::InstVerticalSpeedIndicator ( SGPropertyNode *node ) :
                            _name(node->getStringValue("name", "inst-vertical-speed-indicator")),
                            _num(node->getIntValue("number", 0)),
                            _internal_pressure_inhg( SEA_LEVEL_INHG ),
                            _internal_sea_inhg( SEA_LEVEL_INHG ),
                            _speed_ft_per_s( 0 ),
                            _pressure_table(new SGInterpTable),
                            _altitude_table(new SGInterpTable)
{
    int i;
    for ( i = 0; pressure_data[i][0] != -1; i++)
        _pressure_table->addEntry( pressure_data[i][0], pressure_data[i][1] );

    for ( i = 0; altitude_data[i][0] != -1; i++)
        _altitude_table->addEntry( altitude_data[i][0], altitude_data[i][1] );
}


InstVerticalSpeedIndicator::~InstVerticalSpeedIndicator ()
{
    delete _pressure_table;
    delete _altitude_table;
}


void InstVerticalSpeedIndicator::init ()
{
    SGPropertyNode *node = fgGetNode("/instrumentation", true)->getChild(_name, _num, true);

    _serviceable_node =
        node->getNode("serviceable", true);
    _freeze_node =
        fgGetNode("/sim/freeze/master", true);

    // A real IVSI is operated by static pressure changes.
    // It operates like a conventional VSI, except that an internal sensor
    // detects load factors, to momentarily alters the static pressure
    // (with lag).
    // It appears lag free at subsonic speed; at high altitude indication may
    // be less than 1/3 of actual conditions.
    _pressure_node =
        fgGetNode("/environment/pressure-inhg", true);
    _sea_node =
        fgGetNode("/environment/pressure-sea-level-inhg", true);
    _speed_up_node =
        fgGetNode("/sim/speed-up", true);
    _speed_node =
        node->getNode("indicated-speed-fps", true);
    _speed_min_node =
        node->getNode("indicated-speed-fpm", true);

    // Initialize at ambient pressure
    _internal_pressure_inhg = _pressure_node->getDoubleValue();
}


void InstVerticalSpeedIndicator::update (double dt)
{
    if (_serviceable_node->getBoolValue())
    {
	// avoids hang, when freeze
        if( !_freeze_node->getBoolValue() && std::numeric_limits<double>::min() < fabs(dt))
	{
	    double pressure_inhg = _pressure_node->getDoubleValue();
	    double sea_inhg = _sea_node->getDoubleValue();
	    double speed_up = _speed_up_node->getDoubleValue();
	    
	    // must work by speed up
	    if( speed_up > 1 )
	    {
	        dt *= speed_up;
	    }
	      
	    // limit effect of external environment
	    double rate_sea_inhg_per_s = ( sea_inhg - _internal_sea_inhg ) / dt;
	    
	    if( rate_sea_inhg_per_s > - MAX_INHG_PER_S && rate_sea_inhg_per_s < MAX_INHG_PER_S )
	    {
	       double rate_inhg_per_s = ( pressure_inhg - _internal_pressure_inhg ) / dt;

	       // IVSI determines alone the current altitude, without altimeter setting.
	       // Altimeter setting is 29.92 above 10000 or 18000 ft.
	       // Below this level, the slope is slightly wrong.
	       double altitude_ft = _altitude_table->interpolate( SEA_LEVEL_INHG - pressure_inhg );
	       double slope_inhg = _pressure_table->interpolate( altitude_ft );


	       double last_speed_ft_per_s = _speed_ft_per_s;

	       // slope at 900 m
	       _speed_ft_per_s = - rate_inhg_per_s * 2952.75591 / slope_inhg;

	       // filter noise
	       _speed_ft_per_s = fgGetLowPass( last_speed_ft_per_s, _speed_ft_per_s,
		  			       dt * RESPONSIVENESS );
            }
	    
	    _speed_node->setDoubleValue( _speed_ft_per_s );
	    _speed_min_node->setDoubleValue( _speed_ft_per_s * 60.0 );

	    // backup
	    _internal_pressure_inhg = pressure_inhg;
	    _internal_sea_inhg = sea_inhg;
	}
    }
}

// end of inst_vertical_speed_indicator.cxx
