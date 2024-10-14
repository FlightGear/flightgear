/*
 * SPDX-FileName: heading_indicator_dg.cxx
 * SPDX-FileComment: a Directional Gyro (DG) compass.
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileContributor: Written by Vivian Meazza, started 2005.
 * SPDX-FileContributor: Enhanced by Benedikt Hallinger, 2023
 */

#include "config.h"

#include "heading_indicator_dg.hxx"

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/SGMath.hxx>
#include <iostream>
#include <string>
#include <sstream>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

/** Macro calculating x^6 (faster than super-slow math/pow). */
#define POW6(x) (x*x*x*x*x*x)

HeadingIndicatorDG::HeadingIndicatorDG(SGPropertyNode* node) : _last_heading_deg(0),
                                                               _last_indicated_heading_dg(0),
                                                               _gyro_lag(0)
{
    if (!node->getBoolValue("new-default-power-path", false)) {
        setDefaultPowerSupplyPath("/systems/electrical/outputs/DG");
    }
    if (node->hasChild("suction")) {
        // If vacuum driven, reconfigure default abstract instrument implementation
        _vacuumDriven = true;
        _suctionPath = node->getStringValue("suction");
        SGPropertyNode_ptr _cfg_node = node->getChild("power-supply", 0, true);
        _cfg_node->setStringValue(_suctionPath);

        _minVacuum = node->getDoubleValue("minimum-vacuum", _minVacuum);
    }

    _heading_in_nodePath = node->getStringValue("heading-source", "/orientation/heading-deg");

    SGPropertyNode* gyro_cfg = node->getChild("gyro", 0, true);
    _minSpin = gyro_cfg->getDoubleValue("minimum-spin-norm", 0.9);
    _gyro_spin_up = gyro_cfg->getDoubleValue("spin-up-sec", 4.0);
    _gyro_spin_down = gyro_cfg->getDoubleValue("spin-down-sec", 180.0);

    SGPropertyNode* limits_cfg = node->getChild("limits", 0, true);
    _yaw_rate_nodePath = limits_cfg->getStringValue("yaw-rate-source", "/orientation/yaw-rate-degps");
    _yaw_error_factor = limits_cfg->getDoubleValue("yaw-error-factor", 0.033);
    _yaw_limit_rate = limits_cfg->getDoubleValue("yaw-limit-rate", 5.0);
    _g_error_factor = limits_cfg->getDoubleValue("g-error-factor", 0.033);
    _gnodePath     = limits_cfg->getStringValue("g-node", "/accelerations/pilot-g");
    _g_filtertime = limits_cfg->getDoubleValue("g-filter-time", 10.0);
    _g_limit_lower = limits_cfg->getDoubleValue("g-limit-lower", -0.5);
    _g_limit_upper = limits_cfg->getDoubleValue("g-limit-upper", 1.5);
    _g_limit_tumble = limits_cfg->getDoubleValue("g-limit-tumble-factor", 1.5);

    readConfig(node, "heading-indicator-dg");
}

HeadingIndicatorDG::~HeadingIndicatorDG ()
{
}

void
HeadingIndicatorDG::init ()
{
    std::string branch = nodePath();

    _heading_in_node = fgGetNode(_heading_in_nodePath, true);
    _yaw_rate_node   = fgGetNode(_yaw_rate_nodePath, true);
    _g_node = fgGetNode(_gnodePath, true);
    _we_speed_node = fgGetNode("/velocities/east-relground-fps", true);

    SGPropertyNode *node    = fgGetNode(branch, true );
    _offset_node            = node->getChild("offset-deg", 0, true);
    _heading_bug_error_node = node->getChild("heading-bug-error-deg", 0, true);
    _error_node             = node->getChild("error-deg", 0, true);
    _nav1_error_node        = node->getChild("nav1-course-error-deg", 0, true);
    _heading_out_node       = node->getChild("indicated-heading-deg", 0, true);
    _drift_ph_out_node = node->getChild("drift-per-hour-deg", 0, true);
    _lat_nut_node = node->getChild("latitude-nut-setting", 0, true);
    _transp_wander_out_node = node->getChild("transport-wander-per-hour-deg", 0, true);
    _caged_node = node->getChild("caged-flag", 0, true);
    _tumble_node = node->getChild("tumble-norm", 0, true);
    _tumble_flag_node = node->getChild("tumble-flag", 0, true);
    _align_node             = node->getChild("align-deg", 0, true);
    _spin_node              = node->getChild("spin", 0, true);
    SGPropertyNode* gyro_node = node->getChild("gyro", 0, true);
    _minSpin_node = gyro_node->getChild("minimum-spin", 0, true);
    _gyro_spin_up_node = gyro_node->getChild("spin-up-sec", 0, true);
    _gyro_spin_down_node = gyro_node->getChild("spin-down-sec", 0, true);

    if (_vacuumDriven) {
        _suction_node = fgGetNode(_suctionPath, true);
        _minVacuum_node = node->getChild("minimum-vacuum", 0, true);
    }

    SGPropertyNode* limits_node = node->getChild("limits", 0, true);
    _yaw_error_factor_node = limits_node->getChild("yaw-error-factor", 0, true);
    _yaw_limit_rate_node = limits_node->getChild("yaw-limit-rate", 0, true);
    _g_filtertime_node = limits_node->getChild("g-filter-time", 0.0, true);
    _g_error_factor_node = limits_node->getChild("g-error-factor", 0, true);
    _g_limit_lower_node = limits_node->getChild("g-limit-lower", 0, true);
    _g_limit_upper_node = limits_node->getChild("g-limit-upper", 0, true);
    _g_limit_tumble_node = limits_node->getChild("g-limit-tumble-factor", 0, true);

    initServicePowerProperties(node);

    reinit();
}

void
HeadingIndicatorDG::reinit (void)
{
    // reset errors/drift values
    _align_node->setDoubleValue(0.0);
    _error_node->setDoubleValue(0.0);
    _offset_node->setDoubleValue(0.0);

    _last_heading_deg = _heading_in_node->getDoubleValue();
    _last_indicated_heading_dg = _last_heading_deg;

    if (!_minSpin_node->hasValue())
        _minSpin_node->setDoubleValue(_minSpin);
    if (_vacuumDriven && !_minVacuum_node->hasValue())
        _minVacuum_node->setDoubleValue(_minVacuum);
    if (!_gyro_spin_up_node->hasValue())
        _gyro_spin_up_node->setDoubleValue(_gyro_spin_up);
    if (!_gyro_spin_down_node->hasValue())
        _gyro_spin_down_node->setDoubleValue(_gyro_spin_down);

    _yaw_error_factor_node->setDoubleValue(_yaw_error_factor);
    _yaw_limit_rate_node->setDoubleValue(_yaw_limit_rate);
    _g_filtertime_node->setDoubleValue(_g_filtertime);
    _g_error_factor_node->setDoubleValue(_g_error_factor);
    _g_limit_lower_node->setDoubleValue(_g_limit_lower);
    _g_limit_upper_node->setDoubleValue(_g_limit_upper);
    _g_limit_tumble_node->setDoubleValue(_g_limit_tumble);
    _last_g = _g_node->getDoubleValue();

    _tumble_flag_node->setBoolValue(0);
    _tumble_node->setDoubleValue(0.0);

    _gyro.reinit();
}

void
HeadingIndicatorDG::update (double dt)
{
    // Get the spin from the gyro
    if (_vacuumDriven) {
        // treat the power supply node as vacuum source in inHG
        _minVacuum = _minVacuum_node->getDoubleValue();
        _gyro.set_power_norm(isServiceableAndPowered() * _suction_node->getDoubleValue() / _minVacuum);
    } else {
        // gyro is operated electrically
        _gyro.set_power_norm(isServiceableAndPowered());
    }

    _gyro.set_spin_up(_gyro_spin_up_node->getDoubleValue());
    _gyro.set_spin_down(_gyro_spin_down_node->getDoubleValue());
    _gyro.set_spin_norm(_spin_node->getDoubleValue());
    _gyro.update(dt);

    // read inputs
    double spin = _gyro.get_spin_norm();
    double heading = _heading_in_node->getDoubleValue();
    double offset = _offset_node->getDoubleValue();
    bool isCaged = _caged_node->getBoolValue();

    _spin_node->setDoubleValue( spin );

    // calculate scaling factor
    // caged gyro will be forced into position and behave like "stuck".
    double factor = isCaged ? 0.0 : POW6(spin);

    // calculate time-based precession:
    // 0° at equator, ~15°/hr (360°/day) at poles (+/- 90°Lat)
    // (northern hemisphere causes under-read ie. clockwise rotation)
    // Drift can be corrected by a litude nut setting (wich is a screwed weight on the gimbal)
    double latPos = globals->get_aircraft_position().getLatitudeRad();
    double drift_per_hour = -15 * sin(latPos);
    double lat_nut_setting = _lat_nut_node->getDoubleValue();
    drift_per_hour += 15 * sin(lat_nut_setting * SG_DEGREES_TO_RADIANS); // correction of latitude
    drift_per_hour *= factor;                                            // apply spin factor: non spinning/stuck gyro has no drift
    _drift_ph_out_node->setDoubleValue(drift_per_hour);
    double drift_per_frame = (drift_per_hour / 60 / 60) * dt; // convert hrs->frame(1/s)
    offset += drift_per_frame;                                // apply drift

    // Caluclate transport wander
    // this is: Degrees-of-longitude-travelled * 1/60tan(lat)
    // Travelling East->West gives overreading(+) in norhtern hemisphere, underreading(-) south.
    // Example: flying west at 100 kts at +45° north gives: (100 x Tan 45)/60 = +1.66 degrees per hour.
    double gnd_speed_kth = (-1 * SG_FPS_TO_KT * _we_speed_node->getDoubleValue()); // convert and flip sign: east speed needs to be negative
    double transport_wander_p_hour = gnd_speed_kth * (tan(latPos) / 60);
    transport_wander_p_hour *= factor; // apply spin factor: non spinning/stuck gyro has no drift
    _transp_wander_out_node->setDoubleValue(transport_wander_p_hour);
    double transport_wander_per_frame = (transport_wander_p_hour / 60 / 60) * dt; // convert hrs->frame(1/s)
    offset += transport_wander_per_frame;

    // indication should get more and more stuck at low gyro spins
    _minSpin = _minSpin_node->getDoubleValue();
    if (spin < _minSpin || isCaged) {
        // when gyro spin is low, then any heading change results in
        // increasing the offset
        double diff = SGMiscd::normalizePeriodic(-180.0, 180.0, _last_heading_deg - heading);
        // scaled by 1-factor, so indication is fully stuck at spin==0 (offset compensates
        // any heading change)
        offset += diff * (1.0 - factor) * dt;

        if (isCaged && _gyro_lag == 0.0) {
            // store drift for later, so we can persist the offset once the gyro is back
            _gyro_lag = heading;
        }
    }
    _last_heading_deg = heading;
    if (!isCaged && _gyro_lag != 0.0) {
        // apply stored drift so we avoid the heading jump back to the masked heading when uncaging
        offset += _gyro_lag - heading;
        _gyro_lag = 0.0;
    }

    // normalize offset
    offset = SGMiscd::normalizePeriodic(-180.0,180.0,offset);
    _offset_node->setDoubleValue(offset);

                                // No magvar - set the alignment manually
    double align = _align_node->getDoubleValue();

    // Movement-induced error
    double error = _error_node->getDoubleValue();
    _yaw_error_factor = _yaw_error_factor_node->getDoubleValue();
    _yaw_limit_rate = _yaw_limit_rate_node->getDoubleValue();
    double yaw_rate = _yaw_rate_node->getDoubleValue();
    if (fabs(yaw_rate) > _yaw_limit_rate) {
        error += _yaw_error_factor * -yaw_rate * dt * factor;
    }

    _g_error_factor = _g_error_factor_node->getDoubleValue();
    _g_limit_lower = _g_limit_lower_node->getDoubleValue();
    _g_limit_upper = _g_limit_upper_node->getDoubleValue();
    double g = _g_node->getDoubleValue();
    _g_filtertime = _g_filtertime_node->getDoubleValue();
    if (_g_filtertime > 0.0) {
        g = fgGetLowPass(_last_g, g, dt * _g_filtertime);
    }
    _last_g = g;
    if (g > _g_limit_upper || g < _g_limit_lower) {
        error += _g_error_factor * g * dt * factor;
    }

    // Error due to tumbling gyro: calculate the tumble for the next pass.
    _g_limit_tumble = _g_limit_tumble_node->getDoubleValue();
    double _g_limit_tumble_lower = _g_limit_lower * _g_limit_tumble;
    double _g_limit_tumble_upper = _g_limit_upper * _g_limit_tumble;
    double glimit_tumble_exceed = 0;
    if (g < _g_limit_tumble_lower)
        glimit_tumble_exceed = g / _g_limit_tumble_lower;
    if (g > _g_limit_tumble_upper)
        glimit_tumble_exceed = g / _g_limit_tumble_upper;
    if (glimit_tumble_exceed > 0 && !isCaged)
        _tumble_flag_node->setBoolValue(true);

    if (_tumble_flag_node->getBoolValue()) {
        double tumble = _tumble_node->getDoubleValue();
        double tumble_exceed = glimit_tumble_exceed / 2.0;
        if (tumble_exceed > tumble)
            tumble = tumble_exceed;
        if (tumble > 1.0)
            tumble = 1.0;
        if (tumble < -1.0)
            tumble = -1.0;

        // Reerect in 5 minutes or promptly when forced into position
        double t_reerect = isCaged ? 1.0 : 300.0;
        double step = dt / t_reerect;
        if (tumble < -step)
            tumble += step;
        else if (tumble > step)
            tumble -= step;
        if (fabs(tumble) < 0.01) {
            tumble = 0.0;
            _tumble_flag_node->setBoolValue(false);
        }

        error += tumble * 720.0 * dt / 1.0; // deg/s; where tumble=1.0 -> max rotate speed
        _tumble_node->setDoubleValue(tumble);
    }

    error = SGMiscd::normalizePeriodic(-180.0, 180.0, error);
    _error_node->setDoubleValue(error);

    heading = flightgear::lowPassPeriodicDegreesSigned(_last_indicated_heading_dg, heading, dt * 100 * factor);
    _last_indicated_heading_dg = heading;
    
    heading += offset + align + error;
    heading = SGMiscd::normalizePeriodic(0.0,360.0,heading);

    _heading_out_node->setDoubleValue(heading);

    // calculate the difference between the indicated heading
    // and the selected heading for use with an autopilot
    SGPropertyNode* bnode = fgGetNode("/autopilot/settings/heading-bug-deg", false);
    if ( bnode ) {
        auto diff = SGMiscd::normalizePeriodic(-180.0, 180.0, bnode->getDoubleValue() - heading);
        _heading_bug_error_node->setDoubleValue(diff);
    }
                                 // calculate the difference between the indicated heading
                                 // and the selected nav1 radial for use with an autopilot
    SGPropertyNode* nnode = fgGetNode("/instrumentation/nav/radials/selected-deg", false);
    if ( nnode ) {
        auto diff = SGMiscd::normalizePeriodic(-180.0, 180.0, nnode->getDoubleValue() - heading);
        _nav1_error_node->setDoubleValue( diff );
    }
}

// end of heading_indicator_dg.cxx
