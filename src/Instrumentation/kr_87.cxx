// kr-87.cxx -- class to impliment the King KR 87 Digital ADF
//
// Written by Curtis Olson, started April 2002.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Navaids/navlist.hxx>

#include "kr_87.hxx"

#include <Sound/morse.hxx>
#include <string>
using std::string;

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
FGKR_87::FGKR_87( SGPropertyNode *node ) :
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
    freq(0),
    stby_freq(0),
    needle_deg(0.0),
    flight_timer(0.0),
    elapsed_timer(0.0),
    tmp_timer(0.0),
    _time_before_search_sec(0),
    _sgr(NULL)
{
}


// Destructor
FGKR_87::~FGKR_87() {
}


void FGKR_87::init () {
    SGSoundMgr *smgr = globals->get_soundmgr();
    _sgr = smgr->find("avionics", true);
    _sgr->tie_to_listener();
}


void FGKR_87::bind () {
    _tiedProperties.setRoot(fgGetNode("/instrumentation/kr-87", true));
    // internal values
    _tiedProperties.Tie("internal/valid", this, &FGKR_87::get_valid);
    _tiedProperties.Tie("internal/inrange", this,
                        &FGKR_87::get_inrange);
    _tiedProperties.Tie("internal/dist", this,
                        &FGKR_87::get_dist);
    _tiedProperties.Tie("internal/heading", this,
                        &FGKR_87::get_heading);

    // modes
    _tiedProperties.Tie("modes/ant", this,
                        &FGKR_87::get_ant_mode);
    _tiedProperties.Tie("modes/stby", this,
                        &FGKR_87::get_stby_mode);
    _tiedProperties.Tie("modes/timer", this,
                        &FGKR_87::get_timer_mode);
    _tiedProperties.Tie("modes/count", this,
                        &FGKR_87::get_count_mode);

    // input and buttons
    _tiedProperties.Tie("inputs/rotation-deg", this,
                        &FGKR_87::get_rotation, &FGKR_87::set_rotation);
    fgSetArchivable("/instrumentation/kr-87/inputs/rotation-deg");
    _tiedProperties.Tie("inputs/power-btn", this,
                        &FGKR_87::get_power_btn,
                        &FGKR_87::set_power_btn);
    fgSetArchivable("/instrumentation/kr-87/inputs/power-btn");
    _tiedProperties.Tie("inputs/audio-btn", this,
                        &FGKR_87::get_audio_btn,
                        &FGKR_87::set_audio_btn);
    fgSetArchivable("/instrumentation/kr-87/inputs/audio-btn");
    _tiedProperties.Tie("inputs/volume", this,
                        &FGKR_87::get_vol_btn,
                        &FGKR_87::set_vol_btn);
    fgSetArchivable("/instrumentation/kr-87/inputs/volume");
    _tiedProperties.Tie("inputs/adf-btn", this,
                        &FGKR_87::get_adf_btn,
                        &FGKR_87::set_adf_btn);
    _tiedProperties.Tie("inputs/bfo-btn", this,
                        &FGKR_87::get_bfo_btn,
                        &FGKR_87::set_bfo_btn);
    _tiedProperties.Tie("inputs/frq-btn", this,
                        &FGKR_87::get_frq_btn,
                        &FGKR_87::set_frq_btn);
    _tiedProperties.Tie("inputs/flt-et-btn", this,
                        &FGKR_87::get_flt_et_btn,
                        &FGKR_87::set_flt_et_btn);
    _tiedProperties.Tie("inputs/set-rst-btn", this,
                        &FGKR_87::get_set_rst_btn,
                        &FGKR_87::set_set_rst_btn);

    // outputs
    _tiedProperties.Tie("outputs/selected-khz", this,
                        &FGKR_87::get_freq, &FGKR_87::set_freq);
    fgSetArchivable("/instrumentation/kr-87/outputs/selected-khz");
    _tiedProperties.Tie("outputs/standby-khz", this,
                        &FGKR_87::get_stby_freq, &FGKR_87::set_stby_freq);
    fgSetArchivable("/instrumentation/kr-87/outputs/standby-khz");
    _tiedProperties.Tie("outputs/needle-deg", this,
                        &FGKR_87::get_needle_deg);
    _tiedProperties.Tie("outputs/flight-timer", this,
                        &FGKR_87::get_flight_timer);
    _tiedProperties.Tie("outputs/elapsed-timer", this,
                        &FGKR_87::get_elapsed_timer,
                        &FGKR_87::set_elapsed_timer);

    // annunciators
    _tiedProperties.Tie("annunciators/ant", this,
                        &FGKR_87::get_ant_ann );
    _tiedProperties.Tie("annunciators/adf", this,
                        &FGKR_87::get_adf_ann );
    _tiedProperties.Tie("annunciators/bfo", this,
                        &FGKR_87::get_bfo_ann );
    _tiedProperties.Tie("annunciators/frq", this,
                        &FGKR_87::get_frq_ann );
    _tiedProperties.Tie("annunciators/flt", this,
                        &FGKR_87::get_flt_ann );
    _tiedProperties.Tie("annunciators/et", this,
                        &FGKR_87::get_et_ann );
}


void FGKR_87::unbind () {
    _tiedProperties.Untie();
}


// Update the various nav values based on position and valid tuned in navs
void FGKR_87::update( double dt_sec ) {
    SGGeod acft = globals->get_aircraft_position();

    need_update = false;

    double az1, az2, s;

    // On timeout, scan again
    _time_before_search_sec -= dt_sec;
    if ( _time_before_search_sec < 0 ) {
        search();
    }

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
        // cout << "ant_mode = " << ant_mode << endl;

        if ( frq_btn && frq_btn != last_frq_btn && stby_mode == 0 ) {
            int tmp = freq;
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
            tmp_timer += dt_sec;
            // cout << "tmp_timer = " << tmp_timer << endl;
            if ( tmp_timer > 2.0 ) {
                // button held depressed for 2 seconds
                // cout << "entering elapsed count down mode" << endl;
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
        flight_timer += dt_sec;

        if ( set_rst_btn == 0 ) {
            // only count if set/rst button not depressed
            if ( count_mode == 0 ) {
                elapsed_timer += dt_sec;
            } else if ( count_mode == 1 ) {
                elapsed_timer -= dt_sec;
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
            et_flash_time += dt_sec;
            if ( et_ann && et_flash_time > 0.5 ) {
                et_ann = false;
                et_flash_time -= 0.5;
            } else if ( !et_ann && et_flash_time > 0.2 ) {
                et_ann = true;
                et_flash_time -= 0.2;
            }
        }

        if ( valid ) {
            // cout << "adf is valid" << endl;
            // staightline distance
            // What a hack, dist is a class local variable
            dist = sqrt(distSqr(SGVec3d::fromGeod(acft), xyz));

            // wgs84 heading
            geo_inverse_wgs_84( acft, SGGeod::fromDeg(stn_lon, stn_lat),
                                &az1, &az2, &s );
            heading = az1;
            // cout << " heading = " << heading
            //      << " dist = " << dist << endl;

            effective_range = kludgeRange(stn_elev, acft.getElevationFt(), range);
            if ( dist < effective_range * SG_NM_TO_METER ) {
                inrange = true;
            } else if ( dist < 2 * effective_range * SG_NM_TO_METER ) {
                inrange = sg_random() < 
                    ( 2 * effective_range * SG_NM_TO_METER - dist ) /
                    (effective_range * SG_NM_TO_METER);
            } else {
                inrange = false;
            }

            // cout << "inrange = " << inrange << endl;

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
    fgSetString( "/instrumentation/kr-87/outputs/timer-string",
                 formatted_timer );

    while ( goal_needle_deg < 0.0 ) { goal_needle_deg += 360.0; }
    while ( goal_needle_deg >= 360.0 ) { goal_needle_deg -= 360.0; }

    double diff = goal_needle_deg - needle_deg;
    while ( diff < -180.0 ) { diff += 360.0; }
    while ( diff > 180.0 ) { diff -= 360.0; }

    needle_deg += diff * dt_sec * 4;
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
	    sound = _sgr->find( "adf-ident" );
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
		if ( !_sgr->is_playing("adf-ident") && (vol_btn > 0.05) ) {
		    _sgr->play_once( "adf-ident" );
		    ++play_count;
		}
	    }
	} else {
	    _sgr->stop( "adf-ident" );
	}
    }
}


// Update current nav/adf radio stations based on current postition
void FGKR_87::search() {
  SGGeod pos = globals->get_aircraft_position();
  
				// FIXME: the panel should handle this
    static string last_ident = "";

    // reset search time
    _time_before_search_sec = 1.0;

    ////////////////////////////////////////////////////////////////////////
    // ADF.
    ////////////////////////////////////////////////////////////////////////

  
    FGNavRecord *adf = globals->get_navlist()->findByFreq( freq, pos);
    if ( adf != NULL ) {
	char sfreq[128];
	snprintf( sfreq, 10, "%d", freq );
	ident = sfreq;
	ident += adf->get_ident();
        // cout << "adf ident = " << ident << endl;
	valid = true;
	if ( last_ident != ident ) {
	    last_ident = ident;

	    trans_ident = adf->get_trans_ident();
	    stn_lon = adf->get_lon();
	    stn_lat = adf->get_lat();
	    stn_elev = adf->get_elev_ft();
	    range = adf->get_range();
	    effective_range = kludgeRange(stn_elev, pos.getElevationM(), range);
	    xyz = adf->cart();

	    if ( _sgr->exists( "adf-ident" ) ) {
	        // stop is required! -- remove alone wouldn't stop immediately
	        _sgr->stop( "adf-ident" );
	        _sgr->remove( "adf-ident" );
	    }
	    SGSoundSample *sound;
        sound = FGMorse::instance()->make_ident( trans_ident, FGMorse::LO_FREQUENCY );
	    sound->set_volume( 0.3 );
	    _sgr->add( sound, "adf-ident" );

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
	_sgr->remove( "adf-ident" );
	last_ident = "";
	// cout << "not picking up adf. :-(" << endl;
    }
}


int FGKR_87::get_stby_freq() const {
    if ( stby_mode == 0 ) {
        return stby_freq;
    } else {
        if ( timer_mode == 0 ) {
            return (int)flight_timer;
        } else {
            return (int)elapsed_timer;
        }
    }
}
