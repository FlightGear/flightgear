// replay.cxx - a system to record and replay FlightGear flights
//
// Written by Curtis Olson, started Juley 2003.
//
// Copyright (C) 2003  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <simgear/constants.h>

#include <FDM/flight.hxx>
#include <Network/native_ctrls.hxx>
#include <Network/native_fdm.hxx>
#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>

#include "replay.hxx"


/**
 * Constructor
 */

FGReplay::FGReplay() {
}


/**
 * Destructor
 */

FGReplay::~FGReplay() {
    // no dynamically allocated memory to free
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
        short_term.pop_front();
    }
    while ( !medium_term.empty() ) {
        medium_term.pop_front();
    }
    while ( !medium_term.empty() ) {
        medium_term.pop_front();
    }
}


/** 
 * Bind to the property tree
 */

void FGReplay::bind() {
    // nothing to bind
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

    if ( dt <= 0 ) {
        // don't save data if nothing is going on ...

        return;
    }

    sim_time += dt;

    // build the replay record
    FGNetFDM f;
    FGProps2NetFDM( &f, false );

    // sanity check, don't collect data if FDM data isn't good
    if ( !cur_fdm_state->get_inited() ) {
        return;
    }

    FGNetCtrls c;
    FGProps2NetCtrls( &c, false, false );

    FGReplayData r;
    r.sim_time = sim_time;
    r.ctrls = c;
    r.fdm = f;

    // update the short term list
    short_term.push_back( r );

    FGReplayData st_front = short_term.front();
    if ( sim_time - st_front.sim_time > st_list_time ) {
        while ( sim_time - st_front.sim_time > st_list_time ) {
            st_front = short_term.front();
            short_term.pop_front();
        }

        // update the medium term list
        if ( sim_time - last_mt_time > mt_dt ) {
            last_mt_time = sim_time;
            medium_term.push_back( st_front );

            FGReplayData mt_front = medium_term.front();
            if ( sim_time - mt_front.sim_time > mt_list_time ) {
                while ( sim_time - mt_front.sim_time > mt_list_time ) {
                    mt_front = medium_term.front();
                    medium_term.pop_front();
                }

                // update the long term list
                if ( sim_time - last_lt_time > lt_dt ) {
                    last_lt_time = sim_time;
                    long_term.push_back( mt_front );

                    FGReplayData lt_front = long_term.front();
                    if ( sim_time - lt_front.sim_time > lt_list_time ) {
                        while ( sim_time - lt_front.sim_time > lt_list_time ) {
                            lt_front = long_term.front();
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

    double ratio = (time - f1.sim_time) / (f2.sim_time - f1.sim_time);

    cout << fdm1.longitude << " " << fdm2.longitude << endl;
    result.fdm.longitude = weight( fdm1.longitude, fdm2.longitude, ratio );
    result.fdm.latitude = weight( fdm1.latitude, fdm2.latitude, ratio );
    result.fdm.altitude = weight( fdm1.altitude, fdm2.altitude, ratio );
    result.fdm.agl = weight( fdm1.agl, fdm2.agl, ratio );
    result.fdm.phi = weight( fdm1.phi, fdm2.phi, ratio, true );
    result.fdm.theta = weight( fdm1.theta, fdm2.theta, ratio, true );
    result.fdm.psi = weight( fdm1.psi, fdm2.psi, ratio, true );

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

    result.fdm.stall_warning
        = weight( fdm1.stall_warning, fdm2.stall_warning, ratio );

    result.fdm.A_X_pilot = weight( fdm1.A_X_pilot, fdm2.A_X_pilot, ratio );
    result.fdm.A_Y_pilot = weight( fdm1.A_Y_pilot, fdm2.A_Y_pilot, ratio );
    result.fdm.A_Z_pilot = weight( fdm1.A_Z_pilot, fdm2.A_Z_pilot, ratio );

    return result;
}

/** 
 * interpolate a specific time from a specific list
 */
static void interpolate( double time, replay_list_type list ) {
    // sanity checking
    if ( list.size() == 0 ) {
        // handle empty list
        return;
    } else if ( list.size() == 1 ) {
        // handle list size == 1
        update_fdm( list[0] );
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
        } else if ( list[mid].sim_time < time && list[mid+1].sim_time < time ) {
            // too low
            first = mid;
            mid = ( last + first ) / 2;
        } else if ( list[mid].sim_time > time && list[mid+1].sim_time > time ) {
            // too high
            last = mid;
            mid = ( last + first ) / 2;
        } else {
            done = true;
        }
    }

    FGReplayData result = interpolate( time, list[mid], list[mid+1] );

    update_fdm( result );
}


/** 
 *  Replay a saved frame based on time, interpolate from the two
 *  nearest saved frames.
 */

void FGReplay::replay( double time ) {
    cout << "replay: " << time << " ";
    // find the two frames to interpolate between
    double t1, t2;

    if ( short_term.size() > 0 ) {
        t1 = short_term.back().sim_time;
        t2 = short_term.front().sim_time;
        if ( time > t1 ) {
            // replay the most recent frame
            update_fdm( short_term.back() );
            cout << "first frame" << endl;
        } else if ( time <= t1 && time >= t2 ) {
            interpolate( time, short_term );
            cout << "from short term" << endl;
        } else if ( medium_term.size() > 0 ) {
            t1 = short_term.front().sim_time;
            t2 = medium_term.back().sim_time;
            if ( time <= t1 && time >= t2 ) {
                FGReplayData result = interpolate( time,
                                                   medium_term.back(),
                                                   short_term.front() );
                update_fdm( result );
                cout << "from short/medium term" << endl;
            } else {
                t1 = medium_term.back().sim_time;
                t2 = medium_term.front().sim_time;
                if ( time <= t1 && time >= t2 ) {
                    interpolate( time, medium_term );
                    cout << "from medium term" << endl;
                } else if ( long_term.size() > 0 ) {
                    t1 = medium_term.front().sim_time;
                    t2 = long_term.back().sim_time;
                    if ( time <= t1 && time >= t2 ) {
                        FGReplayData result = interpolate( time,
                                                           long_term.back(),
                                                           medium_term.front());
                        update_fdm( result );
                       cout << "from medium/long term" << endl;
                    } else {
                        t1 = long_term.back().sim_time;
                        t2 = long_term.front().sim_time;
                        if ( time <= t1 && time >= t2 ) {
                            interpolate( time, long_term );
                            cout << "from long term" << endl;
                        } else {
                            // replay the oldest long term frame
                            update_fdm( long_term.front() );
                            cout << "oldest long term frame" << endl;
                        }
                    }
                } else {
                    // replay the oldest medium term frame
                    update_fdm( medium_term.front() );
                    cout << "oldest medium term frame" << endl;
                }
            }
        } else {
            // replay the oldest short term frame
            update_fdm( short_term.front() );
            cout << "oldest short term frame" << endl;
        }
    } else {
        // nothing to replay
    }
}


double FGReplay::get_start_time() {
    if ( long_term.size() > 0 ) {
        return long_term.front().sim_time;
    } else if ( medium_term.size() > 0 ) {
        return medium_term.front().sim_time;
    } else if ( short_term.size() ) {
        return short_term.front().sim_time;
    } else {
        return 0.0;
    }
}

double FGReplay::get_end_time() {
    if ( short_term.size() ) {
        return short_term.back().sim_time;
    } else {
        return 0.0;
    } 
}
