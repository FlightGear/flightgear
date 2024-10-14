/*
 * SPDX-FileName: AIGroundVehicle.hxx
 * SPDX-FileComment: AIShip-derived class creates an AI Ground Vehicle by adding a ground following utility
 * SPDX-FileCopyrightText: Written by Vivian Meazza, started August 2009 - vivian.meazza at lineone.net
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cmath>
#include <string_view>
#include <vector>

#include <simgear/scene/material/mat.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "AIShip.hxx"

#include "AIBase.hxx"
#include "AIManager.hxx"


class FGAIGroundVehicle : public FGAIShip
{
public:
    FGAIGroundVehicle();
    virtual ~FGAIGroundVehicle() = default;

    std::string_view getTypeString(void) const override { return "groundvehicle"; }
    void readFromScenario(SGPropertyNode* scFileNode) override;

    bool init(ModelSearchOrder searchOrder) override;
    void bind() override;
    void reinit() override;
    void update(double dt) override;

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
                          double& range, double& bearing) const;

    SGGeod _selectedpos;

    bool _solid = true;            // if true ground is solid for FDMs
    double _load_resistance = 0.0; // ground load resistance N/m^2
    double _frictionFactor = 0.0;  // dimensionless modifier for Coefficient of Friction

    double _elevation = 0.0;
    double _elevation_coeff = 0.0;
    double _ht_agl_ft = 0.0;

    double _tow_angle_gain = 0.0;
    double _tow_angle_limit = 0.0;

    double _contact_x1_offset = 0.0;
    double _contact_x2_offset = 0.0;
    double _contact_z_offset = 0.0;

    double _pitch = 0.0;
    double _pitch_coeff = 0.0;
    double _pitch_deg = 0.0;

    double _speed_coeff = 0.0;
    double _speed_kt = 0.0;

    double _range_ft = 0.0;
    double _relbrg = 0.0;

    double _parent_speed = 0.0;
    double _parent_x_offset = 0.0;
    double _parent_y_offset = 0.0;
    double _parent_z_offset = 0.0;

    double _hitch_x_offset_m = 0.0;
    double _hitch_y_offset_m = 0.0;
    double _hitch_z_offset_m = 0.0;
    double _break_count = 0.0;

    double _tunnel_start_alt = 0.0;
    double _tunnel_end_alt   = 0.0;
    double _tunnel_distance  = 0.0;
};
