/*
 * SPDX-FileName: heading_indicator_dg.hxx
 * SPDX-FileComment: a Directional Gyro (DG) compass.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileContributor: Written by Vivian Meazza, started 2005.
 * SPDX-FileContributor: Enhanced by Benedikt Hallinger, 2023
 */

#pragma once


#include <Instrumentation/AbstractInstrument.hxx>

#include "gyro.hxx"


/**
 * Model an electrically- or vacuum powered heading indicator.
 *
 * Input properties:
 *
 * /instrumentation/"name"/serviceable
 * /instrumentation/"name"/spin
 * /instrumentation/"name"/offset-deg
 * /orientation/heading-deg
 * /position/latitude-deg
 * /velocities/east-relground-fps
 * /systems/electrical/outputs/DG
 * /systems/vacuum/suction-inhg    (optionally)
 *
 * Output properties:
 *
 * /instrumentation/"name"/indicated-heading-deg
 * /instrumentation/"name"/drift-per-hour-deg
 * /instrumentation/"name"/transport-wander-per-hour-deg
 * 
 * 
 * Configuration:
 * 
 *   name
 *   number
 *   new-default-power-path: use /systems/electrical/outputs/"name"[ number ] instead of 
 *                           /systems/electrical/outputs/DG as the default power
 *                           supply path (not used when power-supply is set)
 *   heading-source          If given, heading is taken from this node (default: "/orientation/heading-deg")
 *   power-supply
 *   minimum-supply-volts
 *   suction                 If given, gyro is vacuum driven from the property given
 *   gyro/spin-up-sec        If given, seconds to spin up until power-norm (from 0->100%)
 *   gyro/spin-down-sec      If given, seconds the gyro will loose spin without power (from 100%->0)
 *
 * These are optional and also configurable at runtime below instruments limits subtree:
 *   minimum-vacuum                 Default 5.0 inHG
 *   gyro/minimum-spin-norm         Default 0.9 (0.0 to 1.0)
 *   limits/yaw-rate-source         Default: "/orientation/yaw-rate-degps"
 *   limits/yaw-error-factor        Default 0.033  (set to 0 to disable yaw-error influence)
 *   limits/yaw-limit-rate          Default 5.0
 *   limits/g-node                  Path to g-node; default "/accelerations/pilot-g"
 *   limits/g-filter-time           Default 10.0 (set to 0 to disable); time for g low-pass filter (to filter out spikes due to caluclation artifacts)
 *   limits/g-error-factor          Default 0.033  (set to 0 to disable g-error influence)
 *   limits/g-limit-lower           Default -0.5
 *   limits/g-limit-upper           Default  1.5
 *   limits/g-limit-tumble-factor   Default 1.5 (150% above specified g-limits in g-limit-lower/g-limit-upper)
 *
 *  These are just for changing at runtime:
 *   latitude-nut-setting    Default 0° latitude (equator; negative=southern hemisphere), to be set by aircraft code
 */
class HeadingIndicatorDG : public AbstractInstrument
{
public:
    HeadingIndicatorDG ( SGPropertyNode *node );
    HeadingIndicatorDG ();
    virtual ~HeadingIndicatorDG ();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "heading-indicator-dg"; };

private:
    Gyro _gyro;
    double _last_heading_deg, _last_indicated_heading_dg;

    std::string _powerSupplyPath;
    std::string _suctionPath;
    std::string _gnodePath;
    std::string _heading_in_nodePath;
    std::string _yaw_rate_nodePath;
    bool _vacuumDriven = false;

    SGPropertyNode_ptr _limits_node;
    double _minVacuum = 4.0; // inHg
    SGPropertyNode_ptr _minVacuum_node;

    double _gyro_lag, _gyro_spin_up, _gyro_spin_down;
    double _minSpin, _yaw_error_factor, _g_error_factor,
        _yaw_limit_rate, _last_g, _g_filtertime, _g_limit_lower, _g_limit_upper, _g_limit_tumble;
    SGPropertyNode_ptr _minSpin_node, _yaw_error_factor_node, _g_error_factor_node,
        _yaw_limit_rate_node, _g_limit_lower_node, _g_limit_upper_node;

    SGPropertyNode_ptr _offset_node;
    SGPropertyNode_ptr _heading_in_node;
    SGPropertyNode_ptr _serviceable_node;
    SGPropertyNode_ptr _heading_out_node;
    SGPropertyNode_ptr _drift_ph_out_node;
    SGPropertyNode_ptr _transp_wander_out_node;
    SGPropertyNode_ptr _we_speed_node;
    SGPropertyNode_ptr _lat_nut_node;
    SGPropertyNode_ptr _caged_node;
    SGPropertyNode_ptr _tumble_node, _tumble_flag_node, _g_limit_tumble_node;
    SGPropertyNode_ptr _electrical_node;
    SGPropertyNode_ptr _error_node;
    SGPropertyNode_ptr _nav1_error_node;
    SGPropertyNode_ptr _align_node;
    SGPropertyNode_ptr _yaw_rate_node;
    SGPropertyNode_ptr _heading_bug_error_node;
    SGPropertyNode_ptr _g_node, _g_filtertime_node;
    SGPropertyNode_ptr _spin_node, _gyro_spin_up_node, _gyro_spin_down_node;
    SGPropertyNode_ptr _suction_node;
};
