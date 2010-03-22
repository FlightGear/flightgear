// replay.cxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started Juley 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/constants.h>

#include <FDM/flight.hxx>
#include <Main/fg_props.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>
#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>

#include "replay.hxx"

const double FGReplay::st_list_time = 60.0;   // 60 secs of high res data
const double FGReplay::mt_list_time = 600.0;  // 10 mins of 1 fps data
const double FGReplay::lt_list_time = 3600.0; // 1 hr of 10 spf data

// short term sample rate is as every frame
const double FGReplay::mt_dt = 0.5; // medium term sample rate (sec)
const double FGReplay::lt_dt = 5.0; // long term sample rate (sec)

/**
 * Constructor
 */

FGReplay::FGReplay() {
}


/**
 * Destructor
 */

FGReplay::~FGReplay() {
    while ( !short_term.empty() ) {
	//cerr << "Deleting Short term" <<endl;
        delete short_term.front();
        short_term.pop_front();
    }
    while ( !medium_term.empty() ) {
	//cerr << "Deleting Medium term" <<endl;
	delete medium_term.front();
        medium_term.pop_front();
    }
    while ( !long_term.empty() ) {
	//cerr << "Deleting Long term" <<endl;
	delete long_term.front();
        long_term.pop_front();
    }
    while ( !recycler.empty() ) {
	//cerr << "Deleting Recycler" <<endl;
	delete recycler.front();
        recycler.pop_front();
    }
}


/** 
 * Initialize the data structures
 */

void FGReplay::init() {
    sim_time = 0.0;
    last_mt_time = 0.0;
    last_lt_time = 0.0;

    // Make sure all queues are flushed
    while ( !short_term.empty() ) {
        delete short_term.front();
        short_term.pop_front();
    }
    while ( !medium_term.empty() ) {
	delete medium_term.front();
        medium_term.pop_front();
    }
    while ( !long_term.empty() ) {
	delete long_term.front();
        long_term.pop_front();
    }
    while ( !recycler.empty() ) {
	delete recycler.front();
        recycler.pop_front();
    }
    // Create an estimated nr of required ReplayData objects
    // 120 is an estimated maximum frame rate. 
    int estNrObjects = (int) ((st_list_time*120) + (mt_list_time*mt_dt) +
                             (lt_list_time*lt_dt)); 
    for (int i = 0; i < estNrObjects; i++) {
        recycler.push_back(new FGReplayData);

    }
}


/** 
 * Bind to the property tree
 */

void FGReplay::bind() {
    disable_replay = fgGetNode( "/sim/replay/disable", true );
}


/** 
 *  Unbind from the property tree
 */

void FGReplay::unbind() {
    // nothing to unbind
}


/** 
 *  Update the saved data
 */

void FGReplay::update( double dt ) {
    timingInfo.clear();
    stamp("begin");
    static SGPropertyNode *replay_master
        = fgGetNode( "/sim/freeze/replay-state", true );

    if( disable_replay->getBoolValue() ) {
        if ( sim_time != 0.0 ) {
            // we were recording data
            init();
        }
        return;
    }
    //stamp("point_01");
    if ( replay_master->getIntValue() > 0 ) {
        // don't record the replay session
        return;
    }
    //cerr << "Recording replay" << endl;
    sim_time += dt;

    // build the replay record
    //FGNetFDM f;
    //FGProps2NetFDM( &f, false );

    // sanity check, don't collect data if FDM data isn't good
    if ( !cur_fdm_state->get_inited() ) {
        return;
    }
    //FGNetCtrls c;
    //FGProps2NetCtrls( &c, false, false );
    //stamp("point_04ba");
    FGReplayData *r;
    //stamp("point_04bb");
    if (!recycler.size()) {
        stamp("Replay_01");
        r = new FGReplayData;
	stamp("Replay_02");
    } else {
	r = recycler.front();
	recycler.pop_front();
	//stamp("point_04be");
    }
    r->sim_time = sim_time;
    //r->ctrls = c;
    //stamp("point_04e");
    FGProps2NetFDM( &(r->fdm), false );
    FGProps2NetCtrls( &(r->ctrls), false, false );
    //r->fdm = f;
    //stamp("point_05");

    // update the short term list
    //stamp("point_06");
    short_term.push_back( r );
    //stamp("point_07");
    FGReplayData *st_front = short_term.front();
    if ( sim_time - st_front->sim_time > st_list_time ) {
        while ( sim_time - st_front->sim_time > st_list_time ) {
            st_front = short_term.front();
            recycler.push_back(st_front);
            short_term.pop_front();
        }
        //stamp("point_08");
        // update the medium term list
        if ( sim_time - last_mt_time > mt_dt ) {
            last_mt_time = sim_time;
            st_front = short_term.front();
            medium_term.push_back( st_front );
            short_term.pop_front();

            FGReplayData *mt_front = medium_term.front();
            if ( sim_time - mt_front->sim_time > mt_list_time ) {
            //stamp("point_09");
                while ( sim_time - mt_front->sim_time > mt_list_time ) {
                    mt_front = medium_term.front();
                    recycler.push_back(mt_front);
                    medium_term.pop_front();
                }
                // update the long term list
                if ( sim_time - last_lt_time > lt_dt ) {
                    last_lt_time = sim_time;
                    mt_front = medium_term.front();
                    long_term.push_back( mt_front );
                    medium_term.pop_front();

                    FGReplayData *lt_front = long_term.front();
                    if ( sim_time - lt_front->sim_time > lt_list_time ) {
			//stamp("point_10");
                        while ( sim_time - lt_front->sim_time > lt_list_time ) {
                            lt_front = long_term.front();
                            recycler.push_back(lt_front);
                            long_term.pop_front();
                        }
                    }
                }
            }
        }
    }

#if 0
    cout << "short term size = " << short_term.size()
         << "  time = " << sim_time - short_term.front().sim_time
         << endl;
    cout << "medium term size = " << medium_term.size()
         << "  time = " << sim_time - medium_term.front().sim_time
         << endl;
    cout << "long term size = " << long_term.size()
         << "  time = " << sim_time - long_term.front().sim_time
         << endl;
#endif
   //stamp("point_finished");
}


static double weight( double data1, double data2, double ratio,
                      bool rotational = false ) {
    if ( rotational ) {
        // special handling of rotational data
        double tmp = data2 - data1;
        if ( tmp > SGD_PI ) {
            tmp -= SGD_2PI;
        } else if ( tmp < -SGD_PI ) {
            tmp += SGD_2PI;
        }
        return data1 + tmp * ratio;
    } else {
        // normal "linear" data
        return data1 + ( data2 - data1 ) * ratio;
    }
}

/** 
 * given two FGReplayData elements and a time, interpolate between them
 */
static void update_fdm( FGReplayData frame ) {
    FGNetFDM2Props( &frame.fdm, false );
    FGNetCtrls2Props( &frame.ctrls, false, false );
}

/** 
 * given two FGReplayData elements and a time, interpolate between them
 */
static FGReplayData interpolate( double time, FGReplayData f1, FGReplayData f2 )
{
    FGReplayData result = f1;

    FGNetFDM fdm1 = f1.fdm;
    FGNetFDM fdm2 = f2.fdm;

    FGNetCtrls ctrls1 = f1.ctrls;
    FGNetCtrls ctrls2 = f2.ctrls;

    double ratio = (time - f1.sim_time) / (f2.sim_time - f1.sim_time);

    // Interpolate FDM data

    // Positions
    result.fdm.longitude = weight( fdm1.longitude, fdm2.longitude, ratio );
    result.fdm.latitude = weight( fdm1.latitude, fdm2.latitude, ratio );
    result.fdm.altitude = weight( fdm1.altitude, fdm2.altitude, ratio );
    result.fdm.agl = weight( fdm1.agl, fdm2.agl, ratio );
    result.fdm.phi = weight( fdm1.phi, fdm2.phi, ratio, true );
    result.fdm.theta = weight( fdm1.theta, fdm2.theta, ratio, true );
    result.fdm.psi = weight( fdm1.psi, fdm2.psi, ratio, true );

    // Velocities
    result.fdm.phidot = weight( fdm1.phidot, fdm2.phidot, ratio, true );
    result.fdm.thetadot = weight( fdm1.thetadot, fdm2.thetadot, ratio, true );
    result.fdm.psidot = weight( fdm1.psidot, fdm2.psidot, ratio, true );
    result.fdm.vcas = weight( fdm1.vcas, fdm2.vcas, ratio );
    result.fdm.climb_rate = weight( fdm1.climb_rate, fdm2.climb_rate, ratio );
    result.fdm.v_north = weight( fdm1.v_north, fdm2.v_north, ratio );
    result.fdm.v_east = weight( fdm1.v_east, fdm2.v_east, ratio );
    result.fdm.v_down = weight( fdm1.v_down, fdm2.v_down, ratio );

    result.fdm.v_wind_body_north
        = weight( fdm1.v_wind_body_north, fdm2.v_wind_body_north, ratio );
    result.fdm.v_wind_body_east
        = weight( fdm1.v_wind_body_east, fdm2.v_wind_body_east, ratio );
    result.fdm.v_wind_body_down
        = weight( fdm1.v_wind_body_down, fdm2.v_wind_body_down, ratio );

    // Stall
    result.fdm.stall_warning
        = weight( fdm1.stall_warning, fdm2.stall_warning, ratio );

    // Accelerations
    result.fdm.A_X_pilot = weight( fdm1.A_X_pilot, fdm2.A_X_pilot, ratio );
    result.fdm.A_Y_pilot = weight( fdm1.A_Y_pilot, fdm2.A_Y_pilot, ratio );
    result.fdm.A_Z_pilot = weight( fdm1.A_Z_pilot, fdm2.A_Z_pilot, ratio );

    unsigned int i;

    // Engine status
    for ( i = 0; i < fdm1.num_engines; ++i ) {
        result.fdm.eng_state[i] = fdm1.eng_state[i];
        result.fdm.rpm[i] = weight( fdm1.rpm[i], fdm2.rpm[i], ratio );
        result.fdm.fuel_flow[i]
            = weight( fdm1.fuel_flow[i], fdm2.fuel_flow[i], ratio );
        result.fdm.fuel_px[i]
            = weight( fdm1.fuel_px[i], fdm2.fuel_px[i], ratio );
        result.fdm.egt[i] = weight( fdm1.egt[i], fdm2.egt[i], ratio );
        result.fdm.cht[i] = weight( fdm1.cht[i], fdm2.cht[i], ratio );
        result.fdm.mp_osi[i] = weight( fdm1.mp_osi[i], fdm2.mp_osi[i], ratio );
        result.fdm.tit[i] = weight( fdm1.tit[i], fdm2.tit[i], ratio );
        result.fdm.oil_temp[i]
            = weight( fdm1.oil_temp[i], fdm2.oil_temp[i], ratio );
        result.fdm.oil_px[i] = weight( fdm1.oil_px[i], fdm2.oil_px[i], ratio );
    }

    // Consumables
    for ( i = 0; i < fdm1.num_tanks; ++i ) {
        result.fdm.fuel_quantity[i]
            = weight( fdm1.fuel_quantity[i], fdm2.fuel_quantity[i], ratio );
    }

    // Gear status
    for ( i = 0; i < fdm1.num_wheels; ++i ) {
        result.fdm.wow[i] = (int)(weight( fdm1.wow[i], fdm2.wow[i], ratio ));
        result.fdm.gear_pos[i]
            = weight( fdm1.gear_pos[i], fdm2.gear_pos[i], ratio );
        result.fdm.gear_steer[i]
            = weight( fdm1.gear_steer[i], fdm2.gear_steer[i], ratio );
        result.fdm.gear_compression[i]
            = weight( fdm1.gear_compression[i], fdm2.gear_compression[i],
                      ratio );
    }

    // Environment
    result.fdm.cur_time = fdm1.cur_time;
    result.fdm.warp = fdm1.warp;
    result.fdm.visibility = weight( fdm1.visibility, fdm2.visibility, ratio );

    // Control surface positions (normalized values)
    result.fdm.elevator = weight( fdm1.elevator, fdm2.elevator, ratio );
    result.fdm.left_flap = weight( fdm1.left_flap, fdm2.left_flap, ratio );
    result.fdm.right_flap = weight( fdm1.right_flap, fdm2.right_flap, ratio );
    result.fdm.left_aileron
        = weight( fdm1.left_aileron, fdm2.left_aileron, ratio );
    result.fdm.right_aileron
        = weight( fdm1.right_aileron, fdm2.right_aileron, ratio );
    result.fdm.rudder = weight( fdm1.rudder, fdm2.rudder, ratio );
    result.fdm.speedbrake = weight( fdm1.speedbrake, fdm2.speedbrake, ratio );
    result.fdm.spoilers = weight( fdm1.spoilers, fdm2.spoilers, ratio );
     
    // Interpolate Control input data

    // Aero controls
    result.ctrls.aileron = weight( ctrls1.aileron, ctrls2.aileron, ratio );
    result.ctrls.elevator = weight( ctrls1.elevator, ctrls2.elevator, ratio );
    result.ctrls.rudder = weight( ctrls1.rudder, ctrls2.rudder, ratio );
    result.ctrls.aileron_trim
        = weight( ctrls1.aileron_trim, ctrls2.aileron_trim, ratio );
    result.ctrls.elevator_trim
        = weight( ctrls1.elevator_trim, ctrls2.elevator_trim, ratio );
    result.ctrls.rudder_trim
        = weight( ctrls1.rudder_trim, ctrls2.rudder_trim, ratio );
    result.ctrls.flaps = weight( ctrls1.flaps, ctrls2.flaps, ratio );
    result.ctrls.flaps_power = ctrls1.flaps_power;
    result.ctrls.flap_motor_ok = ctrls1.flap_motor_ok;

    // Engine controls
    for ( i = 0; i < ctrls1.num_engines; ++i ) {
        result.ctrls.master_bat[i] = ctrls1.master_bat[i];
        result.ctrls.master_alt[i] = ctrls1.master_alt[i];
        result.ctrls.magnetos[i] = ctrls1.magnetos[i];
        result.ctrls.starter_power[i] = ctrls1.starter_power[i];
        result.ctrls.throttle[i]
            = weight( ctrls1.throttle[i], ctrls2.throttle[i], ratio );
        result.ctrls.mixture[i]
            = weight( ctrls1.mixture[i], ctrls2.mixture[i], ratio );
        result.ctrls.fuel_pump_power[i] = ctrls1.fuel_pump_power[i];
        result.ctrls.prop_advance[i]
            = weight( ctrls1.prop_advance[i], ctrls2.prop_advance[i], ratio );
        result.ctrls.engine_ok[i] = ctrls1.engine_ok[i];
        result.ctrls.mag_left_ok[i] = ctrls1.mag_left_ok[i];
        result.ctrls.mag_right_ok[i] = ctrls1.mag_right_ok[i];
        result.ctrls.spark_plugs_ok[i] = ctrls1.spark_plugs_ok[i];
        result.ctrls.oil_press_status[i] = ctrls1.oil_press_status[i];
        result.ctrls.fuel_pump_ok[i] = ctrls1.fuel_pump_ok[i];
    }

    // Fuel management
    for ( i = 0; i < ctrls1.num_tanks; ++i ) {
        result.ctrls.fuel_selector[i] = ctrls1.fuel_selector[i];
    }

    // Brake controls
    result.ctrls.brake_left
            = weight( ctrls1.brake_left, ctrls2.brake_left, ratio );
    result.ctrls.brake_right
            = weight( ctrls1.brake_right, ctrls2.brake_right, ratio );
    result.ctrls.brake_parking
            = weight( ctrls1.brake_parking, ctrls2.brake_parking, ratio );

    // Landing Gear
    result.ctrls.gear_handle = ctrls1.gear_handle;

    // Switches
    result.ctrls.turbulence_norm = ctrls1.turbulence_norm;

    // wind and turbulance
    result.ctrls.wind_speed_kt
        = weight( ctrls1.wind_speed_kt, ctrls2.wind_speed_kt, ratio );
    result.ctrls.wind_dir_deg
        = weight( ctrls1.wind_dir_deg, ctrls2.wind_dir_deg, ratio );
    result.ctrls.turbulence_norm
        = weight( ctrls1.turbulence_norm, ctrls2.turbulence_norm, ratio );

    // other information about environment
    result.ctrls.hground = weight( ctrls1.hground, ctrls2.hground, ratio );
    result.ctrls.magvar = weight( ctrls1.magvar, ctrls2.magvar, ratio );

    // simulation control
    result.ctrls.speedup = ctrls1.speedup;
    result.ctrls.freeze = ctrls1.freeze;

    return result;
}

/** 
 * interpolate a specific time from a specific list
 */
static void interpolate( double time, const replay_list_type &list ) {
    // sanity checking
    if ( list.size() == 0 ) {
        // handle empty list
        return;
    } else if ( list.size() == 1 ) {
        // handle list size == 1
        update_fdm( (*list[0]) );
        return;
    }

    unsigned int last = list.size() - 1;
    unsigned int first = 0;
    unsigned int mid = ( last + first ) / 2;


    bool done = false;
    while ( !done ) {
        // cout << "  " << first << " <=> " << last << endl;
        if ( last == first ) {
            done = true;
        } else if ( list[mid]->sim_time < time && list[mid+1]->sim_time < time ) {
            // too low
            first = mid;
            mid = ( last + first ) / 2;
        } else if ( list[mid]->sim_time > time && list[mid+1]->sim_time > time ) {
            // too high
            last = mid;
            mid = ( last + first ) / 2;
        } else {
            done = true;
        }
    }

    FGReplayData result = interpolate( time, (*list[mid]), (*list[mid+1]) );

    update_fdm( result );
}


/** 
 *  Replay a saved frame based on time, interpolate from the two
 *  nearest saved frames.
 */

void FGReplay::replay( double time ) {
    // cout << "replay: " << time << " ";
    // find the two frames to interpolate between
    double t1, t2;

    if ( short_term.size() > 0 ) {
        t1 = short_term.back()->sim_time;
        t2 = short_term.front()->sim_time;
        if ( time > t1 ) {
            // replay the most recent frame
            update_fdm( (*short_term.back()) );
            // cout << "first frame" << endl;
        } else if ( time <= t1 && time >= t2 ) {
            interpolate( time, short_term );
            // cout << "from short term" << endl;
        } else if ( medium_term.size() > 0 ) {
            t1 = short_term.front()->sim_time;
            t2 = medium_term.back()->sim_time;
            if ( time <= t1 && time >= t2 ) {
                FGReplayData result = interpolate( time,
                                                   (*medium_term.back()),
                                                   (*short_term.front()) );
                update_fdm( result );
                // cout << "from short/medium term" << endl;
            } else {
                t1 = medium_term.back()->sim_time;
                t2 = medium_term.front()->sim_time;
                if ( time <= t1 && time >= t2 ) {
                    interpolate( time, medium_term );
                    // cout << "from medium term" << endl;
                } else if ( long_term.size() > 0 ) {
                    t1 = medium_term.front()->sim_time;
                    t2 = long_term.back()->sim_time;
                    if ( time <= t1 && time >= t2 ) {
                        FGReplayData result = interpolate( time,
                                                           (*long_term.back()),
                                                           (*medium_term.front()));
                        update_fdm( result );
                        // cout << "from medium/long term" << endl;
                    } else {
                        t1 = long_term.back()->sim_time;
                        t2 = long_term.front()->sim_time;
                        if ( time <= t1 && time >= t2 ) {
                            interpolate( time, long_term );
                            // cout << "from long term" << endl;
                        } else {
                            // replay the oldest long term frame
                            update_fdm( (*long_term.front()) );
                            // cout << "oldest long term frame" << endl;
                        }
                    }
                } else {
                    // replay the oldest medium term frame
                    update_fdm( (*medium_term.front()) );
                    // cout << "oldest medium term frame" << endl;
                }
            }
        } else {
            // replay the oldest short term frame
            update_fdm( (*short_term.front()) );
            // cout << "oldest short term frame" << endl;
        }
    } else {
        // nothing to replay
    }
}


double FGReplay::get_start_time() {
    if ( long_term.size() > 0 ) {
        return (*long_term.front()).sim_time;
    } else if ( medium_term.size() > 0 ) {
        return (*medium_term.front()).sim_time;
    } else if ( short_term.size() ) {
        return (*short_term.front()).sim_time;
    } else {
        return 0.0;
    }
}

double FGReplay::get_end_time() {
    if ( short_term.size() ) {
        return (*short_term.back()).sim_time;
    } else {
        return 0.0;
    } 
}
