// Radar Altimeter
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

#ifndef _INST_RADALT_HXX
#define _INST_RADALT_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <Scenery/scenery.hxx>
#include <simgear/scene/material/mat.hxx>

#include "agradar.hxx"

class radAlt : public agRadar{
public:

    radAlt ( SGPropertyNode *node );
    radAlt ();
    virtual ~radAlt ();

private:

    virtual void init ();
    virtual void update (double dt);

    void update_altitude();
    double getDistanceAntennaToHit(SGVec3d h) const;

    SGPropertyNode_ptr _user_alt_agl_node;
    SGPropertyNode_ptr _rad_alt_warning_node;

    double _min_radalt;

};

#endif // _INST_AGRADAR_HXX
