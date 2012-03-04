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

#ifndef _FG_AIGROUNDVEHICLE_HXX
#define _FG_AIGROUNDVEHICLE_HXX

#include <math.h>
#include <vector>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/scene/material/mat.hxx>

#include "AIShip.hxx"

#include "AIManager.hxx"
#include "AIBase.hxx"

class FGAIGroundVehicle : public FGAIShip {
public:
    FGAIGroundVehicle();
    virtual ~FGAIGroundVehicle();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void reinit();
    virtual void update (double dt);

    virtual const char* getTypeString(void) const { return "groundvehicle"; }

private:

    void setNoRoll(bool nr);
    void setContactX1offset(double x1);
    void setContactX2offset(double x2);
    void setXOffset(double x);
    void setYOffset(double y);
    void setZOffset(double z);

    void setPitchCoeff(double pc);
    void setElevCoeff(double ec);
    void setTowAngleGain(double g);
    void setTowAngleLimit(double l);
    void setElevation(double _elevation, double dt, double _elevation_coeff);
    void setPitch(double _pitch, double dt, double _pitch_coeff);
    void setTowAngle(double _relbrg, double dt, double _towangle_coeff);
    void setTrainSpeed(double s, double dt, double coeff);
    void setParent();
    void AdvanceFP();
    void setTowSpeed();
    void RunGroundVehicle(double dt);

    bool getGroundElev(SGGeod inpos);
    bool getPitch();

    SGVec3d getCartHitchPosAt(const SGVec3d& off) const;

    void calcRangeBearing(double lat, double lon, double lat2, double lon2,
        double &range, double &bearing) const;

    SGGeod _selectedpos;

    bool   _solid;           // if true ground is solid for FDMs
    double _load_resistance; // ground load resistanc N/m^2
    double _frictionFactor;  // dimensionless modifier for Coefficient of Friction
    double _elevation, _elevation_coeff;
    double _tow_angle_gain, _tow_angle_limit;
    double _ht_agl_ft;
    double _contact_x1_offset, _contact_x2_offset, _contact_z_offset;
    double _pitch, _pitch_coeff, _pitch_deg;
    double _speed_coeff, _speed_kt;
    double _x_offset, _y_offset;
    double _range_ft;
    double _relbrg;
    double _parent_speed, _parent_x_offset, _parent_y_offset, _parent_z_offset;
    double _hitch_x_offset_m, _hitch_y_offset_m, _hitch_z_offset_m;
    double _dt_count, _next_run, _break_count;

    const SGMaterial* _material;

};

#endif  // FG_AIGROUNDVEHICLE_HXX
