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

#ifndef _FG_AIBaseAircraft_HXX
#define _FG_AIBaseAircraft_HXX

#include "AIBase.hxx"

#include <string>

class FGAIBaseAircraft : public FGAIBase {

public:
    FGAIBaseAircraft();

    void bind();

    // Note that this is mapped to all 6 gear indices gear/gear[0..5]
    void setGearPos(double pos)      { m_gearPos = pos; };
    void setFlapsPos(double pos)      { m_flapsPos = pos; };
    void setSpoilerPos(double pos)    { m_spoilerPos = pos; };
    void setSpeedBrakePos(double pos) { m_speedbrakePos = pos; };
    void setBeaconLight(bool light)   { m_beaconLight = light; };
    void setLandingLight(bool light)  { m_landingLight = light; };
    void setNavLight(bool light)      { m_navLight = light; };
    void setStrobeLight(bool light)   { m_strobeLight = light; };
    void setTaxiLight(bool light)     { m_taxiLight = light; };
    void setCabinLight(bool light)    { m_cabinLight = light; };

    double getGearPos()       const { return m_gearPos; };
    double getFlapsPos()      const { return m_flapsPos; };
    double getSpoilerPos()    const { return m_spoilerPos; };
    double getSpeedBrakePos() const { return m_speedbrakePos; };
    bool   getBeaconLight()   const { return m_beaconLight; };
    bool   getLandingLight()  const { return m_landingLight; };
    bool   getNavLight()      const { return m_navLight; };
    bool   getStrobeLight()   const { return m_strobeLight; };
    bool   getTaxiLight()     const { return m_taxiLight; };
    bool   getCabinLight()    const { return m_cabinLight; };

protected:

    // Aircraft properties.  
    double m_gearPos;
    double m_flapsPos;
    double m_spoilerPos;
    double m_speedbrakePos;

    // Light properties.
    bool m_beaconLight;
    bool m_cabinLight;
    bool m_landingLight;
    bool m_navLight;
    bool m_strobeLight;
    bool m_taxiLight;
};

#endif

