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
#include <simgear/misc/props.hxx>

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


// Open and initialize ATC 610x hardware
bool FGATC610x::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SG_LOG( SG_IO, SG_ALERT,
	    "Initializing ATC 610x hardware, please wait ..." );

    set_hz( 30 );		// default to processing requests @ 30Hz
    set_enabled( true );

    board = 0;			// 610x uses a single board number = 0

    snprintf( lock_file, 256, "/proc/atc610x/board%d/lock", board );
    snprintf( analog_in_file, 256, "/proc/atc610x/board%d/analog_in", board );
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
    while ( ! home ) {
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
    // Finished initing hardware
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "Done initializing ATC 610x hardware." );

    /////////////////////////////////////////////////////////////////////
    // Connect up to property values
    /////////////////////////////////////////////////////////////////////

    mag_compass = fgGetNode( "/steam/mag-compass-deg", true );

    dme_min = fgGetNode( "/radios/dme/ete-min", true );
    dme_kt = fgGetNode( "/radios/dme/speed-kt", true );
    dme_nm = fgGetNode( "/radios/dme/distance-nm", true );

    com1_freq = fgGetNode( "/radios/comm[0]/frequencies/selected-mhz", true );
    com1_stby_freq
	= fgGetNode( "/radios/comm[0]/frequencies/standby-mhz", true );
    com2_freq = fgGetNode( "/radios/comm[1]/frequencies/selected-mhz", true );
    com2_stby_freq
	= fgGetNode( "/radios/comm[1]/frequencies/standby-mhz", true );

    nav1_freq = fgGetNode( "/radios/nav[0]/frequencies/selected-mhz", true );
    nav1_stby_freq
	= fgGetNode( "/radios/nav[0]/frequencies/standby-mhz", true );

    nav2_freq = fgGetNode( "/radios/nav[1]/frequencies/selected-mhz", true );
    nav2_stby_freq
	= fgGetNode( "/radios/nav[1]/frequencies/standby-mhz", true );

    adf_freq = fgGetNode( "/radios/adf/frequencies/selected-khz", true );
    adf_stby_freq = fgGetNode( "/radios/adf/frequencies/standby-khz", true );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Read analog inputs
/////////////////////////////////////////////////////////////////////

#define ATC_AILERON_CENTER 535
#define ATC_ELEVATOR_TRIM_CENTER 512
#define ATC_ELEVATOR_CENTER 543

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

    float tmp, tmp1, tmp2;

    // aileron
    tmp = (float)(analog_in_data[0] - ATC_AILERON_CENTER) / 256.0f;
    fgSetFloat( "/controls/aileron", tmp );
    // cout << "aileron = " << analog_in_data[0] << " = " << tmp;

    // elevator
    tmp = (float)(analog_in_data[4] - ATC_ELEVATOR_TRIM_CENTER) / 512.0f;
    fgSetFloat( "/controls/elevator-trim", tmp );
    // cout << "trim = " << analog_in_data[4] << " = " << tmp;

    // trim
    tmp = (float)(ATC_ELEVATOR_CENTER - analog_in_data[5]) / 100.0f;
    fgSetFloat( "/controls/elevator", tmp );
    // cout << " elev = " << analog_in_data[5] << " = " << tmp << endl;

    // mixture
    tmp = (float)analog_in_data[7] / 680.0f;
    fgSetFloat( "/controls/mixture[0]", tmp );

    // throttle
    tmp = (float)analog_in_data[8] / 690.0f;
    fgSetFloat( "/controls/throttle[0]", tmp );

    // nav1 volume
    tmp = (float)analog_in_data[25] / 1024.0f;
    fgSetFloat( "/radios/nav[0]/volume", tmp );

    // nav2 volume
    tmp = (float)analog_in_data[24] / 1024.0f;
    fgSetFloat( "/radios/nav[1]/volume", tmp );

    // adf volume
    tmp = (float)analog_in_data[26] / 1024.0f;
    fgSetFloat( "/radios/adf/volume", tmp );

    // nav2 obs tuner
    tmp = (float)analog_in_data[29] * 360.0f / 1024.0f;
    fgSetFloat( "/radios/nav[1]/radials/selected-deg", tmp );

    // nav1 obs tuner
    tmp1 = (float)analog_in_data[30] * 360.0f / 1024.0f;
    tmp2 = (float)analog_in_data[31] * 360.0f / 1024.0f;
    fgSetFloat( "/radios/nav[0]/radials/selected-deg", tmp1 );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Read radio switches 
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_radio_switches() {
    float freq, inc;

    ATC610xReadRadios( radios_fd, radio_switch_data );

    // DME Switch
    dme_switch = (radio_switch_data[7] >> 4) & 0x03;
    if ( dme_switch == 0 ) {
	// off
	fgSetInt( "/radios/dme/switch-position", 0 );
    } else if ( dme_switch == 2 ) {
	// nav1
	fgSetInt( "/radios/dme/switch-position", 1 );
    } else if ( dme_switch == 1 ) {
	// nav2
	fgSetInt( "/radios/dme/switch-position", 3 );
    }

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

    // Com1 Tuner
    int com1_tuner_fine = (radio_switch_data[5] >> 4) & 0x0f;
    int com1_tuner_course = radio_switch_data[5] & 0x0f;
    // cout << "com1 = " << com1_tuner_fine << " " << com1_tuner_course << endl;
    static int last_com1_tuner_fine = com1_tuner_fine;
    static int last_com1_tuner_course = com1_tuner_course;
    inc = 0.0;
    if ( com1_tuner_fine != last_com1_tuner_fine ) {
	if ( com1_tuner_fine == 0x0c && last_com1_tuner_fine == 0x01 ) {
	    inc = -0.025;
	} else if ( com1_tuner_fine == 0x01 && last_com1_tuner_fine == 0x0c ) {
	    inc = -0.025;
	} else if ( com1_tuner_fine > last_com1_tuner_fine ) {
	    inc = 0.025;
	} else {
	    inc = -0.025;
	}
    }
    if ( com1_tuner_course != last_com1_tuner_course ) {
	if ( com1_tuner_course == 0x0c && last_com1_tuner_course == 0x01 ) {
	    inc = -1.0;
	} else if ( com1_tuner_course == 0x01
		    && last_com1_tuner_course == 0x0c ) {
	    inc = -1.0;
	} else if ( com1_tuner_course > last_com1_tuner_course ) {
	    inc = 1.0;
	} else {
	    inc = -1.0;
	}
    }
    last_com1_tuner_fine = com1_tuner_fine;
    last_com1_tuner_course = com1_tuner_course;

    freq = com1_stby_freq->getFloatValue() + inc;
    if ( freq < 0.0 ) {
	freq = 140.0;
    }
    if ( freq > 140.0 ) {
	freq = 0.0;
    }
    fgSetFloat( "/radios/comm[0]/frequencies/standby-mhz", freq );

    // Com2 Tuner
    int com2_tuner_fine = (radio_switch_data[13] >> 4) & 0x0f;
    int com2_tuner_course = radio_switch_data[13] & 0x0f;
    static int last_com2_tuner_fine = com2_tuner_fine;
    static int last_com2_tuner_course = com2_tuner_course;
    inc = 0.0;
    if ( com2_tuner_fine != last_com2_tuner_fine ) {
	if ( com2_tuner_fine == 0x0c && last_com2_tuner_fine == 0x01 ) {
	    inc = -0.025;
	} else if ( com2_tuner_fine == 0x01 && last_com2_tuner_fine == 0x0c ) {
	    inc = -0.025;
	} else if ( com2_tuner_fine > last_com2_tuner_fine ) {
	    inc = 0.025;
	} else {
	    inc = -0.025;
	}
    }
    if ( com2_tuner_course != last_com2_tuner_course ) {
	if ( com2_tuner_course == 0x0c && last_com2_tuner_course == 0x01 ) {
	    inc = -1.0;
	} else if ( com2_tuner_course == 0x01
		    && last_com2_tuner_course == 0x0c ) {
	    inc = -1.0;
	} else if ( com2_tuner_course > last_com2_tuner_course ) {
	    inc = 1.0;
	} else {
	    inc = -1.0;
	}
    }
    last_com2_tuner_fine = com2_tuner_fine;
    last_com2_tuner_course = com2_tuner_course;

    freq = com2_stby_freq->getFloatValue() + inc;
    if ( freq < 0.0 ) {
	freq = 140.0;
    }
    if ( freq > 140.0 ) {
	freq = 0.0;
    }
    fgSetFloat( "/radios/comm[1]/frequencies/standby-mhz", freq );

    // Nav1 Tuner
    int nav1_tuner_fine = (radio_switch_data[9] >> 4) & 0x0f;
    int nav1_tuner_course = radio_switch_data[9] & 0x0f;
    static int last_nav1_tuner_fine = nav1_tuner_fine;
    static int last_nav1_tuner_course = nav1_tuner_course;
    inc = 0.0;
    if ( nav1_tuner_fine != last_nav1_tuner_fine ) {
	if ( nav1_tuner_fine == 0x0c && last_nav1_tuner_fine == 0x01 ) {
	    inc = -0.05;
	} else if ( nav1_tuner_fine == 0x01 && last_nav1_tuner_fine == 0x0c ) {
	    inc = -0.05;
	} else if ( nav1_tuner_fine > last_nav1_tuner_fine ) {
	    inc = 0.05;
	} else {
	    inc = -0.05;
	}
    }
    if ( nav1_tuner_course != last_nav1_tuner_course ) {
	if ( nav1_tuner_course == 0x0c && last_nav1_tuner_course == 0x01 ) {
	    inc = -1.0;
	} else if ( nav1_tuner_course == 0x01
		    && last_nav1_tuner_course == 0x0c ) {
	    inc = -1.0;
	} else if ( nav1_tuner_course > last_nav1_tuner_course ) {
	    inc = 1.0;
	} else {
	    inc = -1.0;
	}
    }
    last_nav1_tuner_fine = nav1_tuner_fine;
    last_nav1_tuner_course = nav1_tuner_course;

    freq = nav1_stby_freq->getFloatValue() + inc;
    if ( freq < 108.0 ) {
	freq = 117.95;
    }
    if ( freq > 117.95 ) {
	freq = 108.0;
    }
    fgSetFloat( "/radios/nav[0]/frequencies/standby-mhz", freq );

    // Nav2 Tuner
    int nav2_tuner_fine = (radio_switch_data[17] >> 4) & 0x0f;
    int nav2_tuner_course = radio_switch_data[17] & 0x0f;
    static int last_nav2_tuner_fine = nav2_tuner_fine;
    static int last_nav2_tuner_course = nav2_tuner_course;
    inc = 0.0;
    if ( nav2_tuner_fine != last_nav2_tuner_fine ) {
	if ( nav2_tuner_fine == 0x0c && last_nav2_tuner_fine == 0x01 ) {
	    inc = -0.05;
	} else if ( nav2_tuner_fine == 0x01 && last_nav2_tuner_fine == 0x0c ) {
	    inc = -0.05;
	} else if ( nav2_tuner_fine > last_nav2_tuner_fine ) {
	    inc = 0.05;
	} else {
	    inc = -0.05;
	}
    }
    if ( nav2_tuner_course != last_nav2_tuner_course ) {
	if ( nav2_tuner_course == 0x0c && last_nav2_tuner_course == 0x01 ) {
	    inc = -1.0;
	} else if ( nav2_tuner_course == 0x01
		    && last_nav2_tuner_course == 0x0c ) {
	    inc = -1.0;
	} else if ( nav2_tuner_course > last_nav2_tuner_course ) {
	    inc = 1.0;
	} else {
	    inc = -1.0;
	}
    }
    last_nav2_tuner_fine = nav2_tuner_fine;
    last_nav2_tuner_course = nav2_tuner_course;

    freq = nav2_stby_freq->getFloatValue() + inc;
    if ( freq < 108.0 ) {
	freq = 117.95;
    }
    if ( freq > 117.95 ) {
	freq = 108.0;
    }
    fgSetFloat( "/radios/nav[1]/frequencies/standby-mhz", freq );

    // ADF Tuner
    int adf_tuner_fine = (radio_switch_data[21] >> 4) & 0x0f;
    int adf_tuner_course = radio_switch_data[21] & 0x0f;
    // cout << "adf = " << adf_tuner_fine << " " << adf_tuner_course << endl;
    static int last_adf_tuner_fine = adf_tuner_fine;
    static int last_adf_tuner_course = adf_tuner_course;
    inc = 0.0;
    if ( adf_tuner_fine != last_adf_tuner_fine ) {
	if ( adf_tuner_fine == 0x0c && last_adf_tuner_fine == 0x01 ) {
	    inc = -1.0;
	} else if ( adf_tuner_fine == 0x01 && last_adf_tuner_fine == 0x0c ) {
	    inc = -1.0;
	} else if ( adf_tuner_fine > last_adf_tuner_fine ) {
	    inc = 1.0;
	} else {
	    inc = -1.0;
	}
    }
    if ( adf_tuner_course != last_adf_tuner_course ) {
	if ( adf_tuner_course == 0x0c && last_adf_tuner_course == 0x01 ) {
	    inc = -25.0;
	} else if ( adf_tuner_course == 0x01
		    && last_adf_tuner_course == 0x0c ) {
	    inc = -25.0;
	} else if ( adf_tuner_course > last_adf_tuner_course ) {
	    inc = 25.0;
	} else {
	    inc = -25.0;
	}
    }
    last_adf_tuner_fine = adf_tuner_fine;
    last_adf_tuner_course = adf_tuner_course;

    freq = adf_freq->getFloatValue() + inc;
    if ( freq < 100.0 ) {
	freq = 1299;
    }
    if ( freq > 1299 ) {
	freq = 100.0;
    }
    fgSetFloat( "/radios/adf/frequencies/selected-khz", freq );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Update the radio display 
/////////////////////////////////////////////////////////////////////

bool FGATC610x::do_radio_display() {

    char digits[10];
    int i;

    if ( dme_switch != 0 ) {
	// DME minutes
	float minutes = dme_min->getFloatValue();
	if ( minutes > 999 ) {
	    minutes = 999.0;
	}
	sprintf(digits, "%03.0f", minutes);
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
	sprintf(digits, "%03.0f", knots);
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
	sprintf(digits, "%04.1f", nm);
	for ( i = 0; i < 6; ++i ) {
	    digits[i] -= '0';
	}
	radio_display_data[4] = digits[1] << 4 | digits[3];
	radio_display_data[5] = 0x00 | digits[0];
	// the 0x00 in the upper nibble of the 6th byte of each
	// display turns on the decimal point
    } else {
	// blank dem display
	for ( i = 0; i < 6; ++i ) {
	    radio_display_data[i] = 0xff;
	}
    }

    // Com1 standby frequency
    float com1_stby = com1_stby_freq->getFloatValue();
    if ( fabs(com1_stby) > 999.99 ) {
	com1_stby = 0.0;
    }
    sprintf(digits, "%06.3f", com1_stby);
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
    sprintf(digits, "%06.3f", com1);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[9] = digits[4] << 4 | digits[5];
    radio_display_data[10] = digits[1] << 4 | digits[2];
    radio_display_data[11] = 0x00 | digits[0];
    // the 0x00 in the upper nibble of the 6th byte of each display
    // turns on the decimal point

    // Com2 standby frequency
    float com2_stby = com2_stby_freq->getFloatValue();
    if ( fabs(com2_stby) > 999.99 ) {
	com2_stby = 0.0;
    }
    sprintf(digits, "%06.3f", com2_stby);
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
    sprintf(digits, "%06.3f", com2);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[21] = digits[4] << 4 | digits[5];
    radio_display_data[22] = digits[1] << 4 | digits[2];
    radio_display_data[23] = 0x00 | digits[0];
    // the 0x00 in the upper nibble of the 6th byte of each display
    // turns on the decimal point

    // Nav1 standby frequency
    float nav1_stby = nav1_stby_freq->getFloatValue();
    if ( fabs(nav1_stby) > 999.99 ) {
	nav1_stby = 0.0;
    }
    sprintf(digits, "%06.2f", nav1_stby);
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
    sprintf(digits, "%06.2f", nav1);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[15] = digits[4] << 4 | digits[5];
    radio_display_data[16] = digits[1] << 4 | digits[2];
    radio_display_data[17] = 0x00 | digits[0];
    // the 0x00 in the upper nibble of the 6th byte of each display
    // turns on the decimal point

    // Nav2 standby frequency
    float nav2_stby = nav2_stby_freq->getFloatValue();
    if ( fabs(nav2_stby) > 999.99 ) {
	nav2_stby = 0.0;
    }
    sprintf(digits, "%06.2f", nav2_stby);
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
    sprintf(digits, "%06.2f", nav2);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[27] = digits[4] << 4 | digits[5];
    radio_display_data[28] = digits[1] << 4 | digits[2];
    radio_display_data[29] = 0x00 | digits[0];
    // the 0x00 in the upper nibble of the 6th byte of each display
    // turns on the decimal point

    // ADF standby frequency
    float adf_stby = adf_stby_freq->getFloatValue();
    if ( fabs(adf_stby) > 999.99 ) {
	adf_stby = 0.0;
    }
    sprintf(digits, "%03.0f", adf_stby);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[30] = digits[2] << 4 | 0x0f;
    radio_display_data[31] = digits[0] << 4 | digits[1];

    // ADF in use frequency
    float adf = adf_freq->getFloatValue();
    if ( fabs(adf) > 999.99 ) {
	adf = 0.0;
    }
    sprintf(digits, "%03.0f", adf);
    for ( i = 0; i < 6; ++i ) {
	digits[i] -= '0';
    }
    radio_display_data[33] = digits[1] << 4 | digits[2];
    radio_display_data[34] = 0xf0 | digits[0];

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

    // magnetos and starter switch
    if ( switch_matrix[board][3][1] == 1 ) {
	fgSetInt( "/controls/magnetos[0]", 3 );
	fgSetBool( "/controls/starter[0]", true );
    } else if ( switch_matrix[board][2][1] == 1 ) {
	fgSetInt( "/controls/magnetos[0]", 3 );
	fgSetBool( "/controls/starter[0]", false );
    } else if ( switch_matrix[board][1][1] == 1 ) {
	fgSetInt( "/controls/magnetos[0]", 2 );
	fgSetBool( "/controls/starter[0]", false );
    } else if ( switch_matrix[board][0][1] == 1 ) {
	fgSetInt( "/controls/magnetos[0]", 1 );
	fgSetBool( "/controls/starter[0]", false );
    } else {
	fgSetInt( "/controls/magnetos[0]", 0 );
	fgSetBool( "/controls/starter[0]", false );
    }

    // flaps
    if ( switch_matrix[board][6][3] == 1 ) {
	fgSetFloat( "/controls/flaps", 1.0 );
    } else if ( switch_matrix[board][5][3] == 1 ) {
	fgSetFloat( "/controls/flaps", 0.66 );
    } else if ( switch_matrix[board][4][3] == 1 ) {
	fgSetFloat( "/controls/flaps", 0.33 );
    } else if ( switch_matrix[board][4][3] == 0 ) {
	fgSetFloat( "/controls/flaps", 0.0 );
    }

    return true;
}


bool FGATC610x::process() {

    // Lock the hardware, skip if it's not ready yet
    if ( ATC610xLock( lock_fd ) > 0 ) {

	do_analog_in();
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
