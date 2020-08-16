// FGAIBaseAircraft - abstract base class for AI aircraft
// Written by Stuart Buchanan, started August 2002
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

#include "AIBaseAircraft.hxx"
#include <src/Main/globals.hxx>

FGAIBaseAircraft::FGAIBaseAircraft() : FGAIBase(otStatic, false)
{
    m_gearPos = 0.0f;
    m_flapsPos = 0.0f;
    m_spoilerPos = 0.0f;
    m_speedbrakePos = 0.0f;

    // Light properties.
    m_beaconLight = false;
    m_cabinLight = false;
    m_landingLight = false;
    m_navLight = false;
    m_strobeLight = false;
    m_taxiLight = false;
}

void FGAIBaseAircraft::bind() {
    FGAIBase::bind();

    // All gear positions are linked for simplicity
    tie("gear/gear[0]/position-norm", SGRawValuePointer<double>(&m_gearPos));
    tie("gear/gear[1]/position-norm", SGRawValuePointer<double>(&m_gearPos));
    tie("gear/gear[2]/position-norm", SGRawValuePointer<double>(&m_gearPos));
    tie("gear/gear[3]/position-norm", SGRawValuePointer<double>(&m_gearPos));
    tie("gear/gear[4]/position-norm", SGRawValuePointer<double>(&m_gearPos));
    tie("gear/gear[5]/position-norm", SGRawValuePointer<double>(&m_gearPos));

    tie("surface-positions/flap-pos-norm",
        SGRawValueMethods<FGAIBaseAircraft,double>(*this,
        &FGAIBaseAircraft::getFlapsPos,
        &FGAIBaseAircraft::setFlapsPos));

    tie("surface-positions/spoiler-pos-norm",
        SGRawValueMethods<FGAIBaseAircraft,double>(*this,
        &FGAIBaseAircraft::getSpoilerPos,
        &FGAIBaseAircraft::setSpoilerPos));

    tie("surface-positions/speedbrake-pos-norm",
        SGRawValueMethods<FGAIBaseAircraft,double>(*this,
        &FGAIBaseAircraft::getSpeedBrakePos,
        &FGAIBaseAircraft::setSpeedBrakePos));

    tie("controls/lighting/beacon",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getBeaconLight,
        &FGAIBaseAircraft::setBeaconLight));

    tie("controls/lighting/cabin-lights",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getCabinLight,
        &FGAIBaseAircraft::setCabinLight));

    tie("controls/lighting/landing-lights",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getLandingLight,
        &FGAIBaseAircraft::setLandingLight));

    tie("controls/lighting/nav-lights",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getNavLight,
        &FGAIBaseAircraft::setNavLight));

    tie("controls/lighting/strobe",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getStrobeLight,
        &FGAIBaseAircraft::setStrobeLight));

    tie("controls/lighting/taxi-lights",
        SGRawValueMethods<FGAIBaseAircraft,bool>(*this,
        &FGAIBaseAircraft::getTaxiLight,
        &FGAIBaseAircraft::setTaxiLight));

}
