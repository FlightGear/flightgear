// atc610x.cxx -- FGFS interface to ATC 610x hardware
//
// Written by Curtis Olson, started January 2002
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

#include <simgear/compiler.h>

#include <stdlib.h>		// atoi() atof() abs()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>              //snprintf
#if defined( _MSC_VER ) || defined(__MINGW32__)
#  include <io.h>                 //lseek, read, write
#endif

#include STL_STRING

#include <plib/ul.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "atc610x.hxx"

SG_USING_STD(string);


// Lock the ATC 610 hardware
static int ATC610xLock( int fd ) {
    // rewind
    lseek( fd, 0, SEEK_SET );

    char tmp[2];
    int result = read( fd, tmp, 1 );
    if ( result != 1 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Lock failed" );
    }

    return result;
}


// Write a radios command
static int ATC610xRelease( int fd ) {
    // rewind
    lseek( fd, 0, SEEK_SET );

    char tmp[2];
    tmp[0] = tmp[1] = 0;
    int result = write( fd, tmp, 1 );

    if ( result != 1 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Release failed" );
    }

    return result;
}


// Read analog inputs
static void ATC610xReadAnalogInputs( int fd, unsigned char *analog_in_bytes ) {
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = read( fd, analog_in_bytes, ATC_ANAL_IN_BYTES );
    if ( result != ATC_ANAL_IN_BYTES ) {
	SG_LOG( SG_IO, SG_ALERT, "Read failed" );
	exit( -1 );
    }
}


// Write a radios command
static int ATC610xSetRadios( int fd,
			     unsigned char data[ATC_RADIO_DISPLAY_BYTES] )
{
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = write( fd, data, ATC_RADIO_DISPLAY_BYTES );

    if ( result != ATC_RADIO_DISPLAY_BYTES ) {
	SG_LOG( SG_IO, SG_DEBUG, "Write failed" );
    }

    return result;
}


// Read status of last radios written to
static void ATC610xReadRadios( int fd, unsigned char *switch_data ) {
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = read( fd, switch_data, ATC_RADIO_SWITCH_BYTES );
    if ( result != ATC_RADIO_SWITCH_BYTES ) {
	SG_LOG( SG_IO, SG_ALERT, "Read failed" );
	exit( -1 );
    }
}

// Write a stepper command
static int ATC610xSetStepper( int fd, unsigned char channel,
			      unsigned char value )
{
    // rewind
    lseek( fd, 0, SEEK_SET );

    // Write the value
    unsigned char buf[3];
    buf[0] = channel;
    buf[1] = value;
    buf[2] = 0;
    int result = write( fd, buf, 2 );
    if ( result != 2 ) {
	SG_LOG( SG_IO, SG_INFO, "Write failed" );
    }
    SG_LOG( SG_IO, SG_DEBUG,
	    "Sent cmd = " << (int)channel << " value = " << (int)value );
    return result;
}


// Read status of last stepper written to
static unsigned char ATC610xReadStepper( int fd ) {
    int result;

    // rewind
    lseek( fd, 0, SEEK_SET );

    // Write the value
    unsigned char buf[2];
    result = read( fd, buf, 1 );
    if ( result != 1 ) {
	SG_LOG( SG_IO, SG_ALERT, "Read failed" );
	exit( -1 );
    }
    SG_LOG( SG_IO, SG_DEBUG, "Read result = " << (int)buf[0] );

    return buf[0];
}


// Read switch inputs
static void ATC610xReadSwitches( int fd, unsigned char *switch_bytes ) {
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = read( fd, switch_bytes, ATC_SWITCH_BYTES );
    if ( result != ATC_SWITCH_BYTES ) {
	SG_LOG( SG_IO, SG_ALERT, "Read failed" );
	exit( -1 );
    }
}


// Turn a lamp on or off
void ATC610xSetLamp( int fd, int channel, bool value ) {
    // lamp channels 0-63 are written to LampPort0, channels 64-127
    // are written to LampPort1

    // bits 0-6 are the lamp address
    // bit 7 is the value (on/off)

    int result;

    // Write the value
    unsigned char buf[3];
    buf[0] = channel;
    buf[1] = value;
    buf[2] = 0;
    result = write( fd, buf, 2 );
    if ( result != 2 ) {
        SG_LOG( SG_IO, SG_ALERT,  "Write failed" );
        exit( -1 );
    }
}


void FGATC610x::init_config() {
#if defined( unix ) || defined( __CYGWIN__ )
    // Next check home directory for .fgfsrc.hostname file
    char *envp = ::getenv( "HOME" );
    if ( envp != NULL ) {
        SGPath atc610x_config( envp );
        atc610x_config.append( ".fgfs-atc610x.xml" );
        readProperties( atc610x_config.str(), globals->get_props() );
    }
#endif
}


// Open and initialize ATC 610x hardware
bool FGATC610x::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    // This loads the config parameters generated by "simcal"
    init_config();

    SG_LOG( SG_IO, SG_ALERT,
	    "Initializing ATC 610x hardware, please wait ..." );

    set_hz( 30 );		// default to processing requests @ 30Hz
    set_enabled( true );

    board = 0;			// 610x uses a single board number = 0

    snprintf( lock_file, 256, "/proc/atc610x/board%d/lock", board );
    snprintf( analog_in_file, 256, "/proc/atc610x/board%d/analog_in", board );
    snprintf( lamps_file, 256, "/proc/atc610x/board%d/lamps", board );
    snprintf( radios_file, 256, "/proc/atc610x/board%d/radios", board );
    snprintf( stepper_file, 256, "/proc/atc610x/board%d/steppers", board );
    snprintf( switches_file, 256, "/proc/atc610x/board%d/switches", board );

    /////////////////////////////////////////////////////////////////////
    // Open the /proc files
    /////////////////////////////////////////////////////////////////////

    lock_fd = ::open( lock_file, O_RDWR );
    if ( lock_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", lock_file );
	perror( msg );
	exit( -1 );
    }

    analog_in_fd = ::open( analog_in_file, O_RDONLY );
    if ( analog_in_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", analog_in_file );
	perror( msg );
	exit( -1 );
    }

    lamps_fd = ::open( lamps_file, O_WRONLY );
    if ( lamps_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", lamps_file );
	perror( msg );
	exit( -1 );
    }

    radios_fd = ::open( radios_file, O_RDWR );
    if ( radios_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", radios_file );
	perror( msg );
	exit( -1 );
    }

    stepper_fd = ::open( stepper_file, O_RDWR );
    if ( stepper_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", stepper_file );
	perror( msg );
	exit( -1 );
    }

    switches_fd = ::open( switches_file, O_RDONLY );
    if ( switches_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", switches_file );
	perror( msg );
	exit( -1 );
    }

    /////////////////////////////////////////////////////////////////////
    // Home the compass stepper motor
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "  - Homing the compass stepper motor" );

    // Lock the hardware, keep trying until we succeed
    while ( ATC610xLock( lock_fd ) <= 0 );

    // Send the stepper home command
    ATC610xSetStepper( stepper_fd, ATC_COMPASS_CH, ATC_STEPPER_HOME );

    // Release the hardware
    ATC610xRelease( lock_fd );

    SG_LOG( SG_IO, SG_ALERT,
	    "  - Waiting for compass to come home." );

    bool home = false;
    int timeout = 900;          // about 30 seconds
    timeout = 0;
    while ( ! home && timeout > 0 ) {
        if ( timeout % 150 == 0 ) {
            SG_LOG( SG_IO, SG_INFO, "waiting for compass = " << timeout );
        } else {
            SG_LOG( SG_IO, SG_DEBUG, "Checking if compass home ..." );
        }

	while ( ATC610xLock( lock_fd ) <= 0 );

	unsigned char result = ATC610xReadStepper( stepper_fd );
	if ( result == 0 ) {
	    home = true;
	}

	ATC610xRelease( lock_fd );

#if defined( _MSC_VER )
	ulMilliSecondSleep(33);
#elif defined (WIN32) && !defined(__CYGWIN__)
        Sleep (33);
#else
	usleep(33);
#endif

        --timeout;
    }

    compass_position = 0.0;

    /////////////////////////////////////////////////////////////////////
    // Blank the radio display
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "  - Clearing the radios displays." );

    // Prepair the data
    unsigned char value = 0xff;
    for ( int channel = 0; channel < ATC_RADIO_DISPLAY_BYTES; ++channel ) {
	radio_display_data[channel] = value;
    }

    // Lock the hardware, keep trying until we succeed
    while ( ATC610xLock( lock_fd ) <= 0 );

    // Set radio display
    ATC610xSetRadios( radios_fd, radio_display_data );

    ATC610xRelease( lock_fd );

    /////////////////////////////////////////////////////////////////////
    // Blank the lamps
    /////////////////////////////////////////////////////////////////////

    for ( int i = 0; i < 128; ++i ) {
        ATC610xSetLamp( lamps_fd, i, false );
    }

    /////////////////////////////////////////////////////////////////////
    // Finished initing hardware
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "Done initializing ATC 610x hardware." );

    /////////////////////////////////////////////////////////////////////
    // Connect up to property values
    /////////////////////////////////////////////////////////////////////

    mag_compass = fgGetNode( "/instrumentation/magnetic-compass/indicated-heading-deg", true );

    dme_min = fgGetNode( "/instrumentation/dme/indicated-time-min", true );
    dme_kt = fgGetNode( "/instrumentation/dme/indicated-ground-speed-kt",
                        true );
    dme_nm = fgGetNode( "/instrumentation/dme/indicated-distance-nm", true );
    dme_in_range = fgGetNode( "/instrumentation/dme/in-range", true );

    adf_bus_power = fgGetNode( "/systems/electrical/outputs/adf", true );
    dme_bus_power = fgGetNode( "/systems/electrical/outputs/dme", true );
    navcom1_bus_power = fgGetNode( "/systems/electrical/outputs/navcom[0]",
                                   true );
    navcom2_bus_power = fgGetNode( "/systems/electrical/outputs/navcom[1]",
                                   true );
    xpdr_bus_power = fgGetNode( "/systems/electrical/outputs/transponder",
                                 true );

    navcom1_power_btn = fgGetNode( "/radios/comm[0]/inputs/power-btn", true );
    navcom2_power_btn = fgGetNode( "/radios/comm[1]/inputs/power-btn", true );

    com1_freq = fgGetNode( "/radios/comm[0]/frequencies/selected-mhz", true );
    com1_stby_freq
	= fgGetNode( "/radios/comm[0]/frequencies/standby-mhz", true );

    com2_freq = fgGetNode( "/radios/comm[1]/frequencies/selected-mhz", true );
    com2_stby_freq
	= fgGetNode( "/radios/comm[1]/frequencies/standby-mhz", true );

    nav1_freq = fgGetNode( "/radios/nav[0]/frequencies/selected-mhz", true );
    nav1_stby_freq
	= fgGetNode( "/radios/nav[0]/frequencies/standby-mhz", true );
    nav1_obs = fgGetNode( "/radios/nav[0]/radials/selected-deg", true );

    nav2_freq = fgGetNode( "/radios/nav[1]/frequencies/selected-mhz", true );
    nav2_stby_freq
	= fgGetNode( "/radios/nav[1]/frequencies/standby-mhz", true );
    nav2_obs = fgGetNode( "/radios/nav[1]/radials/selected-deg", true );

    adf_power_btn = fgGetNode( "/radios/kr-87/inputs/power-btn", true );
    adf_vol = fgGetNode( "/radios/kr-87/inputs/volume", true );
    adf_adf_btn = fgGetNode( "/radios/kr-87/inputs/adf-btn", true );
    adf_bfo_btn = fgGetNode( "/radios/kr-87/inputs/bfo-btn", true );
    adf_freq = fgGetNode( "/radios/kr-87/outputs/selected-khz", true );
    adf_stby_freq = fgGetNode( "/radios/kr-87/outputs/standby-khz", true );
    adf_stby_mode = fgGetNode( "/radios/kr-87/modes/stby", true );
    adf_timer_mode = fgGetNode( "/radios/kr-87/modes/timer", true );
    adf_count_mode = fgGetNode( "/radios/kr-87/modes/count", true );
    adf_flight_timer = fgGetNode( "/radios/kr-87/outputs/flight-timer", true );
    adf_elapsed_timer = fgGetNode( "/radios/kr-87/outputs/elapsed-timer",
                                   true );
    adf_ant_ann = fgGetNode( "/radios/kr-87/annunciators/ant", true );
    adf_adf_ann = fgGetNode( "/radios/kr-87/annunciators/adf", true );
    adf_bfo_ann = fgGetNode( "/radios/kr-87/annunciators/bfo", true );
    adf_frq_ann = fgGetNode( "/radios/kr-87/annunciators/frq", true );
    adf_flt_ann = fgGetNode( "/radios/kr-87/annunciators/flt", true );
    adf_et_ann = fgGetNode( "/radios/kr-87/annunciators/et", true );

    inner = fgGetNode( "/radios/marker-beacon/inner", true );
    middle = fgGetNode( "/radios/marker-beacon/middle", true );
    outer = fgGetNode( "/radios/marker-beacon/outer", true );

    xpdr_ident_btn = fgGetNode( "/radios/kt-70/inputs/ident-btn", true );
    xpdr_digit1 = fgGetNode( "/radios/kt-70/inputs/digit1", true );
    xpdr_digit2 = fgGetNode( "/radios/kt-70/inputs/digit2", true );
    xpdr_digit3 = fgGetNode( "/radios/kt-70/inputs/digit3", true );
    xpdr_digit4 = fgGetNode( "/radios/kt-70/inputs/digit4", true );
    xpdr_func_knob = fgGetNode( "/radios/kt-70/inputs/func-knob", true );
    xpdr_id_code = fgGetNode( "/radios/kt-70/outputs/id-code", true );
    xpdr_flight_level = fgGetNode( "/radios/kt-70/outputs/flight-level", true );
    xpdr_fl_ann = fgGetNode( "/radios/kt-70/annunciators/fl", true );
    xpdr_alt_ann = fgGetNode( "/radios/kt-70/annunciators/alt", true );
    xpdr_gnd_ann = fgGetNode( "/radios/kt-70/annunciators/gnd", true );
    xpdr_on_ann = fgGetNode( "/radios/kt-70/annunciators/on", true );
    xpdr_sby_ann = fgGetNode( "/radios/kt-70/annunciators/sby", true );
    xpdr_reply_ann = fgGetNode( "/radios/kt-70/annunciators/reply", true );

    ati_bird
      = fgGetNode( "/instrumentation/attitude-indicator/horizon-offset-deg",
		   true );
    alt_press = fgGetNode( "/instrumentation/altimeter/setting-inhg", true );
    adf_hdg = fgGetNode( "/radios/kr-87/inputs/rotation-deg", true );
    hdg_bug = fgGetNode( "/autopilot/settings/heading-bug-deg", true );

    elevator_center = fgGetNode( "/input/atc610x/elevator/center", true );
    elevator_min = fgGetNode( "/input/atc610x/elevator/min", true );
    elevator_max = fgGetNode( "/input/atc610x/elevator/max", true );

    ailerons_center = fgGetNode( "/input/atc610x/ailerons/center", true );
    ailerons_min = fgGetNode( "/input/atc610x/ailerons/min", true );
    ailerons_max = fgGetNode( "/input/atc610x/ailerons/max", true );

    rudder_center = fgGetNode( "/input/atc610x/rudder/center", true );
    rudder_min = fgGetNode( "/input/atc610x/rudder/min", true );
    rudder_max = fgGetNode( "/input/atc610x/rudder/max", true );

    brake_left_min = fgGetNode( "/input/atc610x/brake-left/min", true );
    brake_left_max = fgGetNode( "/input/atc610x/brake-left/max", true );

    brake_right_min = fgGetNode( "/input/atc610x/brake-right/min", true );
    brake_right_max = fgGetNode( "/input/atc610x/brake-right/max", true );

    throttle_min = fgGetNode( "/input/atc610x/throttle/min", true );
    throttle_max = fgGetNode( "/input/atc610x/throttle/max", true );

    mixture_min = fgGetNode( "/input/atc610x/mixture/min", true );
    mixture_max = fgGetNode( "/input/atc610x/mixture/max", true );

    trim_center = fgGetNode( "/input/atc610x/trim/center", true );
    trim_min = fgGetNode( "/input/atc610x/trim/min", true );
    trim_max = fgGetNode( "/input/atc610x/trim/max", true );

    nav1vol_min = fgGetNode( "/input/atc610x/nav1vol/min", true );
    nav1vol_max = fgGetNode( "/input/atc610x/nav1vol/max", true );

    nav2vol_min = fgGetNode( "/input/atc610x/nav2vol/min", true );
    nav2vol_max = fgGetNode( "/input/atc610x/nav2vol/max", true );

    ignore_flight_controls
        = fgGetNode( "/input/atc610x/ignore-flight-controls", true );

    comm1_serviceable = fgGetNode( "/instrumentation/comm[0]/serviceable", true );
    comm2_serviceable = fgGetNode( "/instrumentation/comm[1]/serviceable", true );
    nav1_serviceable = fgGetNode( "/instrumentation/nav[0]/serviceable", true );
    nav2_serviceable = fgGetNode( "/instrumentation/nav[1]/serviceable", true );
    adf_serviceable = fgGetNode( "/instrumentation/adf/serviceable", true );
    xpdr_serviceable = fgGetNode( "/radios/kt-70/inputs/serviceable",
                                 true );
    dme_serviceable = fgGetNode( "/instrumentation/dme/serviceable", true );

    // default to having everything serviceable
    comm1_serviceable->setBoolValue( true );
    comm2_serviceable->setBoolValue( true );
    nav1_serviceable->setBoolValue( true );
    nav2_serviceable->setBoolValue( true );
    adf_serviceable->setBoolValue( true );
    xpdr_serviceable->setBoolValue( true );
    dme_serviceable->setBoolValue( true );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Read analog inputs
/////////////////////////////////////////////////////////////////////

// scale a number between min and max (with center defined) to a scale
// from -1.0 to 1.0
static double scale( int center, int min, int max, int value ) {
    // cout << center << " " << min << " " << max << " " << value << " ";
    double result;
    double range;

    if ( value <= center ) {
        range = center - min;
        result = (value - center) / range;
    } else {
        range = max - center;
        result = (value - center) / range;            
    }

    if ( result < -1.0 ) result = -1.0;
    if ( result > 1.0 ) result = 1.0;

    // cout << result << endl;

    return result;
}


// scale a number between min and max to a scale from 0.0 to 1.0
static double scale( int min, int max, int value ) {
    // cout << center << " " << min << " " << max << " " << value << " ";
    double result;
    double range;

    range = max - min;
    result = (value - min) / range;

    if ( result < 0.0 ) result = 0.0;
    if ( result > 1.0 ) result = 1.0;

    // cout << result << endl;

    return result;
}


static int tony_magic( int raw, int obs[3] ) {
    int result = 0;

    obs[0] = raw;

    if ( obs[1] < 30 ) {
        if ( obs[2] >= 68 && obs[2] < 480 ) {
            result = -6;
        } else if ( obs[2] >= 480 ) {
            result = 6;
        }
        obs[2] = obs[1];
        obs[1] = obs[0];
    } else if ( obs[1] < 68 ) {
        // do nothing
        obs[1] = obs[0];
    } else if ( obs[2] < 30 ) {
        if ( obs[1] >= 68 && obs[1] < 480 ) {
            result = 6;
	    obs[2] = obs[1];
	    obs[1] = obs[0];
        } else if ( obs[1] >= 480 ) {
	    result = -6;
            if ( obs[0] < obs[1] ) {
		obs[2] = obs[1];
		obs[1] = obs[0];
	    } else {
	        obs[2] = obs[0];
		obs[1] = obs[0];
	    }
        }
    } else if ( obs[1] > 980 ) {
        if ( obs[2] <= 956 && obs[2] > 480 ) {
            result = 6;
        } else if ( obs[2] <= 480 ) {
            result = -6;
        }
        obs[2] = obs[1];
        obs[1] = obs[0];
    } else if ( obs[1] > 956 ) {
        // do nothing
        obs[1] = obs[0];
    } else if ( obs[2] > 980 ) {
        if ( obs[1] <= 956 && obs[1] > 480 ) {
            result = -6;
	    obs[2] = obs[1];
	    obs[1] = obs[0];
        } else if ( obs[1] <= 480 ) {
	    result = 6;
            if ( obs[0] > obs[1] ) {
		obs[2] = obs[1];
		obs[1] = obs[0];
	    } else {
		obs[2] = obs[0];
		obs[1] = obs[0];
	    }
        }
    } else {
        if ( obs[1] < 480 && obs[2] > 480 ) {
	    // crossed gap going up
	    if ( obs[0] < obs[1] ) {
	        // caught a bogus intermediate value coming out of the gap
	        obs[1] = obs[0];
	    }
	} else if ( obs[1] > 480 && obs[2] < 480 ) {
	    // crossed gap going down
	    if ( obs[0] > obs[1] ) {
	        // caught a bogus intermediate value coming out of the gap
	      obs[1] = obs[0];
	    }
	} else if ( obs[0] > 480 && obs[1] < 480 && obs[2] < 480 ) {
            // crossed the gap going down
	    if ( obs[1] > obs[2] ) {
	        // caught a bogus intermediate value coming out of the gap
	        obs[1] = obs[2];
	    }
	} else if ( obs[0] < 480 && obs[1] > 480 && obs[2] > 480 ) {
            // crossed the gap going up
	    if ( obs[1] < obs[2] ) {
	        // caught a bogus intermediate value coming out of the gap
	        obs[1] = obs[2];
	    }
	}
        result = obs[1] - obs[2];
        if ( abs(result) > 400 ) {
            // ignore
            result = 0;
        }
        obs[2] = obs[1];
        obs[1] = obs[0];
    }

    // cout << " result = " << result << endl;
    if ( result < -500 ) { result += 1024; }
    if ( result > 500 ) { result -= 1024; }

    return result;
}


static double instr_pot_filter( double ave, double val ) {
    if ( fabs(ave - val) < 400 || fabs(val) < fabs(ave) ) {
        return 0.5 * ave + 0.5 * val;
    } else {
        return ave;
    }
}


bool FGATC610x::do_analog_in() {
    // Read raw data in byte form
    ATC610xReadAnalogInputs( analog_in_fd, analog_in_bytes );

    // Convert to integer values
    for ( int channel = 0; channel < ATC_ANAL_IN_VALUES; ++channel ) {
	unsigned char hi = analog_in_bytes[2 * channel] & 0x03;
	unsigned char lo = analog_in_bytes[2 * channel + 1];
	analog_in_data[channel] = hi * 256 + lo;

	// printf("%02x %02x ", hi, lo );
	// printf("%04d ", value );
    }

    float tmp;

    if ( !ignore_flight_controls->getBoolValue() ) {
        // aileron
        tmp = scale( ailerons_center->getIntValue(),
                     ailerons_min->getIntValue(),
                     ailerons_max->getIntValue(), analog_in_data[0] );
        fgSetFloat( "/controls/flight/aileron", tmp );
        // cout << "aileron = " << analog_in_data[0] << " = " << tmp;

        // elevator
        tmp = -scale( elevator_center->getIntValue(),
                      elevator_min->getIntValue(),
                      elevator_max->getIntValue(), analog_in_data[5] );
        fgSetFloat( "/controls/flight/elevator", tmp );
        // cout << "trim = " << analog_in_data[4] << " = " << tmp;

        // elevator trim
        tmp = scale( trim_center->getIntValue(), trim_min->getIntValue(),
                     trim_max->getIntValue(), analog_in_data[4] );
        fgSetFloat( "/controls/flight/elevator-trim", tmp );
        // cout << " elev = " << analog_in_data[5] << " = " << tmp << endl;

        // mixture
        tmp = scale( mixture_min->getIntValue(), mixture_max->getIntValue(),
                     analog_in_data[6] );
        fgSetFloat( "/controls/engines/engine[0]/mixture", tmp );
        fgSetFloat( "/controls/engines/engine[1]/mixture", tmp );

        // throttle
        tmp = scale( throttle_min->getIntValue(), throttle_max->getIntValue(),
                     analog_in_data[8] );
        fgSetFloat( "/controls/engines/engine[0]/throttle", tmp );
        fgSetFloat( "/controls/engines/engine[1]/throttle", tmp );
        // cout << "throttle = " << tmp << endl;

        if ( use_rudder ) {
            // rudder
            tmp = scale( rudder_center->getIntValue(),
                         rudder_min->getIntValue(),
                         rudder_max->getIntValue(), analog_in_data[10] );
            fgSetFloat( "/controls/flight/rudder", -tmp );

            // toe brakes
            tmp = scale( brake_left_min->getIntValue(),
                         brake_left_max->getIntValue(),
                         analog_in_data[20] );
            fgSetFloat( "/controls/gear/brake-left", tmp );
            tmp = scale( brake_right_min->getIntValue(),
                         brake_right_max->getIntValue(),
                         analog_in_data[21] );
            fgSetFloat( "/controls/gear/brake-right", tmp );
        }
    }

    // nav1 volume
    tmp = (float)analog_in_data[25] / 1024.0f;
    fgSetFloat( "/radios/nav[0]/volume", tmp );

    // nav2 volume
    tmp = (float)analog_in_data[24] / 1024.0f;
    fgSetFloat( "/radios/nav[1]/volume", tmp );

    // adf volume
    tmp = (float)analog_in_data[26] / 1024.0f;
    fgSetFloat( "/radios/kr-87/inputs/volume", tmp );

    // instrument panel pots
    static bool first = true;
    static int obs1[3], obs2[3], obs3[3], obs4[3], obs5[3], obs6[3];
    static double diff1_ave = 0.0;
    static double diff2_ave = 0.0;
    static double diff3_ave = 0.0;
    static double diff4_ave = 0.0;
    static double diff5_ave = 0.0;
    static double diff6_ave = 0.0;

    if ( first ) {
        first = false;
        obs1[0] = obs1[1] = obs1[2] = analog_in_data[11];
        obs2[0] = obs2[1] = obs2[2] = analog_in_data[28];
        obs3[0] = obs3[1] = obs3[2] = analog_in_data[29];
        obs4[0] = obs4[1] = obs4[2] = analog_in_data[30];
        obs5[0] = obs5[1] = obs5[2] = analog_in_data[31];
        obs6[0] = obs6[1] = obs6[2] = analog_in_data[14];
    }

    int diff1 = tony_magic( analog_in_data[11], obs1 );
    int diff2 = tony_magic( analog_in_data[28], obs2 );
    int diff3 = tony_magic( analog_in_data[29], obs3 );
    int diff4 = tony_magic( analog_in_data[30], obs4 );
    int diff5 = tony_magic( analog_in_data[31], obs5 );
    int diff6 = tony_magic( analog_in_data[14], obs6 );

    diff1_ave = instr_pot_filter( diff1_ave, diff1 );
    diff2_ave = instr_pot_filter( diff2_ave, diff2 );
    diff3_ave = instr_pot_filter( diff3_ave, diff3 );
    diff4_ave = instr_pot_filter( diff4_ave, diff4 );
    diff5_ave = instr_pot_filter( diff5_ave, diff5 );
    diff6_ave = instr_pot_filter( diff6_ave, diff6 );

    tmp = alt_press->getDoubleValue() + (diff1_ave * (0.25/888.0) );
    if ( tmp < 27.9 ) { tmp = 27.9; }
    if ( tmp > 31.4 ) { tmp = 31.4; }
    fgSetFloat( "/instrumentation/altimeter/setting-inhg", tmp );

    tmp = ati_bird->getDoubleValue() + (diff2_ave * (20.0/888.0) );
    if ( tmp < -10.0 ) { tmp = -10.0; }
    if ( tmp > 10.0 ) { tmp = 10.0; }
    fgSetFloat( "/instrumentation/attitude-indicator/horizon-offset-deg", tmp );

    tmp = nav1_obs->getDoubleValue() + (diff3_ave * (72.0/888.0) );
    while ( tmp >= 360.0 ) { tmp -= 360.0; }
    while ( tmp < 0.0 ) { tmp += 360.0; }
    // cout << " obs = " << tmp << endl;
    fgSetFloat( "/radios/nav[0]/radials/selected-deg", tmp );

    tmp = nav2_obs->getDoubleValue() + (diff4_ave * (72.0/888.0) );
    while ( tmp >= 360.0 ) { tmp -= 360.0; }
    while ( tmp < 0.0 ) { tmp += 360.0; }
    // cout << " obs = " << tmp << endl;
    fgSetFloat( "/radios/nav[1]/radials/selected-deg", tmp );

    tmp = adf_hdg->getDoubleValue() + (diff5_ave * (72.0/888.0) );
    while ( tmp >= 360.0 ) { tmp -= 360.0; }
    while ( tmp < 0.0 ) { tmp += 360.0; }
    // cout << " obs = " << tmp << endl;
    fgSetFloat( "/radios/kr-87/inputs/rotation-deg", tmp );

    tmp = hdg_bug->getDoubleValue() + (diff6_ave * (72.0/888.0) );
    while ( tmp >= 360.0 ) { tmp -= 360.0; }
    while ( tmp < 0.0 ) { tmp += 360.0; }
    // cout << " obs = " << tmp << endl;
    fgSetFloat( "/autopilot/settings/heading-bug-deg", tmp );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Write the lights
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_lights() {

    // Marker beacons
    ATC610xSetLamp( lamps_fd, 4, inner->getBoolValue() );
    ATC610xSetLamp( lamps_fd, 5, middle->getBoolValue() );
    ATC610xSetLamp( lamps_fd, 3, outer->getBoolValue() );

    // ADF annunciators
    ATC610xSetLamp( lamps_fd, 11, adf_ant_ann->getBoolValue() ); // ANT
    ATC610xSetLamp( lamps_fd, 12, adf_adf_ann->getBoolValue() ); // ADF
    ATC610xSetLamp( lamps_fd, 13, adf_bfo_ann->getBoolValue() ); // BFO
    ATC610xSetLamp( lamps_fd, 14, adf_frq_ann->getBoolValue() ); // FRQ
    ATC610xSetLamp( lamps_fd, 15, adf_flt_ann->getBoolValue() ); // FLT
    ATC610xSetLamp( lamps_fd, 16, adf_et_ann->getBoolValue() ); // ET

    // Transponder annunciators
    ATC610xSetLamp( lamps_fd, 17, xpdr_fl_ann->getBoolValue() ); // FL
    ATC610xSetLamp( lamps_fd, 18, xpdr_alt_ann->getBoolValue() ); // ALT
    ATC610xSetLamp( lamps_fd, 19, xpdr_gnd_ann->getBoolValue() ); // GND
    ATC610xSetLamp( lamps_fd, 20, xpdr_on_ann->getBoolValue() ); // ON
    ATC610xSetLamp( lamps_fd, 21, xpdr_sby_ann->getBoolValue() ); // SBY
    ATC610xSetLamp( lamps_fd, 22, xpdr_reply_ann->getBoolValue() ); // R

    return true;
}


/////////////////////////////////////////////////////////////////////
// Read radio switches 
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_radio_switches() {
    double freq, coarse_freq, fine_freq, value;
    int diff;

    ATC610xReadRadios( radios_fd, radio_switch_data );

    // DME Switch
    dme_switch = (radio_switch_data[7] >> 4) & 0x03;
    if ( dme_switch == 0 ) {
	// off
	fgSetInt( "/instrumentation/dme/switch-position", 0 );
    } else if ( dme_switch == 2 ) {
	// nav1
	fgSetInt( "/instrumentation/dme/switch-position", 1 );
        fgSetString( "/instrumentation/dme/frequencies/source",
                     "/radios/nav[0]/frequencies/selected-mhz" );
	freq = fgGetFloat( "/radios/nav[0]/frequencies/selected-mhz", true );
        fgSetFloat( "/instrumentation/dme/frequencies/selected-mhz", freq );
    } else if ( dme_switch == 1 ) {
	// nav2
	fgSetInt( "/instrumentation/dme/switch-position", 3 );
        fgSetString( "/instrumentation/dme/frequencies/source",
                     "/radios/nav[1]/frequencies/selected-mhz" );
	freq = fgGetFloat( "/radios/nav[1]/frequencies/selected-mhz", true );
        fgSetFloat( "/instrumentation/dme/frequencies/selected-mhz", freq );
    }

    // NavCom1 Power
    fgSetBool( "/radios/comm[0]/inputs/power-btn",
               radio_switch_data[7] & 0x01 );

    if ( navcom1_has_power() && comm1_serviceable->getBoolValue() ) {
        // Com1 Swap
        int com1_swap = !((radio_switch_data[7] >> 1) & 0x01);
        static int last_com1_swap;
        if ( com1_swap && (last_com1_swap != com1_swap) ) {
            float tmp = com1_freq->getFloatValue();
            fgSetFloat( "/radios/comm[0]/frequencies/selected-mhz",
                        com1_stby_freq->getFloatValue() );
            fgSetFloat( "/radios/comm[0]/frequencies/standby-mhz", tmp );
        }
        last_com1_swap = com1_swap;
    }

    // NavCom2 Power
    fgSetBool( "/radios/comm[1]/inputs/power-btn",
               radio_switch_data[15] & 0x01 );

    if ( navcom2_has_power() && comm2_serviceable->getBoolValue() ) {
        // Com2 Swap
        int com2_swap = !((radio_switch_data[15] >> 1) & 0x01);
        static int last_com2_swap;
        if ( com2_swap && (last_com2_swap != com2_swap) ) {
            float tmp = com2_freq->getFloatValue();
            fgSetFloat( "/radios/comm[1]/frequencies/selected-mhz",
                        com2_stby_freq->getFloatValue() );
            fgSetFloat( "/radios/comm[1]/frequencies/standby-mhz", tmp );
        }
        last_com2_swap = com2_swap;
    }

    if ( navcom1_has_power() && nav1_serviceable->getBoolValue() ) {
        // Nav1 Swap
        int nav1_swap = radio_switch_data[11] & 0x01;
        static int last_nav1_swap;
        if ( nav1_swap && (last_nav1_swap != nav1_swap) ) {
            float tmp = nav1_freq->getFloatValue();
            fgSetFloat( "/radios/nav[0]/frequencies/selected-mhz",
                        nav1_stby_freq->getFloatValue() );
            fgSetFloat( "/radios/nav[0]/frequencies/standby-mhz", tmp );
        }
        last_nav1_swap = nav1_swap;
    }

    if ( navcom2_has_power() && nav2_serviceable->getBoolValue() ) {
        // Nav2 Swap
        int nav2_swap = !(radio_switch_data[19] & 0x01);
        static int last_nav2_swap;
        if ( nav2_swap && (last_nav2_swap != nav2_swap) ) {
            float tmp = nav2_freq->getFloatValue();
            fgSetFloat( "/radios/nav[1]/frequencies/selected-mhz",
                        nav2_stby_freq->getFloatValue() );
            fgSetFloat( "/radios/nav[1]/frequencies/standby-mhz", tmp );
        }
        last_nav2_swap = nav2_swap;
    }

    if ( navcom1_has_power() && comm1_serviceable->getBoolValue() ) {
        // Com1 Tuner
        int com1_tuner_fine = ((radio_switch_data[5] >> 4) & 0x0f) - 1;
        int com1_tuner_coarse = (radio_switch_data[5] & 0x0f) - 1;
        static int last_com1_tuner_fine = com1_tuner_fine;
        static int last_com1_tuner_coarse = com1_tuner_coarse;

        freq = com1_stby_freq->getFloatValue();
        coarse_freq = (int)freq;
        fine_freq = (int)((freq - coarse_freq) * 40 + 0.5);

        if ( com1_tuner_fine != last_com1_tuner_fine ) {
            diff = com1_tuner_fine - last_com1_tuner_fine;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( com1_tuner_fine < last_com1_tuner_fine ) {
                    // going up
                    diff = 12 - last_com1_tuner_fine + com1_tuner_fine;
                } else {
                    // going down
                    diff = com1_tuner_fine - 12 - last_com1_tuner_fine;
                }
            }
            fine_freq += diff;
        }
        while ( fine_freq >= 40.0 ) { fine_freq -= 40.0; }
        while ( fine_freq < 0.0 )  { fine_freq += 40.0; }

        if ( com1_tuner_coarse != last_com1_tuner_coarse ) {
            diff = com1_tuner_coarse - last_com1_tuner_coarse;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( com1_tuner_coarse < last_com1_tuner_coarse ) {
                    // going up
                    diff = 12 - last_com1_tuner_coarse + com1_tuner_coarse;
                } else {
                    // going down
                    diff = com1_tuner_coarse - 12 - last_com1_tuner_coarse;
                }
            }
            coarse_freq += diff;
        }
        if ( coarse_freq < 118.0 ) { coarse_freq += 19.0; }
        if ( coarse_freq > 136.0 ) { coarse_freq -= 19.0; }

        last_com1_tuner_fine = com1_tuner_fine;
        last_com1_tuner_coarse = com1_tuner_coarse;

        fgSetFloat( "/radios/comm[0]/frequencies/standby-mhz", 
                    coarse_freq + fine_freq / 40.0 );
    }

    if ( navcom2_has_power() && comm2_serviceable->getBoolValue() ) {
        // Com2 Tuner
        int com2_tuner_fine = ((radio_switch_data[13] >> 4) & 0x0f) - 1;
        int com2_tuner_coarse = (radio_switch_data[13] & 0x0f) - 1;
        static int last_com2_tuner_fine = com2_tuner_fine;
        static int last_com2_tuner_coarse = com2_tuner_coarse;

        freq = com2_stby_freq->getFloatValue();
        coarse_freq = (int)freq;
        fine_freq = (int)((freq - coarse_freq) * 40 + 0.5);

        if ( com2_tuner_fine != last_com2_tuner_fine ) {
            diff = com2_tuner_fine - last_com2_tuner_fine;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( com2_tuner_fine < last_com2_tuner_fine ) {
                    // going up
                    diff = 12 - last_com2_tuner_fine + com2_tuner_fine;
                } else {
                    // going down
                    diff = com2_tuner_fine - 12 - last_com2_tuner_fine;
                }
            }
            fine_freq += diff;
        }
        while ( fine_freq >= 40.0 ) { fine_freq -= 40.0; }
        while ( fine_freq < 0.0 )  { fine_freq += 40.0; }

        if ( com2_tuner_coarse != last_com2_tuner_coarse ) {
            diff = com2_tuner_coarse - last_com2_tuner_coarse;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( com2_tuner_coarse < last_com2_tuner_coarse ) {
                    // going up
                    diff = 12 - last_com2_tuner_coarse + com2_tuner_coarse;
                } else {
                    // going down
                    diff = com2_tuner_coarse - 12 - last_com2_tuner_coarse;
                }
            }
            coarse_freq += diff;
        }
        if ( coarse_freq < 118.0 ) { coarse_freq += 19.0; }
        if ( coarse_freq > 136.0 ) { coarse_freq -= 19.0; }

        last_com2_tuner_fine = com2_tuner_fine;
        last_com2_tuner_coarse = com2_tuner_coarse;

        fgSetFloat( "/radios/comm[1]/frequencies/standby-mhz",
                    coarse_freq + fine_freq / 40.0 );
    }

    if ( navcom1_has_power() && nav1_serviceable->getBoolValue() ) {
        // Nav1 Tuner
        int nav1_tuner_fine = ((radio_switch_data[9] >> 4) & 0x0f) - 1;
        int nav1_tuner_coarse = (radio_switch_data[9] & 0x0f) - 1;
        static int last_nav1_tuner_fine = nav1_tuner_fine;
        static int last_nav1_tuner_coarse = nav1_tuner_coarse;

        freq = nav1_stby_freq->getFloatValue();
        coarse_freq = (int)freq;
        fine_freq = (int)((freq - coarse_freq) * 20 + 0.5);

        if ( nav1_tuner_fine != last_nav1_tuner_fine ) {
            diff = nav1_tuner_fine - last_nav1_tuner_fine;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( nav1_tuner_fine < last_nav1_tuner_fine ) {
                    // going up
                    diff = 12 - last_nav1_tuner_fine + nav1_tuner_fine;
                } else {
                    // going down
                    diff = nav1_tuner_fine - 12 - last_nav1_tuner_fine;
                }
            }
            fine_freq += diff;
        }
        while ( fine_freq >= 20.0 ) { fine_freq -= 20.0; }
        while ( fine_freq < 0.0 )  { fine_freq += 20.0; }

        if ( nav1_tuner_coarse != last_nav1_tuner_coarse ) {
            diff = nav1_tuner_coarse - last_nav1_tuner_coarse;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( nav1_tuner_coarse < last_nav1_tuner_coarse ) {
                    // going up
                    diff = 12 - last_nav1_tuner_coarse + nav1_tuner_coarse;
                } else {
                    // going down
                    diff = nav1_tuner_coarse - 12 - last_nav1_tuner_coarse;
                }
            }
            coarse_freq += diff;
        }
        if ( coarse_freq < 108.0 ) { coarse_freq += 10.0; }
        if ( coarse_freq > 117.0 ) { coarse_freq -= 10.0; }

        last_nav1_tuner_fine = nav1_tuner_fine;
        last_nav1_tuner_coarse = nav1_tuner_coarse;

        fgSetFloat( "/radios/nav[0]/frequencies/standby-mhz",
                    coarse_freq + fine_freq / 20.0 );
    }

    if ( navcom2_has_power() && nav2_serviceable->getBoolValue() ) {
        // Nav2 Tuner
        int nav2_tuner_fine = ((radio_switch_data[17] >> 4) & 0x0f) - 1;
        int nav2_tuner_coarse = (radio_switch_data[17] & 0x0f) - 1;
        static int last_nav2_tuner_fine = nav2_tuner_fine;
        static int last_nav2_tuner_coarse = nav2_tuner_coarse;

        freq = nav2_stby_freq->getFloatValue();
        coarse_freq = (int)freq;
        fine_freq = (int)((freq - coarse_freq) * 20 + 0.5);

        if ( nav2_tuner_fine != last_nav2_tuner_fine ) {
            diff = nav2_tuner_fine - last_nav2_tuner_fine;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( nav2_tuner_fine < last_nav2_tuner_fine ) {
                    // going up
                    diff = 12 - last_nav2_tuner_fine + nav2_tuner_fine;
                } else {
                    // going down
                    diff = nav2_tuner_fine - 12 - last_nav2_tuner_fine;
                }
            }
            fine_freq += diff;
        }
        while ( fine_freq >= 20.0 ) { fine_freq -= 20.0; }
        while ( fine_freq < 0.0 )  { fine_freq += 20.0; }

        if ( nav2_tuner_coarse != last_nav2_tuner_coarse ) {
            diff = nav2_tuner_coarse - last_nav2_tuner_coarse;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( nav2_tuner_coarse < last_nav2_tuner_coarse ) {
                    // going up
                    diff = 12 - last_nav2_tuner_coarse + nav2_tuner_coarse;
                } else {
                    // going down
                    diff = nav2_tuner_coarse - 12 - last_nav2_tuner_coarse;
                }
            }
            coarse_freq += diff;
        }
        if ( coarse_freq < 108.0 ) { coarse_freq += 10.0; }
        if ( coarse_freq > 117.0 ) { coarse_freq -= 10.0; }

        last_nav2_tuner_fine = nav2_tuner_fine;
        last_nav2_tuner_coarse = nav2_tuner_coarse;

        fgSetFloat( "/radios/nav[1]/frequencies/standby-mhz", 
                    coarse_freq + fine_freq / 20.0);
    }

    // ADF Tuner
    
    int adf_tuner_fine = ((radio_switch_data[21] >> 4) & 0x0f) - 1;
    int adf_tuner_coarse = (radio_switch_data[21] & 0x0f) - 1;
    static int last_adf_tuner_fine = adf_tuner_fine;
    static int last_adf_tuner_coarse = adf_tuner_coarse;

    if ( adf_has_power() && adf_serviceable->getBoolValue() ) {
        // cout << "adf_stby_mode = " << adf_stby_mode->getIntValue() << endl;
        if ( adf_count_mode->getIntValue() == 2 ) {
            // tune count down timer
            value = adf_elapsed_timer->getDoubleValue();
        } else {
            // tune frequency
            if ( adf_stby_mode->getIntValue() == 1 ) {
                value = adf_freq->getFloatValue();
            } else {
                value = adf_stby_freq->getFloatValue();
            }
        }

        if ( adf_tuner_fine != last_adf_tuner_fine ) {
            diff = adf_tuner_fine - last_adf_tuner_fine;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( adf_tuner_fine < last_adf_tuner_fine ) {
                    // going up
                    diff = 12 - last_adf_tuner_fine + adf_tuner_fine;
                } else {
                    // going down
                    diff = adf_tuner_fine - 12 - last_adf_tuner_fine;
                }
            }
            value += diff;
        }

        if ( adf_tuner_coarse != last_adf_tuner_coarse ) {
            diff = adf_tuner_coarse - last_adf_tuner_coarse;
            if ( abs(diff) > 4 ) {
                // roll over
                if ( adf_tuner_coarse < last_adf_tuner_coarse ) {
                    // going up
                    diff = 12 - last_adf_tuner_coarse + adf_tuner_coarse;
                } else {
                    // going down
                    diff = adf_tuner_coarse - 12 - last_adf_tuner_coarse;
                }
            }
            if ( adf_count_mode->getIntValue() == 2 ) {
                value += 60 * diff;
            } else {
                value += 25 * diff;
            }
        }
        if ( adf_count_mode->getIntValue() == 2 ) {
            if ( value < 0 ) { value += 3600; }
            if ( value > 3599 ) { value -= 3600; }
        } else {
            if ( value < 200 ) { value += 1600; }
            if ( value > 1799 ) { value -= 1600; }
        }
 
        if ( adf_count_mode->getIntValue() == 2 ) {
            fgSetFloat( "/radios/kr-87/outputs/elapsed-timer", value );
        } else {
            if ( adf_stby_mode->getIntValue() == 1 ) {
                fgSetFloat( "/radios/kr-87/outputs/selected-khz", value );
            } else {
                fgSetFloat( "/radios/kr-87/outputs/standby-khz", value );
            }
        }
    }
    last_adf_tuner_fine = adf_tuner_fine;
    last_adf_tuner_coarse = adf_tuner_coarse;


    // ADF buttons 
    fgSetInt( "/radios/kr-87/inputs/adf-btn",
              !(radio_switch_data[23] & 0x01) );
    fgSetInt( "/radios/kr-87/inputs/bfo-btn",
              !(radio_switch_data[23] >> 1 & 0x01) );
#ifdef CURT_HARDWARE
    fgSetInt( "/radios/kr-87/inputs/frq-btn",
              !(radio_switch_data[23] >> 2 & 0x01) );
#else
    fgSetInt( "/radios/kr-87/inputs/frq-btn",
              (radio_switch_data[23] >> 2 & 0x01) );
#endif
    fgSetInt( "/radios/kr-87/inputs/flt-et-btn",
              !(radio_switch_data[23] >> 3 & 0x01) );
    fgSetInt( "/radios/kr-87/inputs/set-rst-btn",
              !(radio_switch_data[23] >> 4 & 0x01) );
    fgSetInt( "/radios/kr-87/inputs/power-btn",
              radio_switch_data[23] >> 5 & 0x01 );
    /* cout << "adf = " << !(radio_switch_data[23] & 0x01)
         << " bfo = " << !(radio_switch_data[23] >> 1 & 0x01)
         << " frq = " << !(radio_switch_data[23] >> 2 & 0x01)
         << " flt/et = " << !(radio_switch_data[23] >> 3 & 0x01)
         << " set/rst = " << !(radio_switch_data[23] >> 4 & 0x01)
         << endl; */

    // Transponder Tuner
    int i;
    int digit_tuner[4];
    digit_tuner[0] = radio_switch_data[25] & 0x0f;
    digit_tuner[1] = ( radio_switch_data[25] >> 4 ) & 0x0f;
    digit_tuner[2] = radio_switch_data[29] & 0x0f;
    digit_tuner[3] = ( radio_switch_data[29] >> 4 ) & 0x0f;

    static int last_digit_tuner[4];
    static bool first_time = true;
    if ( first_time ) {
        first_time = false;
        for ( i = 0; i < 4; ++i ) {
            last_digit_tuner[i] = digit_tuner[i];
        }
    }

    if ( xpdr_has_power() && xpdr_serviceable->getBoolValue() ) {
        int id_code = xpdr_id_code->getIntValue();
        int digit[4];
        int place = 1000;
        for ( i = 0; i < 4; ++i ) {
            digit[i] = id_code / place;
            id_code -= digit[i] * place;
            place /= 10;
        }

        for ( i = 0; i < 4; ++i ) {
            if ( digit_tuner[i] != last_digit_tuner[i] ) {
                diff = digit_tuner[i] - last_digit_tuner[i];
                if ( abs(diff) > 4 ) {
                    // roll over
                    if ( digit_tuner[i] < last_digit_tuner[i] ) {
                        // going up
                        diff = 16 - last_digit_tuner[i] + digit_tuner[i];
                    } else {
                        // going down
                        diff = digit_tuner[i] - 16 - last_digit_tuner[i];
                    }
                }
                digit[i] += diff;
            }
            while ( digit[i] >= 8 ) { digit[i] -= 8; }
            while ( digit[i] < 0 )  { digit[i] += 8; }
        }

        fgSetInt( "/radios/kt-70/inputs/digit1", digit[0] );
        fgSetInt( "/radios/kt-70/inputs/digit2", digit[1] );
        fgSetInt( "/radios/kt-70/inputs/digit3", digit[2] );
        fgSetInt( "/radios/kt-70/inputs/digit4", digit[3] );
    }
    for ( i = 0; i < 4; ++i ) {
        last_digit_tuner[i] = digit_tuner[i];
    }

    int tmp = 0;
    for ( i = 0; i < 5; ++i ) {
        if ( radio_switch_data[27] >> i & 0x01 ) {
            tmp = i + 1;
        }
    }
    fgSetInt( "/radios/kt-70/inputs/func-knob", tmp );
    fgSetInt( "/radios/kt-70/inputs/ident-btn",
              !(radio_switch_data[27] >> 5 & 0x01) );

    // Audio panel switches
    fgSetInt( "/radios/nav[0]/audio-btn",
              (radio_switch_data[3] & 0x01) );
    fgSetInt( "/radios/nav[1]/audio-btn",
              (radio_switch_data[3] >> 2 & 0x01) );
    fgSetInt( "/radios/kr-87/inputs/audio-btn",
              (radio_switch_data[3] >> 4 & 0x01) );
    fgSetInt( "/radios/marker-beacon/audio-btn",
              (radio_switch_data[3] >> 6 & 0x01) );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Update the radio display 
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_radio_display() {

    char digits[10];
    int i;

    if ( dme_has_power() && dme_serviceable->getBoolValue() ) {
        if ( dme_in_range->getBoolValue() ) {
            // DME minutes
            float minutes = dme_min->getFloatValue();
            if ( minutes > 999 ) {
                minutes = 999.0;
            }
            snprintf(digits, 7, "%03.0f", minutes);
            for ( i = 0; i < 6; ++i ) {
                digits[i] -= '0';
            }
            radio_display_data[0] = digits[1] << 4 | digits[2];
            radio_display_data[1] = 0xf0 | digits[0];
	
            // DME knots
            float knots = dme_kt->getFloatValue();
            if ( knots > 999 ) {
                knots = 999.0;
            }
            snprintf(digits, 7, "%03.0f", knots);
            for ( i = 0; i < 6; ++i ) {
                digits[i] -= '0';
            }
            radio_display_data[2] = digits[1] << 4 | digits[2];
            radio_display_data[3] = 0xf0 | digits[0];

            // DME distance (nm)
            float nm = dme_nm->getFloatValue();
            if ( nm > 99 ) {
                nm = 99.0;
            }
            snprintf(digits, 7, "%04.1f", nm);
            for ( i = 0; i < 6; ++i ) {
                digits[i] -= '0';
            }
            radio_display_data[4] = digits[1] << 4 | digits[3];
            radio_display_data[5] = 0x00 | digits[0];
            // the 0x00 in the upper nibble of the 6th byte of each
            // display turns on the decimal point
        } else {
            // out of range
            radio_display_data[0] = 0xbb;
            radio_display_data[1] = 0xfb;
            radio_display_data[2] = 0xbb;
            radio_display_data[3] = 0xfb;
            radio_display_data[4] = 0xbb;
            radio_display_data[5] = 0x0b;
        }
    } else {
	// blank dem display
	for ( i = 0; i < 6; ++i ) {
	    radio_display_data[i] = 0xff;
	}
    }

    if ( navcom1_has_power() && comm1_serviceable->getBoolValue() ) {
        // Com1 standby frequency
        float com1_stby = com1_stby_freq->getFloatValue();
        if ( fabs(com1_stby) > 999.99 ) {
            com1_stby = 0.0;
        }
        snprintf(digits, 7, "%06.3f", com1_stby);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[6] = digits[4] << 4 | digits[5];
        radio_display_data[7] = digits[1] << 4 | digits[2];
        radio_display_data[8] = 0xf0 | digits[0];

        // Com1 in use frequency
        float com1 = com1_freq->getFloatValue();
        if ( fabs(com1) > 999.99 ) {
            com1 = 0.0;
        }
        snprintf(digits, 7, "%06.3f", com1);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[9] = digits[4] << 4 | digits[5];
        radio_display_data[10] = digits[1] << 4 | digits[2];
        radio_display_data[11] = 0x00 | digits[0];
        // the 0x00 in the upper nibble of the 6th byte of each display
        // turns on the decimal point
    } else {
        radio_display_data[6] = 0xff;
        radio_display_data[7] = 0xff;
        radio_display_data[8] = 0xff;
        radio_display_data[9] = 0xff;
        radio_display_data[10] = 0xff;
        radio_display_data[11] = 0xff;
    }

    if ( navcom2_has_power() && comm2_serviceable->getBoolValue() ) {
        // Com2 standby frequency
        float com2_stby = com2_stby_freq->getFloatValue();
        if ( fabs(com2_stby) > 999.99 ) {
            com2_stby = 0.0;
        }
        snprintf(digits, 7, "%06.3f", com2_stby);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[18] = digits[4] << 4 | digits[5];
        radio_display_data[19] = digits[1] << 4 | digits[2];
        radio_display_data[20] = 0xf0 | digits[0];

        // Com2 in use frequency
        float com2 = com2_freq->getFloatValue();
        if ( fabs(com2) > 999.99 ) {
	com2 = 0.0;
        }
        snprintf(digits, 7, "%06.3f", com2);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[21] = digits[4] << 4 | digits[5];
        radio_display_data[22] = digits[1] << 4 | digits[2];
        radio_display_data[23] = 0x00 | digits[0];
        // the 0x00 in the upper nibble of the 6th byte of each display
        // turns on the decimal point
    } else {
        radio_display_data[18] = 0xff;
        radio_display_data[19] = 0xff;
        radio_display_data[20] = 0xff;
        radio_display_data[21] = 0xff;
        radio_display_data[22] = 0xff;
        radio_display_data[23] = 0xff;
    }

    if ( navcom1_has_power() && nav1_serviceable->getBoolValue() ) {
        // Nav1 standby frequency
        float nav1_stby = nav1_stby_freq->getFloatValue();
        if ( fabs(nav1_stby) > 999.99 ) {
	nav1_stby = 0.0;
        }
        snprintf(digits, 7, "%06.2f", nav1_stby);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[12] = digits[4] << 4 | digits[5];
        radio_display_data[13] = digits[1] << 4 | digits[2];
        radio_display_data[14] = 0xf0 | digits[0];

        // Nav1 in use frequency
        float nav1 = nav1_freq->getFloatValue();
        if ( fabs(nav1) > 999.99 ) {
            nav1 = 0.0;
        }
        snprintf(digits, 7, "%06.2f", nav1);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[15] = digits[4] << 4 | digits[5];
        radio_display_data[16] = digits[1] << 4 | digits[2];
        radio_display_data[17] = 0x00 | digits[0];
        // the 0x00 in the upper nibble of the 6th byte of each display
        // turns on the decimal point
    } else {
        radio_display_data[12] = 0xff;
        radio_display_data[13] = 0xff;
        radio_display_data[14] = 0xff;
        radio_display_data[15] = 0xff;
        radio_display_data[16] = 0xff;
        radio_display_data[17] = 0xff;
    }

    if ( navcom2_has_power() && nav2_serviceable->getBoolValue() ) {
        // Nav2 standby frequency
        float nav2_stby = nav2_stby_freq->getFloatValue();
        if ( fabs(nav2_stby) > 999.99 ) {
            nav2_stby = 0.0;
        }
        snprintf(digits, 7, "%06.2f", nav2_stby);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[24] = digits[4] << 4 | digits[5];
        radio_display_data[25] = digits[1] << 4 | digits[2];
        radio_display_data[26] = 0xf0 | digits[0];

        // Nav2 in use frequency
        float nav2 = nav2_freq->getFloatValue();
        if ( fabs(nav2) > 999.99 ) {
            nav2 = 0.0;
        }
        snprintf(digits, 7, "%06.2f", nav2);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[27] = digits[4] << 4 | digits[5];
        radio_display_data[28] = digits[1] << 4 | digits[2];
        radio_display_data[29] = 0x00 | digits[0];
        // the 0x00 in the upper nibble of the 6th byte of each display
        // turns on the decimal point
    } else {
        radio_display_data[24] = 0xff;
        radio_display_data[25] = 0xff;
        radio_display_data[26] = 0xff;
        radio_display_data[27] = 0xff;
        radio_display_data[28] = 0xff;
        radio_display_data[29] = 0xff;
    }

    // ADF standby frequency / timer
    if ( adf_has_power() && adf_serviceable->getBoolValue() ) {
        if ( adf_stby_mode->getIntValue() == 0 ) {
            // frequency
            float adf_stby = adf_stby_freq->getFloatValue();
            if ( fabs(adf_stby) > 1799 ) {
                adf_stby = 1799;
            }
            snprintf(digits, 7, "%04.0f", adf_stby);
            for ( i = 0; i < 6; ++i ) {
                digits[i] -= '0';
            }
            radio_display_data[30] = digits[3] << 4 | 0x0f;
            radio_display_data[31] = digits[1] << 4 | digits[2];
            if ( digits[0] == 0 ) {
                radio_display_data[32] = 0xff;
            } else {
                radio_display_data[32] = 0xf0 | digits[0];
            }
        } else {
            // timer
            double time;
            int hours, min, sec;
            if ( adf_timer_mode->getIntValue() == 0 ) {
                time = adf_flight_timer->getDoubleValue();
            } else {
                time = adf_elapsed_timer->getDoubleValue();
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
            // cout << big << ":" << little << endl;
            snprintf(digits, 7, "%02d%02d", big, little);
            for ( i = 0; i < 6; ++i ) {
                digits[i] -= '0';
            }
            radio_display_data[30] = digits[2] << 4 | digits[3];
            radio_display_data[31] = digits[0] << 4 | digits[1];
            radio_display_data[32] = 0xff;
        }

        // ADF in use frequency
        float adf = adf_freq->getFloatValue();
        if ( fabs(adf) > 1799 ) {
            adf = 1799;
        }
        snprintf(digits, 7, "%04.0f", adf);
        for ( i = 0; i < 6; ++i ) {
            digits[i] -= '0';
        }
        radio_display_data[33] = digits[2] << 4 | digits[3];
        if ( digits[0] == 0 ) {
            radio_display_data[34] = 0xf0 | digits[1];
        } else {
            radio_display_data[34] = digits[0] << 4 | digits[1];
        }
        if ( adf_stby_mode->getIntValue() == 0 ) {
	  radio_display_data[35] = 0xff;
	} else {
	  radio_display_data[35] = 0x0f;
	}
    } else {
        radio_display_data[30] = 0xff;
        radio_display_data[31] = 0xff;
        radio_display_data[32] = 0xff;
        radio_display_data[33] = 0xff;
        radio_display_data[34] = 0xff;
        radio_display_data[35] = 0xff;
    }
    
    // Transponder code and flight level
    if ( xpdr_has_power() && xpdr_serviceable->getBoolValue() ) {
        if ( xpdr_func_knob->getIntValue() == 2 ) {
            // test mode
            radio_display_data[36] = 8 << 4 | 8;
            radio_display_data[37] = 8 << 4 | 8;
            radio_display_data[38] = 0xff;
            radio_display_data[39] = 8 << 4 | 0x0f;
            radio_display_data[40] = 8 << 4 | 8;
        } else {
            // other on modes
            int id_code = xpdr_id_code->getIntValue();
            int place = 1000;
            for ( i = 0; i < 4; ++i ) {
                digits[i] = id_code / place;
                id_code -= digits[i] * place;
                place /= 10;
            }
            radio_display_data[36] = digits[2] << 4 | digits[3];
            radio_display_data[37] = digits[0] << 4 | digits[1];
            radio_display_data[38] = 0xff;

            if ( xpdr_func_knob->getIntValue() == 3 ||
                 xpdr_func_knob->getIntValue() == 5 )
            {
                // do flight level display
                snprintf(digits, 7, "%03d", xpdr_flight_level->getIntValue() );
                for ( i = 0; i < 6; ++i ) {
                    digits[i] -= '0';
                }
                radio_display_data[39] = digits[2] << 4 | 0x0f;
                radio_display_data[40] = digits[0] << 4 | digits[1];
            } else {
                // blank flight level display
                radio_display_data[39] = 0xff;
                radio_display_data[40] = 0xff;
            }
        }
    } else {
        // off
        radio_display_data[36] = 0xff;
        radio_display_data[37] = 0xff;
        radio_display_data[38] = 0xff;
        radio_display_data[39] = 0xff;
        radio_display_data[40] = 0xff;
    }

    ATC610xSetRadios( radios_fd, radio_display_data );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Drive the stepper motors
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_steppers() {
    float diff = mag_compass->getFloatValue() - compass_position;
    while ( diff < -180.0 ) { diff += 360.0; }
    while ( diff >  180.0 ) { diff -= 360.0; }

    int steps = (int)(diff * 4);
    // cout << "steps = " << steps << endl;
    if ( steps > 4 ) { steps = 4; }
    if ( steps < -4 ) { steps = -4; }

    if ( abs(steps) > 0 ) {
	unsigned char cmd = 0x80;	// stepper command
	if ( steps > 0 ) {
	    cmd |= 0x20;		// go up
	} else {
	    cmd |= 0x00;		// go down
	}
	cmd |= abs(steps);

	// sync compass_position with hardware position
	compass_position += (float)steps / 4.0;

	ATC610xSetStepper( stepper_fd, ATC_COMPASS_CH, cmd );
    }

    return true;
}


/////////////////////////////////////////////////////////////////////
// Read the switch positions
/////////////////////////////////////////////////////////////////////

// decode the packed switch data
static void update_switch_matrix(
        int board,
	unsigned char switch_data[ATC_SWITCH_BYTES],
	int switch_matrix[2][ATC_NUM_COLS][ATC_SWITCH_BYTES] )
{
    for ( int row = 0; row < ATC_SWITCH_BYTES; ++row ) {
	unsigned char switches = switch_data[row];

	for( int column = 0; column < ATC_NUM_COLS; ++column ) {
	    switch_matrix[board][column][row] = switches & 1;
	    switches = switches >> 1;
	}			
    }
}                     

bool FGATC610x::do_switches() {
    ATC610xReadSwitches( switches_fd, switch_data );

    // unpack the switch data
    int switch_matrix[2][ATC_NUM_COLS][ATC_SWITCH_BYTES];
    update_switch_matrix( board, switch_data, switch_matrix );

    // master switches
    fgSetBool( "/controls/switches/master-bat", switch_matrix[board][5][1] );
    fgSetBool( "/controls/switches/master-alt", switch_matrix[board][4][1] );
    fgSetBool( "/controls/switches/master-avionics",
               switch_matrix[board][0][3] );

    if ( !ignore_flight_controls->getBoolValue() ) {
        // magnetos and starter switch
        int magnetos = 0;
        bool starter = false;
        if ( switch_matrix[board][3][1] == 1 ) {
            magnetos = 3;
            starter = true;
        } else if ( switch_matrix[board][2][1] == 1 ) {
            magnetos = 3;
            starter = false;
        } else if ( switch_matrix[board][1][1] == 1 ) {
            magnetos = 2;
            starter = false;
        } else if ( switch_matrix[board][0][1] == 1 ) {
            magnetos = 1;
            starter = false;
        } else {
            magnetos = 0;
            starter = false;
        }

        // do a bit of filtering on the magneto/starter switch and the
        // flap lever because these are not well debounced in hardware
        static int mag1, mag2, mag3;
        mag3 = mag2;
        mag2 = mag1;
        mag1 = magnetos;
        if ( mag1 == mag2 && mag2 == mag3 ) {
            fgSetInt( "/controls/engines/engine[0]/magnetos", magnetos );
            fgSetInt( "/controls/engines/engine[1]/magnetos", magnetos );
        }
        static bool start1, start2, start3;
        start3 = start2;
        start2 = start1;
        start1 = starter;
        if ( start1 == start2 && start2 == start3 ) {
            fgSetBool( "/controls/engines/engine[0]/starter", starter );
            fgSetBool( "/controls/engines/engine[1]/starter", starter );
        }
    }

    // other toggle switches
    fgSetBool( "/controls/engines/engine[0]/fuel-pump",
               switch_matrix[board][0][2] );
    fgSetBool( "/controls/engines/engine[1]/fuel-pump",
               switch_matrix[board][0][2] );
    fgSetBool( "/controls/switches/flashing-beacon",
               switch_matrix[board][1][2] );
    fgSetBool( "/controls/switches/landing-light", switch_matrix[board][2][2] );
    fgSetBool( "/controls/switches/taxi-lights", switch_matrix[board][3][2] );
    fgSetBool( "/controls/switches/nav-lights",
               switch_matrix[board][4][2] );
    fgSetBool( "/controls/switches/strobe-lights", switch_matrix[board][5][2] );
    fgSetBool( "/controls/switches/pitot-heat", switch_matrix[board][6][2] );

    // flaps
    if ( !ignore_flight_controls->getBoolValue() ) {
        float flaps = 0.0;
        if ( switch_matrix[board][6][3] ) {
            flaps = 1.0;
        } else if ( switch_matrix[board][5][3] ) {
            flaps = 2.0 / 3.0;
        } else if ( switch_matrix[board][4][3] ) {
            flaps = 1.0 / 3.0;
        } else if ( !switch_matrix[board][4][3] ) {
            flaps = 0.0;
        }

        // do a bit of filtering on the magneto/starter switch and the
        // flap lever because these are not well debounced in hardware
        static float flap1, flap2, flap3;
        flap3 = flap2;
        flap2 = flap1;
        flap1 = flaps;
        if ( flap1 == flap2 && flap2 == flap3 ) {
            fgSetFloat( "/controls/flight/flaps", flaps );
        }
    }

    // fuel selector (also filtered)
    int fuel = 0;
    if ( switch_matrix[board][2][3] ) {
        // both
        fuel = 3;
    } else if ( switch_matrix[board][1][3] ) {
        // left
        fuel = 1;
    } else if ( switch_matrix[board][3][3] ) {
        // right
        fuel = 2;
    } else {
        // fuel cutoff
        fuel = 0;
    }

    const int max_fuel = 60;
    static int fuel_list[max_fuel];
    int i;
    for ( i = max_fuel - 1; i >= 0; --i ) {
      fuel_list[i+1] = fuel_list[i];
    }
    fuel_list[0] = fuel;
    bool all_same = true;
    for ( i = 0; i < max_fuel - 1; ++i ) {
      if ( fuel_list[i] != fuel_list[i+1] ) {
	all_same = false;
      }
    }
    if ( all_same ) {
        fgSetBool( "/controls/fuel/tank[0]/fuel_selector", (fuel & 0x01) > 0 );
        fgSetBool( "/controls/fuel/tank[1]/fuel_selector", (fuel & 0x02) > 0 );
    }

    // circuit breakers
#ifdef ATC_SUPPORT_CIRCUIT_BREAKERS_NOT_THE_DEFAULT
    fgSetBool( "/controls/circuit-breakers/cabin-lights-pwr",
               switch_matrix[board][0][0] );
    fgSetBool( "/controls/circuit-breakers/instr-ignition-switch",
               switch_matrix[board][1][0] );
    fgSetBool( "/controls/circuit-breakers/flaps",
               switch_matrix[board][2][0] );
    fgSetBool( "/controls/circuit-breakers/avn-bus-1",
               switch_matrix[board][3][0] );
    fgSetBool( "/controls/circuit-breakers/avn-bus-2",
               switch_matrix[board][4][0] );
    fgSetBool( "/controls/circuit-breakers/turn-coordinator",
               switch_matrix[board][5][0] );
    fgSetBool( "/controls/circuit-breakers/instrument-lights",
               switch_matrix[board][6][0] );
    fgSetBool( "/controls/circuit-breakers/annunciators",
               switch_matrix[board][7][0] );
#else
    fgSetBool( "/controls/circuit-breakers/cabin-lights-pwr", true );
    fgSetBool( "/controls/circuit-breakers/instr-ignition-switch", true );
    fgSetBool( "/controls/circuit-breakers/flaps", true );
    fgSetBool( "/controls/circuit-breakers/avn-bus-1", true );
    fgSetBool( "/controls/circuit-breakers/avn-bus-2", true );
    fgSetBool( "/controls/circuit-breakers/turn-coordinator", true );
    fgSetBool( "/controls/circuit-breakers/instrument-lights", true );
    fgSetBool( "/controls/circuit-breakers/annunciators", true );
#endif

    if ( !ignore_flight_controls->getBoolValue() ) {
        fgSetDouble( "/controls/gear/brake-parking",
                     switch_matrix[board][7][3] );
    }

    fgSetDouble( "/radios/marker-beacon/power-btn",
                 switch_matrix[board][6][1] );

    return true;
}


bool FGATC610x::process() {
    // Lock the hardware, skip if it's not ready yet
    if ( ATC610xLock( lock_fd ) > 0 ) {

	do_analog_in();
	do_lights();
	do_radio_switches();
	do_radio_display();
	do_steppers();
	do_switches();
	
	ATC610xRelease( lock_fd );

	return true;
    } else {
	return false;
    }
}


bool FGATC610x::close() {

    return true;
}
