// inst_vertical_speed_indicator.hxx -- Instantaneous VSI (emulation calibrated to standard atmosphere).
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

#ifndef __INST_VERTICAL_SPEED_INDICATOR_HXX
#define __INST_VERTICAL_SPEED_INDICATOR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


class SGInterpTable;


/**
 * Model an instantaneous VSI tied to the external pressure.
 *
 * Input properties:
 *
 * /instrumentation/inst-vertical-speed-indicator/serviceable
 * /environment/pressure-inhg
 * /environment/pressure-sea-level-inhg
 * /sim/speed-up
 * /sim/freeze/master
 *
 * Output properties:
 *
 * /instrumentation/inst-vertical-speed-indicator/indicated-speed-fps
 * /instrumentation/inst-vertical-speed-indicator/indicated-speed-fpm
 */
class InstVerticalSpeedIndicator : public SGSubsystem
{

public:

    InstVerticalSpeedIndicator ( SGPropertyNode *node );
    virtual ~InstVerticalSpeedIndicator ();

    virtual void init ();
    virtual void update (double dt);

private:

    std::string _name;
    int _num;

    double _internal_pressure_inhg;
    double _internal_sea_inhg;

    double _speed_ft_per_s;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _freeze_node;
    SGPropertyNode_ptr _pressure_node;
    SGPropertyNode_ptr _sea_node;
    SGPropertyNode_ptr _speed_up_node;
    SGPropertyNode_ptr _speed_node;
    SGPropertyNode_ptr _speed_min_node;

    SGInterpTable * _pressure_table;
    SGInterpTable * _altitude_table;
};

#endif // __INST_VERTICAL_SPEED_INDICATOR_HXX
