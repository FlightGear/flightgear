/*
 * SPDX-License-Identifier: CC0-1.0
 * 
 * mag_compass.hxx - models a magnetic compass
 * Written by David Megginson, started 2002.
 * 
 * This file is in the Public Domain and comes with no warranty.
*/

#pragma once

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>


/**
 * Model a magnetic compass.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/pitch-offset-deg
 * /instrumentation/"name"/max-pitch-deg
 * /instrumentation/"name"/max-roll-deg
 * /orientation/roll-deg
 * /orientation/pitch-deg
 * /orientation/heading-magnetic-deg
 * /orientation/side-slip-deg
 * /environment/magnetic-dip-deg
 * /accelerations/pilot/north-accel-fps_sec
 * /accelerations/pilot/east-accel-fps_sec
 * /accelerations/pilot/down-accel-fps_sec
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-heading-deg
 * /instrumentation/"name"/pitch-deg
 * /instrumentation/"name"/roll-deg
 *
 * Config properties:
 * /instrumentation/"name"/fluid-viscosity
 *
 */
class MagCompass : public SGSubsystem
{
public:
    MagCompass ( SGPropertyNode *node);
    MagCompass ();
    virtual ~MagCompass ();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "magnetic-compass"; }

private:
    double _rate_degps;

    double _last_pitch, _last_roll, _cfg_viscosity;

    std::string _name;
    int _num;
    SGSharedPtr<SGInterpTable> _deviation_table;
    SGPropertyNode_ptr _deviation_node;

    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _pitch_offset_node;
    SGPropertyNode_ptr _roll_node;
    SGPropertyNode_ptr _pitch_node;
    SGPropertyNode_ptr _heading_node;
    SGPropertyNode_ptr _beta_node;
    SGPropertyNode_ptr _dip_node;
    SGPropertyNode_ptr _x_accel_node;
    SGPropertyNode_ptr _y_accel_node;
    SGPropertyNode_ptr _z_accel_node;
    SGPropertyNode_ptr _fluid_viscosity;
    SGPropertyNode_ptr _out_node;
    SGPropertyNode_ptr _roll_out_node;
    SGPropertyNode_ptr _pitch_out_node;
};
