// FGAIGroundVehicle - FGAIShip-derived class creates an AI Ground Vehicle
// by adding a ground following utility
//
// Written by Vivian Meazza, started August 2009.
// - vivian.meazza at lineone.net
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

#ifndef _FG_AIESCORT_HXX
#define _FG_AIESCORT_HXX

#include <string>
#include <list>

#include <simgear/compiler.h>

#include "AIBase.hxx"

#include "AIShip.hxx"

#include "AIManager.hxx"
#include "AIBase.hxx"

class FGAIEscort : public FGAIShip {
public:
    FGAIEscort();
    virtual ~FGAIEscort();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void reinit();
    virtual void update (double dt);

    virtual const char* getTypeString(void) const { return "escort"; }

private:
    void setStnRange(double r);
    void setStnBrg(double y);
    void setStationSpeed();
    void setStnLimit(double l);
    void setStnAngleLimit(double l);
    void setStnSpeed(double s);
    void setStnHtFt(double h);
    void setStnPatrol(bool p);
    void setStnDegTrue(bool t);
    void setParent();

    void setMaxSpeed(double m);
    void setUpdateInterval(double i);

    void RunEscort(double dt);

    bool getGroundElev(SGGeod inpos);

    SGVec3d getCartHitchPosAt(const SGVec3d& off) const;

    void calcRangeBearing(double lat, double lon, double lat2, double lon2,
        double &range, double &bearing) const;
    double calcTrueBearingDeg(double bearing, double heading);

    SGGeod _selectedpos;
    SGGeod _tgtpos;

    bool   _solid;           // if true ground is solid for FDMs
    double _load_resistance; // ground load resistanc N/m^2
    double _frictionFactor;  // dimensionless modifier for Coefficient of Friction
    double _tgtrange, _tgtbrg;
    double _ht_agl_ft;
    double _relbrg, _truebrg;
    double _parent_speed, _parent_hdg;
    double _interval;

    double _stn_relbrg, _stn_truebrg, _stn_brg, _stn_range, _stn_height;
    double _stn_speed, _stn_angle_limit, _stn_limit;

    double _max_speed;

    const SGMaterial* _material;

    bool _MPControl, _patrol, _stn_deg_true;

//    std::string _parent;

};

#endif  // FG_AIGROUNDVEHICLE_HXX
