// Air Ground Radar
//
// Written by Vivian MEAZZA, started Feb 2008.
// 
//
// Copyright (C) 2008  Vivain MEAZZA - vivian.meazza@lineone.net
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
//

#ifndef _INST_AGRADAR_HXX
#define _INST_AGRADAR_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <Scenery/scenery.hxx>
#include <simgear/scene/material/mat.hxx>

#include "wxradar.hxx"

class agRadar : public wxRadarBg{
public:

    agRadar ( SGPropertyNode *node );
    agRadar ();
    virtual ~agRadar ();

    virtual void init ();
    virtual void update (double dt);

    void setUserPos();
    void setUserVec(int az, double el);
    void update_terrain();
    void setAntennaPos();

    bool getMaterial();

    double _load_resistance;    // ground load resistanc N/m^2
    double _frictionFactor;     // dimensionless modifier for Coefficient of Friction
    double _bumpinessFactor;    // dimensionless modifier for Bumpiness
    double _elevation_m;        // ground elevation in meters
    bool   _solid;              // if true ground is solid for FDMs

    const SGMaterial* _material;

    string _mat_name; // ground material

    SGVec3d getCartUserPos() const;
    SGVec3d getCartAntennaPos()const;

    SGVec3d uservec;

    SGPropertyNode_ptr _user_hdg_deg_node;
    SGPropertyNode_ptr _user_roll_deg_node;
    SGPropertyNode_ptr _user_pitch_deg_node;
    SGPropertyNode_ptr _terrain_warning_node;

    SGGeod userpos;
    SGGeod hitpos;
    SGGeod antennapos;
};

#endif // _INST_AGRADAR_HXX
