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
  double factor = (aircraftElev - stationElev)*SG_METER_TO_FEET / 5000.0;
  double range = fabs(nominalRange * factor);

				// Clamp the range to keep it sane; for
				// now, never less than 50% or more than
				// 500% of nominal range.
  if (range < nominalRange/2.0) {
    range = nominalRange/2.0;
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
    bus_power(fgGetNode("/systems/electrical/outputs/adf", true)),
    serviceable(fgGetNode("/instrumentation/adf/serviceable", true)),
    need_update(true),
    valid(false),
    inrange(false),
    dist(0.0),
    heading(0.0),
    goal_needle_deg(0.0),
    et_flash_time(0.0),
    ant_mode(0),
    stby_mode(0),
    timer_mode(0),
    count_mode(0),
    rotation(0),
    power_btn(true),
    audio_btn(true),
    vol_btn(0.5),
    adf_btn(true),
    bfo_btn(false),
    frq_btn(false),
    last_frq_btn(false),
    flt_et_btn(false),
    last_flt_et_btn(false),
    set_rst_btn(false),
    last_set_rst_btn(false),
    freq(0.0),
    stby_freq(0.0),
    needle_deg(0.0),
    flight_timer(0.0),
    elapsed_timer(0.0),
    tmp_timer(0.0)
{
}


// Destructor
FGKR_87::~FGKR_87() {
}


void FGKR_87::init () {
    serviceable->setBoolValue( true );
    morse.init();
}


void FGKR_87::bind () {
    // internal values
    fgTie("/radios/kr-87/internal/valid", this, &FGKR_87::get_valid);
    fgTie("/radios/kr-87/internal/inrange", this, &FGKR_87::get_inrange);
    fgTie("/radios/kr-87/internal/dist", this, &FGKR_87::get_dist);
    fgTie("/radios/kr-87/internal/heading", this, &FGKR_87::get_heading);

    // modes
    fgTie("/radios/kr-87/modes/ant", this,
	  &FGKR_87::get_ant_mode);
    fgTie("/radios/kr-87/modes/stby", this,
	  &FGKR_87::get_stby_mode);
    fgTie("/radios/kr-87/modes/timer", this,
	  &FGKR_87::get_timer_mode);
    fgTie("/radios/kr-87/modes/count", this,
	  &FGKR_87::get_count_mode);

    // input and buttons
    fgTie("/radios/kr-87/inputs/rotation-deg", this,
	  &FGKR_87::get_rotation, &FGKR_87::set_rotation);
    fgSetArchivable("/radios/kr-87/inputs/rotation-deg");
    fgTie("/radios/kr-87/inputs/power-btn", this,
	  &FGKR_87::get_power_btn,
	  &FGKR_87::set_power_btn);
    fgSetArchivable("/radios/kr-87/inputs/power-btn");
    fgTie("/radios/kr-87/inputs/audio-btn", this,
	  &FGKR_87::get_audio_btn,
	  &FGKR_87::set_audio_btn);
    fgSetArchivable("/radios/kr-87/inputs/audio-btn");
    fgTie("/radios/kr-87/inputs/volume", this,
	  &FGKR_87::get_vol_btn,
	  &FGKR_87::set_vol_btn);
    fgSetArchivable("/radios/kr-87/inputs/volume");
    fgTie("/radios/kr-87/inputs/adf-btn", this,
	  &FGKR_87::get_adf_btn,
	  &FGKR_87::set_adf_btn);
    fgTie("/radios/kr-87/inputs/bfo-btn", this,
	  &FGKR_87::get_bfo_btn,
	  &FGKR_87::set_bfo_btn);
    fgTie("/radios/kr-87/inputs/frq-btn", this,
	  &FGKR_87::get_frq_btn,
	  &FGKR_87::set_frq_btn);
    fgTie("/radios/kr-87/inputs/flt-et-btn", this,
	  &FGKR_87::get_flt_et_btn,
	  &FGKR_87::set_flt_et_btn);
    fgTie("/radios/kr-87/inputs/set-rst-btn", this,
	  &FGKR_87::get_set_rst_btn,
	  &FGKR_87::set_set_rst_btn);

    // outputs
    fgTie("/radios/kr-87/outputs/selected-khz", this,
	  &FGKR_87::get_freq, &FGKR_87::set_freq);
    fgSetArchivable("/radios/kr-87/outputs/selected-khz");
    fgTie("/radios/kr-87/outputs/standby-khz", this,
	  &FGKR_87::get_stby_freq, &FGKR_87::set_stby_freq);
    fgSetArchivable("/radios/kr-87/outputs/standby-khz");
    fgTie("/radios/kr-87/outputs/needle-deg", this,
	  &FGKR_87::get_needle_deg);
    fgTie("/radios/kr-87/outputs/flight-timer", this, &FGKR_87::get_flight_timer);
    fgTie("/radios/kr-87/outputs/elapsed-timer", this,
          &FGKR_87::get_elapsed_timer,
          &FGKR_87::set_elapsed_timer);

    // annunciators
    fgTie("/radios/kr-87/annunciators/ant", this, &FGKR_87::get_ant_ann );
    fgTie("/radios/kr-87/annunciators/adf", this, &FGKR_87::get_adf_ann );
    fgTie("/radios/kr-87/annunciators/bfo", this, &FGKR_87::get_bfo_ann );
    fgTie("/radios/kr-87/annunciators/frq", this, &FGKR_87::get_frq_ann );
    fgTie("/radios/kr-87/annunciators/flt", this, &FGKR_87::get_flt_ann );
    fgTie("/radios/kr-87/annunciators/et", this, &FGKR_87::get_et_ann );
}


void FGKR_87::unbind () {
    // internal values
    fgUntie("/radios/kr-87/internal/valid");
    fgUntie("/radios/kr-87/internal/inrange");
    fgUntie("/radios/kr-87/internal/dist");
    fgUntie("/radios/kr-87/internal/heading");

    // modes
    fgUntie("/radios/kr-87/modes/ant");
    fgUntie("/radios/kr-87/modes/stby");
    fgUntie("/radios/kr-87/modes/timer");
    fgUntie("/radios/kr-87/modes/count");

    // input and buttons
    fgUntie("/radios/kr-87/inputs/rotation-deg");
    fgUntie("/radios/kr-87/inputs/power-btn");
    fgUntie("/radios/kr-87/inputs/volume");
    fgUntie("/radios/kr-87/inputs/adf-btn");
    fgUntie("/radios/kr-87/inputs/bfo-btn");
    fgUntie("/radios/kr-87/inputs/frq-btn");
    fgUntie("/radios/kr-87/inputs/flt-et-btn");
    fgUntie("/radios/kr-87/inputs/set-rst-btn");
    fgUntie("/radios/kr-87/inputs/ident-btn");

    // outputs
    fgUntie("/radios/kr-87/outputs/selected-khz");
    fgUntie("/radios/kr-87/outputs/standby-khz");
    fgUntie("/radios/kr-87/outputs/needle-deg");
    fgUntie("/radios/kr-87/outputs/flight-timer");
    fgUntie("/radios/kr-87/outputs/elapsed-timer");

    // annunciators
    fgUntie("/radios/kr-87/annunciators/ant");
    fgUntie("/radios/kr-87/annunciators/adf");
    fgUntie("/radios/kr-87/annunciators/bfo");
    fgUntie("/radios/kr-87/annunciators/frq");
    fgUntie("/radios/kr-87/annunciators/flt");
    fgUntie("/radios/kr-87/annunciators/et");
}


// Update the various nav values based on position and valid tuned in navs
void FGKR_87::update( double dt ) {
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

    if ( has_power() && serviceable->getBoolValue() ) {
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
            // cout << "tmp_timer = " << tmp_timer << endl;
            if ( tmp_timer > 2.0 ) {
                // button held depressed for 2 seconds
                cout << "entering elapsed count down mode" << endl;
                timer_mode = 1;
                count_mode = 2;
                elapsed_timer = 0.0;
            }    
        }
        if ( set_rst_btn == 0 && set_rst_btn != last_set_rst_btn ) {
            // button released
            if ( tmp_timer > 2.0 ) {
                // button held depressed for 2 seconds, don't adjust
                // mode, just exit                
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

        // annunciators
        ant_ann = !adf_btn;
        adf_ann = adf_btn;
        bfo_ann = bfo_btn;
        frq_ann = !stby_mode;
        flt_ann = stby_mode && !timer_mode;
        if ( count_mode < 2 ) {
            et_ann = stby_mode && timer_mode;
        } else {
            et_flash_time += dt;
            if ( et_ann && et_flash_time > 0.5 ) {
                et_ann = false;
                et_flash_time -= 0.5;
            } else if ( !et_ann && et_flash_time > 0.2 ) {
                et_ann = true;
                et_flash_time -= 0.2;
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
        // unit turned off
        goal_needle_deg = 0.0;
        flight_timer = 0.0;
        elapsed_timer = 0.0;
        ant_ann = false;
        adf_ann = false;
        bfo_ann = false;
        frq_ann = false;
        flt_ann = false;
        et_ann = false;
    }

    // formatted timer
    double time;
    int hours, min, sec;
    if ( timer_mode == 0 ) {
        time = flight_timer;
    } else {
        time = elapsed_timer;
    }
    // cout << time << endl;
    hours = (int)(time / 3600.0);
    time -= hours * 3600.00;
    min = (int)(time / 60.0);
    time -= min * 60.0;
    sec = (int)time;
    int big, little;
    if ( hours > 0 ) {
        big = hours;
        if ( big > 99 ) {
            big = 99;
        }
        little = min;
    } else {
        big = min;
        little = sec;
    }
    if ( big > 99 ) {
        big = 99;
    }
    char formatted_timer[128];
    // cout << big << ":" << little << endl;
    snprintf(formatted_timer, 6, "%02d:%02d", big, little);
    fgSetString( "/radios/kr-87/outputs/timer-string", formatted_timer );

    while ( goal_needle_deg < 0.0 ) { goal_needle_deg += 360.0; }
    while ( goal_needle_deg >= 360.0 ) { goal_needle_deg -= 360.0; }

    double diff = goal_needle_deg - needle_deg;
    while ( diff < -180.0 ) { diff += 360.0; }
    while ( diff > 180.0 ) { diff -= 360.0; }

    needle_deg += diff * dt * 4;
    while ( needle_deg < 0.0 ) { needle_deg += 360.0; }
    while ( needle_deg >= 360.0 ) { needle_deg -= 360.0; }

    // cout << "goal = " << goal_needle_deg << " actual = " << needle_deg
    //      << endl;
    // cout << "flt = " << flight_timer << " et = " << elapsed_timer 
    //      << " needle = " << needle_deg << endl;

    if ( valid && inrange && serviceable->getBoolValue() ) {
	// play station ident via audio system if on + ant mode,
	// otherwise turn it off
	if ( vol_btn >= 0.01 && audio_btn ) {
	    SGSoundSample *sound;
	    sound = globals->get_soundmgr()->find( "adf-ident" );
            if ( sound != NULL ) {
                if ( !adf_btn ) {
                    sound->set_volume( vol_btn );
                } else {
                    sound->set_volume( vol_btn / 4.0 );
                }
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
}


// Update current nav/adf radio stations based on current postition
void FGKR_87::search() {
    double acft_lon = lon_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_lat = lat_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
    double acft_elev = alt_node->getDoubleValue() * SG_FEET_TO_METER;

				// FIXME: the panel should handle this
    static string last_ident = "";

    ////////////////////////////////////////////////////////////////////////
    // ADF.
    ////////////////////////////////////////////////////////////////////////

    FGNav *nav
        = current_navlist->findByFreq(freq, acft_lon, acft_lat, acft_elev);

    if ( nav != NULL ) {
	char sfreq[128];
	snprintf( sfreq, 10, "%.0f", freq );
	ident = sfreq;
	ident += nav->get_ident();
        // cout << "adf ident = " << ident << endl;
	valid = true;
	if ( last_ident != ident ) {
	    last_ident = ident;

	    trans_ident = nav->get_trans_ident();
	    stn_lon = nav->get_lon();
	    stn_lat = nav->get_lat();
	    stn_elev = nav->get_elev_ft();
	    range = nav->get_range();
	    effective_range = kludgeRange(stn_elev, acft_elev, range);
	    x = nav->get_x();
	    y = nav->get_y();
	    z = nav->get_z();

	    if ( globals->get_soundmgr()->exists( "adf-ident" ) ) {
		globals->get_soundmgr()->remove( "adf-ident" );
	    }
	    SGSoundSample *sound;
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

	    // cout << "Found an adf station in range" << endl;
	    // cout << " id = " << nav->get_ident() << endl;
	}
    } else {
	valid = false;
	ident = "";
	trans_ident = "";
	globals->get_soundmgr()->remove( "adf-ident" );
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
