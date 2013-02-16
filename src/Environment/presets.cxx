// presets.cxx -- Wrap environment presets
//
// Written by Torsten Dreyer, January 2011
//
// Copyright (C) 2010  Torsten Dreyer Torsten(at)t3r(dot)de
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
#  include <config.h>
#endif
#include "presets.hxx"

#include <cmath>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/SGMath.hxx>
#include <Main/fg_props.hxx>

namespace Environment {
namespace Presets {

PresetBase::PresetBase( const char * overrideNodePath )
    : _overrideNodePath( overrideNodePath ) 
{
}

void PresetBase::setOverride( bool value ) 
{ 
    /*
    Don't initialize node in constructor because the class is used as a singleton
    and created as a static variable in the initialization sequence when globals()
    is not yet initialized and returns null.
    */
    if( _overrideNode == NULL ) 
        _overrideNode = fgGetNode( _overrideNodePath.c_str(), true );
    _overrideNode->setBoolValue( value ); 
}


Wind::Wind() : 
    PresetBase("/environment/config/presets/wind-override")
{
}


void Wind::preset( double min_hdg, double max_hdg, double speed_kt, double gust_kt )
{
    // see: PresetBase::setOverride()

    //TODO: handle variable wind and gusts
    if( _fromNorthNode == NULL )
        _fromNorthNode = fgGetNode("/environment/config/presets/wind-from-north-fps", true );
    
    if( _fromEastNode == NULL )
        _fromEastNode = fgGetNode("/environment/config/presets/wind-from-east-fps", true );

    double avgHeading_rad = 
      SGMiscd::normalizeAngle2(
        (SGMiscd::normalizeAngle(min_hdg*SG_DEGREES_TO_RADIANS) + 
         SGMiscd::normalizeAngle(max_hdg*SG_DEGREES_TO_RADIANS))/2);

    double speed_fps = speed_kt * SG_NM_TO_METER * SG_METER_TO_FEET / 3600.0;
    _fromNorthNode->setDoubleValue( speed_fps * cos(avgHeading_rad) );
    _fromEastNode->setDoubleValue( speed_fps * sin(avgHeading_rad) );
    setOverride( true );
}

Visibility::Visibility() : 
    PresetBase("/environment/config/presets/visibility-m-override")
{
}

void Visibility::preset( double visibility_m )
{
    // see: PresetBase::setOverride()
    if( _visibilityNode == NULL )
        _visibilityNode = fgGetNode("/environment/config/presets/visibility-m", true );

    _visibilityNode->setDoubleValue(visibility_m );
    setOverride( true );
}

Turbulence::Turbulence() : 
    PresetBase("/environment/config/presets/turbulence-magnitude-norm-override")
{
}


void Turbulence::preset(double magnitude_norm)
{
    // see: PresetBase::setOverride()
    if( _magnitudeNode == NULL )
        _magnitudeNode = fgGetNode("/environment/config/presets/turbulence-magnitude-norm", true );

    _magnitudeNode->setDoubleValue( magnitude_norm );
    setOverride( true );
}

Ceiling::Ceiling() : 
    PresetBase("/environment/config/presets/ceiling-override")
{
}


void Ceiling::preset( double elevation, double thickness )
{
    // see: PresetBase::setOverride()
    if( _elevationNode == NULL )
        _elevationNode = fgGetNode("/environment/config/presets/ceiling-elevation-ft", true);

    if( _thicknessNode == NULL )
        _thicknessNode = fgGetNode("/environment/config/presets/ceiling-elevation-ft", true);

    _elevationNode->setDoubleValue( elevation );
    _thicknessNode->setDoubleValue( thickness );
    setOverride( true );
}

} // namespace Presets
} // namespace Environment

