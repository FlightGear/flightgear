// kr-87.cxx -- class to impliment the King KR 87 Digital ADF
//
// Written by Curtis Olson, started April 2002.
//
// Copyright (C) 2002  Curtis L. Olson - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>
#include <Navaids/navlist.hxx>
#include <Time/FGEventMgr.hxx>

#include "kr_87.hxx"

#include <string>
SG_USING_STD(string);

static int play_count = 0;
static time_t last_time = 0;


/**
 * Boy, this is ugly!  Make the VOR range vary by altitude difference.
 */
static double kludgeRange ( double stationElev, double aircraftElev,
			    double nominalRange)
{
				// Assume that the nominal range (usually
				// 50nm) applies at a 5,000 ft difference.
				// Just a wild guess!
  double factor = ((aircraftElev*SG_METER_TO_FEET) - stationElev) / 5000.0;
  double range = fabs(nominalRange * factor);

				// Clamp the range to keep it sane; for
				// now, never less than 25% or more than
				// 500% of nominal range.
  if (range < nominalRange/4.0) {
    range = nominalRange/4.0;
  } else if (range > nominalRange*5.0) {
    range = nominalRange*5.0;
  }

  return range;
}


// Constructor
FGKR_87::FGKR_87() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    need_update(true),
    valid(false),
    freq(0.0),
    stby_freq(0.0),
    on_off_vol_btn(0.0),
    adf_btn(false),
    bfo_btn(false),
    frq_btn(false),
    last_frq_btn(false),
    flt_et_btn(false),
    last_flt_et_btn(false),
    set_rst_btn(false),
    last_set_rst_btn(false),
    goal_needle_deg(0.0),
    needle_deg(0.0),
    flight_timer(0.0),
    elapsed_timer(0.0),
    tmp_timer(0.0),
    ant_mode(0),
    stby_mode(0),
    timer_mode(0),
    count_mode(0)
{
    SGPath path( globals->get_fg_root() );
    SGPath term = path;
    term.append( "Navaids/range.term" );
    SGPath low = path;
    low.append( "Navaids/range.low" );
    SGPath high = path;
    high.append( "Navaids/range.high" );

    term_tbl = new SGInterpTable( term.str() );
    low_tbl = new SGInterpTable( low.str() );
    high_tbl = new SGInterpTable( high.str() );
}


// Destructor
FGKR_87::~FGKR_87() 
{
    delete term_tbl;
    delete low_tbl;
    delete high_tbl;
}


void
FGKR_87::init ()
{
    morse.init();

    update(0);			// FIXME: use dt
}

void
FGKR_87::bind ()
{
				// User inputs
    fgTie("/radios/adf/frequencies/selected-khz", this,
	  &FGKR_87::get_freq, &FGKR_87::set_freq);
    fgSetArchivable("/radios/adf/frequencies/selected-khz");
    fgTie("/radios/adf/frequencies/standby-khz", this,
	  &FGKR_87::get_stby_freq, &FGKR_87::set_stby_freq);
    fgSetArchivable("/radios/adf/frequencies/standby-khz");
    fgTie("/radios/adf/rotation-deg", this,
	  &FGKR_87::get_rotation, &FGKR_87::set_rotation);
    fgSetArchivable("/radios/adf/rotation-deg");
    fgTie("/radios/adf/needle-deg", this,
	  &FGKR_87::get_needle_deg);
    fgTie("/radios/adf/on-off-volume", this,
	  &FGKR_87::get_on_off_vol_btn,
	  &FGKR_87::set_on_off_vol_btn);
    fgSetArchivable("/radios/adf/on-off-volume");
    fgTie("/radios/adf/adf-btn", this,
	  &FGKR_87::get_adf_btn,
	  &FGKR_87::set_adf_btn);
    fgTie("/radios/adf/bfo-btn", this,
	  &FGKR_87::get_bfo_btn,
	  &FGKR_87::set_bfo_btn);
    fgTie("/radios/adf/frq-btn", this,
	  &FGKR_87::get_frq_btn,
	  &FGKR_87::set_frq_btn);
    fgTie("/radios/adf/flt-et-btn", this,
	  &FGKR_87::get_flt_et_btn,
	  &FGKR_87::set_flt_et_btn);
    fgTie("/radios/adf/set-rst-btn", this,
	  &FGKR_87::get_set_rst_btn,
	  &FGKR_87::set_set_rst_btn);
    fgTie("/radios/adf/stby-mode", this,
	  &FGKR_87::get_stby_mode);
    fgTie("/radios/adf/timer-mode", this,
	  &FGKR_87::get_timer_mode);
    fgTie("/radios/adf/count-mode", this,
	  &FGKR_87::get_count_mode);
    fgTie("/radios/adf/ident", this,
	  &FGKR_87::get_ident_btn,
	  &FGKR_87::set_ident_btn);
    fgSetArchivable("/radios/adf/ident");

                                // calculated values
    fgTie("/radios/adf/inrange", this, &FGKR_87::get_inrange);
    fgTie("/radios/adf/heading", this, &FGKR_87::get_heading);
    fgTie("/radios/adf/flight-timer", this, &FGKR_87::get_flight_timer);
    fgTie("/radios/adf/elapsed-timer", this,
          &FGKR_87::get_elapsed_timer,
          &FGKR_87::set_elapsed_timer);
}

void
FGKR_87::unbind ()
{
    fgUntie("/radios/adf/frequencies/selected-khz");
    fgUntie("/radios/adf/frequencies/standby-khz");
    fgUntie("/radios/adf/rotation-deg");
    fgUntie("/radios/adf/needle-deg");
    fgUntie("/radios/adf/on-off-volume");
    fgUntie("/radios/adf/adf-btn");
    fgUntie("/radios/adf/bfo-btn");
    fgUntie("/radios/adf/frq-btn");
    fgUntie("/radios/adf/flt-et-btn");
    fgUntie("/radios/adf/set-rst-btn");
    fgUntie("/radios/adf/timer-mode");
    fgUntie("/radios/adf/count-mode");
    fgUntie("/radios/adf/ident");
    fgUntie("/radios/adf/inrange");
    fgUntie("/radios/adf/heading");
    fgUntie("/radios/adf/flight-timer");
    fgUntie("/radios/adf/elapsed-timer");
}


// Update the various nav values based on position and valid tuned in navs
void 
FGKR_87::update(double dt) 
{
    double acft_lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

    need_update = false;

    Point3D aircraft = sgGeodToCart( Point3D( acft_lon, acft_lat, acft_elev ) );
    Point3D station;
    double az1, az2, s;

    ////////////////////////////////////////////////////////////////////////
    // Radio
    ////////////////////////////////////////////////////////////////////////

    if ( on_off_vol_btn >= 0.01 ) {
        // buttons
        if ( adf_btn == 0 ) {
            ant_mode = 1;
        } else {
            ant_mode = 0;
        }

        if ( frq_btn && frq_btn != last_frq_btn && stby_mode == 0 ) {
            double tmp = freq;
            freq = stby_freq;
            stby_freq = tmp;
        } else if ( frq_btn ) {
            stby_mode = 0;
            count_mode = 0;
        }
        last_frq_btn = frq_btn;

        if ( flt_et_btn && flt_et_btn != last_flt_et_btn ) {
            if ( stby_mode == 0 ) {
                timer_mode = 0;
            } else {
                timer_mode = !timer_mode;
            }
            stby_mode = 1;
        }
        last_flt_et_btn = flt_et_btn;

        if ( set_rst_btn == 1 && set_rst_btn != last_set_rst_btn ) {
            // button depressed
           tmp_timer = 0.0;
        }
        if ( set_rst_btn == 1 && set_rst_btn == last_set_rst_btn ) {
            // button depressed and was last iteration too
            tmp_timer += dt;
            cout << "tmp_timer = " << tmp_timer << endl;
        }
        if ( set_rst_btn == 0 && set_rst_btn != last_set_rst_btn ) {
            // button released
            if ( tmp_timer > 2.0 ) {
                // button held depressed for 2 seconds
                cout << "entering elapsed count down mode" << endl;
                timer_mode = 1;
                count_mode = 2;
                elapsed_timer = 0.0;
            } else if ( count_mode == 2 ) {
                count_mode = 1;
            } else {
                count_mode = 0;
                elapsed_timer = 0.0;
            }
        }
        last_set_rst_btn = set_rst_btn;

        // timers
        flight_timer += dt;

        if ( set_rst_btn == 0 ) {
            // only count if set/rst button not depressed
            if ( count_mode == 0 ) {
                elapsed_timer += dt;
            } else if ( count_mode == 1 ) {
                elapsed_timer -= dt;
                if ( elapsed_timer < 1.0 ) {
                    count_mode = 0;
                    elapsed_timer = 0.0;
                }
            }
        }

        if ( valid ) {
            // staightline distance
            station = Point3D( x, y, z );
            dist = aircraft.distance3D( station );

            // wgs84 heading
            geo_inverse_wgs_84( acft_elev,
                                acft_lat * SGD_RADIANS_TO_DEGREES,
                                acft_lon * SGD_RADIANS_TO_DEGREES, 
                                stn_lat, stn_lon,
                                &az1, &az2, &s );
            heading = az1;
            // cout << " heading = " << heading
            //      << " dist = " << dist << endl;

            effective_range = kludgeRange(stn_elev, acft_elev, range);
            if ( dist < effective_range * SG_NM_TO_METER ) {
                inrange = true;
            } else if ( dist < 2 * effective_range * SG_NM_TO_METER ) {
                inrange = sg_random() < 
                    ( 2 * effective_range * SG_NM_TO_METER - dist ) /
                    (effective_range * SG_NM_TO_METER);
            } else {
                inrange = false;
            }

            if ( inrange ) {
                goal_needle_deg = heading
                    - fgGetDouble("/orientation/heading-deg");
            }
        } else {
            inrange = false;
        }

        if ( ant_mode ) {
            goal_needle_deg = 90.0;
        }
    } else {
        goal_needle_deg = 0.0;
        flight_timer = 0.0;
        elapsed_timer = 0.0;
    }

    cout << "goal = " << goal_needle_deg << "actual = " << needle_deg << endl;
    double diff = goal_needle_deg - needle_deg;
    while ( diff < -180.0 ) { diff += 360.0; }
    while ( diff > 180.0 ) { diff -= 360.0; }
    needle_deg += diff * dt * 4;

    // cout << "flt = " << flight_timer << " et = " << elapsed_timer 
    //      << " needle = " << needle_deg << endl;

#ifdef ENABLE_AUDIO_SUPPORT
    if ( valid && inrange ) {
	// play station ident via audio system if on + ident,
	// otherwise turn it off
	if ( on_off_vol_btn >= 0.01 && ident_btn ) {
	    FGSimpleSound *sound;
	    sound = globals->get_soundmgr()->find( "adf-ident" );
            if ( sound != NULL ) {
                sound->set_volume( on_off_vol_btn );
            } else {
                SG_LOG( SG_COCKPIT, SG_ALERT, "Can't find adf-ident sound" );
            }
	    if ( last_time <
		 globals->get_time_params()->get_cur_time() - 30 ) {
		last_time = globals->get_time_params()->get_cur_time();
		play_count = 0;
	    }
	    if ( play_count < 4 ) {
		// play ADF ident
		if ( !globals->get_soundmgr()->is_playing("adf-ident") ) {
		    globals->get_soundmgr()->play_once( "adf-ident" );
		    ++play_count;
		}
	    }
	} else {
	    globals->get_soundmgr()->stop( "adf-ident" );
	}
    }
#endif
}


// Update current nav/adf radio stations based on current postition
void FGKR_87::search() 
{
    double acft_lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

				// FIXME: the panel should handle this
    FGNav nav;

    static string last_ident = "";

    ////////////////////////////////////////////////////////////////////////
    // ADF.
    ////////////////////////////////////////////////////////////////////////

    if ( current_navlist->query( acft_lon, acft_lat, acft_elev, freq, &nav ) ) {
	char sfreq[128];
	snprintf( sfreq, 10, "%.0f", freq );
	ident = sfreq;
	ident += nav.get_ident();
	cout << "adf ident = " << ident << endl;
	valid = true;
	if ( last_ident != ident ) {
	    last_ident = ident;

	    trans_ident = nav.get_trans_ident();
	    stn_lon = nav.get_lon();
	    stn_lat = nav.get_lat();
	    stn_elev = nav.get_elev();
	    range = nav.get_range();
	    effective_range = kludgeRange(stn_elev, acft_elev, range);
	    x = nav.get_x();
	    y = nav.get_y();
	    z = nav.get_z();

#ifdef ENABLE_AUDIO_SUPPORT
	    if ( globals->get_soundmgr()->exists( "adf-ident" ) ) {
		globals->get_soundmgr()->remove( "adf-ident" );
	    }
	    FGSimpleSound *sound;
	    sound = morse.make_ident( trans_ident, LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    globals->get_soundmgr()->add( sound, "adf-ident" );

	    int offset = (int)(sg_random() * 30.0);
	    play_count = offset / 4;
	    last_time = globals->get_time_params()->get_cur_time() -
		offset;
	    // cout << "offset = " << offset << " play_count = "
	    //      << play_count << " last_time = "
	    //      << last_time << " current time = "
	    //      << globals->get_time_params()->get_cur_time() << endl;
#endif

	    // cout << "Found an adf station in range" << endl;
	    // cout << " id = " << nav.get_ident() << endl;
	}
    } else {
	valid = false;
	ident = "";
	trans_ident = "";
#ifdef ENABLE_AUDIO_SUPPORT
	globals->get_soundmgr()->remove( "adf-ident" );
#endif
	last_ident = "";
	// cout << "not picking up adf. :-(" << endl;
    }
}


double FGKR_87::get_stby_freq() const {
    if ( stby_mode == 0 ) {
        return stby_freq;
    } else {
        if ( timer_mode == 0 ) {
            return flight_timer;
        } else {
            return elapsed_timer;
        }
    }
}
