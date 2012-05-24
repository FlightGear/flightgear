// ATC-Outputs.hxx -- Translate FGFS properties to ATC hardware outputs.
//
// Written by Curtis Olson, started November 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include <config.h>
#endif

#include <simgear/compiler.h>

#if defined( unix ) || defined( __CYGWIN__ )
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <ostream>
#endif

#include <errno.h>
#include <math.h>

#include <string>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>

#include "ATC-Outputs.hxx"

using std::string;



// Lock the ATC hardware
static int ATCLock( int fd ) {
#if defined( unix ) || defined( __CYGWIN__ )
    // rewind
    lseek( fd, 0, SEEK_SET );

    char tmp[2];
    int result = read( fd, tmp, 1 );
    if ( result != 1 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Lock failed" );
    }

    return result;
#else
    return -1;
#endif
}


// Release the ATC hardware
static int ATCRelease( int fd ) {
#if defined( unix ) || defined( __CYGWIN__ )
    // rewind
    lseek( fd, 0, SEEK_SET );

    char tmp[2];
    tmp[0] = tmp[1] = 0;
    int result = write( fd, tmp, 1 );

    if ( result != 1 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Release failed" );
    }

    return result;
#else
    return -1;
#endif
}


// Constructor: The _board parameter specifies which board to
// reference.  Possible values are 0 or 1.  The _config_file parameter
// specifies the location of the output config file (xml)
FGATCOutput::FGATCOutput( const int _board, const SGPath &_config_file ) :
    is_open(false),
    analog_out_node(NULL),
    lamps_out_node(NULL),
    radio_display_node(NULL),
    steppers_node(NULL)
{
    board = _board;
    config = _config_file;
}


// Write analog out data
static int ATCSetAnalogOut( int fd,
			    unsigned char data[ATC_ANALOG_OUT_CHANNELS*2] )
{
#if defined( unix ) || defined( __CYGWIN__ )
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = write( fd, data, ATC_ANALOG_OUT_CHANNELS*2 );

    if ( result != ATC_ANALOG_OUT_CHANNELS*2 ) {
	SG_LOG( SG_IO, SG_DEBUG, "Write failed" );
    }

    return result;
#else
    return -1;
#endif
}


// Write a radios command
static int ATCSetRadios( int fd, unsigned char data[ATC_RADIO_DISPLAY_BYTES] ) {
#if defined( unix ) || defined( __CYGWIN__ )
    // rewind
    lseek( fd, 0, SEEK_SET );

    int result = write( fd, data, ATC_RADIO_DISPLAY_BYTES );

    if ( result != ATC_RADIO_DISPLAY_BYTES ) {
	SG_LOG( SG_IO, SG_DEBUG, "Write failed" );
    }

    return result;
#else
    return -1;
#endif
}


// Write a stepper command
static int ATCSetStepper( int fd, unsigned char channel,
			      unsigned char value )
{
#if defined( unix ) || defined( __CYGWIN__ )
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
#else
    return -1;
#endif
}


#ifdef ATCFLIGHTSIM_HAVE_COMPASS
// Read status of last stepper written to
static unsigned char ATCReadStepper( int fd ) {
#if defined( unix ) || defined( __CYGWIN__ )
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
#else
    return 0;
#endif
}
#endif


// Turn a lamp on or off
void ATCSetLamp( int fd, int channel, bool value ) {
#if defined( unix ) || defined( __CYGWIN__ )
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
#endif
}


void FGATCOutput::init_config() {
#if defined( unix ) || defined( __CYGWIN__ )
    if ( config.str()[0] != '/' ) {
        // not an absolute path, prepend the standard location
        SGPath tmp;
        char *envp = ::getenv( "HOME" );
        if ( envp != NULL ) {
            tmp = envp;
            tmp.append( ".atcflightsim" );
            tmp.append( config.str() );
            config = tmp;
        }
    }
    readProperties( config.str(), globals->get_props() );
#endif
}


// Open and initialize the ATC hardware
bool FGATCOutput::open( int lock_fd ) {
    if ( is_open ) {
	SG_LOG( SG_IO, SG_ALERT, "This board is already open for output! "
                << board );
	return false;
    }

    // This loads the config parameters generated by "simcal"
    init_config();

    SG_LOG( SG_IO, SG_ALERT,
	    "Initializing ATC output hardware, please wait ..." );

    snprintf( analog_out_file, 256,
              "/proc/atcflightsim/board%d/analog_out", board );
    snprintf( lamps_file, 256,
              "/proc/atcflightsim/board%d/lamps", board );
    snprintf( radio_display_file, 256,
              "/proc/atcflightsim/board%d/radios", board );
    snprintf( stepper_file, 256,
              "/proc/atcflightsim/board%d/steppers", board );

#if defined( unix ) || defined( __CYGWIN__ )

    /////////////////////////////////////////////////////////////////////
    // Open the /proc files
    /////////////////////////////////////////////////////////////////////

    analog_out_fd = ::open( analog_out_file, O_WRONLY );
    if ( analog_out_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", analog_out_file );
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

    radio_display_fd = ::open( radio_display_file, O_RDWR );
    if ( radio_display_fd == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error opening %s", radio_display_file );
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

#endif

#ifdef ATCFLIGHTSIM_HAVE_COMPASS
    /////////////////////////////////////////////////////////////////////
    // Home the compass stepper motor
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "  - Homing the compass stepper motor" );

    // Lock the hardware, keep trying until we succeed
    while ( ATCLock( lock_fd ) <= 0 );

    // Send the stepper home command
    ATCSetStepper( stepper_fd, ATC_COMPASS_CH, ATC_STEPPER_HOME );

    // Release the hardware
    ATCRelease( lock_fd );

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

	while ( ATCLock( lock_fd ) <= 0 );

	unsigned char result = ATCReadStepper( stepper_fd );
	if ( result == 0 ) {
	    home = true;
	}

	ATCRelease( lock_fd );

#ifdef _WIN32
        Sleep (33);
#else
	usleep(33);
#endif

        --timeout;
    }

    compass_position = 0.0;
#endif

    // Lock the hardware, keep trying until we succeed
    while ( ATCLock( lock_fd ) <= 0 );

    /////////////////////////////////////////////////////////////////////
    // Zero the analog outputs
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "  - Zeroing Analog Outputs." );

    for ( int channel = 0; channel < ATC_ANALOG_OUT_CHANNELS; ++channel ) {
	analog_out_data[2*channel] = 0;
	analog_out_data[2*channel + 1] = 0;
    }
    ATCSetAnalogOut( analog_out_fd, analog_out_data );


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
    ATCSetRadios( radio_display_fd, radio_display_data );

    ATCRelease( lock_fd );

    /////////////////////////////////////////////////////////////////////
    // Blank the lamps
    /////////////////////////////////////////////////////////////////////

    for ( int i = 0; i < 128; ++i ) {
        ATCSetLamp( lamps_fd, i, false );
    }

    /////////////////////////////////////////////////////////////////////
    // Finished initing hardware
    /////////////////////////////////////////////////////////////////////

    SG_LOG( SG_IO, SG_ALERT,
	    "Done initializing ATC output hardware." );

    is_open = true;

    /////////////////////////////////////////////////////////////////////
    // Connect up to property values
    /////////////////////////////////////////////////////////////////////

    char base_name[256];

    snprintf( base_name, 256, "/output/atc-board[%d]/analog-outputs", board );
    analog_out_node = fgGetNode( base_name );

    snprintf( base_name, 256, "/output/atc-board[%d]/lamps", board );
    lamps_out_node = fgGetNode( base_name );

    snprintf( base_name, 256, "/output/atc-board[%d]/radio-display", board );
    radio_display_node = fgGetNode( base_name );

    snprintf( base_name, 256, "/output/atc-board[%d]/steppers", board );
    steppers_node = fgGetNode( base_name );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Write the lanalog outputs
/////////////////////////////////////////////////////////////////////

bool FGATCOutput::do_analog_out() {
    if ( analog_out_node != NULL ) {
        for ( int i = 0; i < analog_out_node->nChildren(); ++i ) {
            // read the next config entry from the property tree

            SGPropertyNode *child = analog_out_node->getChild(i);
            string cname = child->getName();
            int index = child->getIndex();
            string name = "";
            string type = "";
            SGPropertyNode *src_prop = NULL;
	    double x0 = 0.0, y0 = 0.0, x1 = 0.0, y1 = 0.0;
            if ( cname == "analog-out" ) {
                SGPropertyNode *prop;
                prop = child->getChild( "name" );
                if ( prop != NULL ) {
                    name = prop->getStringValue();
                }
                prop = child->getChild( "type" );
                if ( prop != NULL ) {
                    type = prop->getStringValue();
                }
                prop = child->getChild( "prop" );
                if ( prop != NULL ) {
                    src_prop = fgGetNode( prop->getStringValue(), true );
                }
                prop = child->getChild( "value-lo" );
                if ( prop != NULL ) {
		    x0 = prop->getDoubleValue();
                }
                prop = child->getChild( "meter-lo" );
                if ( prop != NULL ) {
		    y0 = prop->getDoubleValue();
                }
                prop = child->getChild( "value-hi" );
                if ( prop != NULL ) {
		    x1 = prop->getDoubleValue();
                }
                prop = child->getChild( "meter-hi" );
                if ( prop != NULL ) {
		    y1 = prop->getDoubleValue();
                }
		// crunch linear interpolation formula
		double dx = x1 - x0;
		double dy = y1 - y0;
		double slope = dy / dx;
		double value = src_prop->getDoubleValue();
		int meter = (value - x0) * slope + y0;
		if ( meter < 0 ) { meter = 0; }
		if ( meter > 1023 ) { meter = 1023; }
		analog_out_data[2*index] = meter / 256;
		analog_out_data[2*index + 1] = meter - analog_out_data[2*index] * 256;
           } else {
                SG_LOG( SG_IO, SG_DEBUG,
                        "Input config error, expecting 'analog-out' but found "
                        << cname );
            }
	    ATCSetAnalogOut( analog_out_fd, analog_out_data );
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////
// Write the lights
/////////////////////////////////////////////////////////////////////

bool FGATCOutput::do_lamps() {
    if ( lamps_out_node != NULL ) {
        for ( int i = 0; i < lamps_out_node->nChildren(); ++i ) {
            // read the next config entry from the property tree

            SGPropertyNode *child = lamps_out_node->getChild(i);
            string cname = child->getName();
            int index = child->getIndex();
            string name = "";
            string type = "";
            SGPropertyNode *src_prop = NULL;
            if ( cname == "lamp" ) {
                SGPropertyNode *prop;
                prop = child->getChild( "name" );
                if ( prop != NULL ) {
                    name = prop->getStringValue();
                }
                prop = child->getChild( "type" );
                if ( prop != NULL ) {
                    type = prop->getStringValue();
                }
                prop = child->getChild( "prop" );
                if ( prop != NULL ) {
                    src_prop = fgGetNode( prop->getStringValue(), true );
                }
                ATCSetLamp( lamps_fd, index, src_prop->getBoolValue() );
            } else {
                SG_LOG( SG_IO, SG_DEBUG,
                        "Input config error, expecting 'lamp' but found "
                        << cname );
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////////////////////////
// Update the radio display 
/////////////////////////////////////////////////////////////////////


static bool navcom1_has_power() {
    static SGPropertyNode *navcom1_bus_power
        = fgGetNode( "/systems/electrical/outputs/nav[0]", true );
    static SGPropertyNode *navcom1_power_btn
        = fgGetNode( "/instrumentation/nav[0]/power-btn", true );

    return (navcom1_bus_power->getDoubleValue() > 1.0)
        && navcom1_power_btn->getBoolValue();
}

static bool navcom2_has_power() {
    static SGPropertyNode *navcom2_bus_power
        = fgGetNode( "/systems/electrical/outputs/nav[1]", true );
    static SGPropertyNode *navcom2_power_btn
        = fgGetNode( "/instrumentation/nav[1]/power-btn", true );

    return (navcom2_bus_power->getDoubleValue() > 1.0)
        && navcom2_power_btn->getBoolValue();
}

static bool dme_has_power() {
    static SGPropertyNode *dme_bus_power
        = fgGetNode( "/systems/electrical/outputs/dme", true );
    
    return (dme_bus_power->getDoubleValue() > 1.0);
}

static bool adf_has_power() {
    static SGPropertyNode *adf_bus_power
        = fgGetNode( "/systems/electrical/outputs/adf", true );
    static SGPropertyNode *adf_power_btn
        = fgGetNode( "/instrumentation/kr-87/inputs/power-btn", true );

    return (adf_bus_power->getDoubleValue() > 1.0)
        && adf_power_btn->getBoolValue();
}

static bool xpdr_has_power() {
    static SGPropertyNode *xpdr_bus_power
        = fgGetNode( "/systems/electrical/outputs/transponder", true );
    static SGPropertyNode *xpdr_func_knob
        = fgGetNode( "/instrumentation/transponder/inputs/func-knob", true );

    return (xpdr_bus_power->getDoubleValue() > 1.0)
        && (xpdr_func_knob->getIntValue() > 0);
}

bool FGATCOutput::do_radio_display() {
    static SGPropertyNode *dme_serviceable
        = fgGetNode( "/instrumentation/dme/serviceable", true );
    static SGPropertyNode *dme_in_range
        = fgGetNode( "/instrumentation/dme/in-range", true );
    static SGPropertyNode *dme_min
        = fgGetNode( "/instrumentation/dme/indicated-time-min", true );
    static SGPropertyNode *dme_kt
        = fgGetNode( "/instrumentation/dme/indicated-ground-speed-kt", true );
    static SGPropertyNode *dme_nm
        = fgGetNode( "/instrumentation/dme/indicated-distance-nm", true );

    static SGPropertyNode *comm1_serviceable
        = fgGetNode( "/instrumentation/comm[0]/serviceable", true );
    static SGPropertyNode *com1_freq
        = fgGetNode( "/instrumentation/comm[0]/frequencies/selected-mhz", true);
    static SGPropertyNode *com1_stby_freq
	= fgGetNode( "/instrumentation/comm[0]/frequencies/standby-mhz", true );

    static SGPropertyNode *comm2_serviceable
        = fgGetNode( "/instrumentation/comm[1]/serviceable", true );
    static SGPropertyNode *com2_freq
        = fgGetNode( "/instrumentation/comm[1]/frequencies/selected-mhz", true);
    static SGPropertyNode *com2_stby_freq
	= fgGetNode( "/instrumentation/comm[1]/frequencies/standby-mhz", true );

    static SGPropertyNode *nav1_serviceable
        = fgGetNode( "/instrumentation/nav[0]/serviceable", true );
    static SGPropertyNode *nav1_freq
        = fgGetNode( "/instrumentation/nav[0]/frequencies/selected-mhz", true );
    static SGPropertyNode *nav1_stby_freq
	= fgGetNode( "/instrumentation/nav[0]/frequencies/standby-mhz", true );

    static SGPropertyNode *nav2_serviceable
        = fgGetNode( "/instrumentation/nav[1]/serviceable", true );
    static SGPropertyNode *nav2_freq
        = fgGetNode( "/instrumentation/nav[1]/frequencies/selected-mhz", true );
    static SGPropertyNode *nav2_stby_freq
        = fgGetNode( "/instrumentation/nav[1]/frequencies/standby-mhz", true );

    static SGPropertyNode *adf_serviceable
        = fgGetNode( "/instrumentation/adf/serviceable", true );
    static SGPropertyNode *adf_freq
        = fgGetNode( "/instrumentation/kr-87/outputs/selected-khz", true );
    static SGPropertyNode *adf_stby_freq
        = fgGetNode( "/instrumentation/kr-87/outputs/standby-khz", true );
    static SGPropertyNode *adf_stby_mode
        = fgGetNode( "/instrumentation/kr-87/modes/stby", true );
    static SGPropertyNode *adf_timer_mode
        = fgGetNode( "/instrumentation/kr-87/modes/timer", true );
    // static SGPropertyNode *adf_count_mode
    //     = fgGetNode( "/instrumentation/kr-87/modes/count", true );
    static SGPropertyNode *adf_flight_timer
        = fgGetNode( "/instrumentation/kr-87/outputs/flight-timer", true );
    static SGPropertyNode *adf_elapsed_timer
        = fgGetNode( "/instrumentation/kr-87/outputs/elapsed-timer", true );

    static SGPropertyNode *xpdr_serviceable
        = fgGetNode( "/instrumentation/transponder/inputs/serviceable", true );
    static SGPropertyNode *xpdr_func_knob
        = fgGetNode( "/instrumentation/transponder/inputs/func-knob", true );
    static SGPropertyNode *xpdr_flight_level
        = fgGetNode( "/instrumentation/transponder/outputs/flight-level", true );
    static SGPropertyNode *xpdr_id_code
        = fgGetNode( "/instrumentation/transponder/outputs/id-code", true );

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

    ATCSetRadios( radio_display_fd, radio_display_data );

    return true;
}


/////////////////////////////////////////////////////////////////////
// Drive the stepper motors
/////////////////////////////////////////////////////////////////////

bool FGATCOutput::do_steppers() {
    SGPropertyNode *mag_compass
        = fgGetNode( "/instrumentation/magnetic-compass/indicated-heading-deg",
                     true );

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

	ATCSetStepper( stepper_fd, ATC_COMPASS_CH, cmd );
    }

    return true;
}


// process the hardware outputs.  This code assumes the calling layer
// will lock the hardware.
bool FGATCOutput::process() {
    if ( !is_open ) {
	SG_LOG( SG_IO, SG_ALERT, "This board has not been opened for output! "
                << board );
	return false;
    }

    do_analog_out();
    do_lamps();
    do_radio_display();
#ifdef ATCFLIGHTSIM_HAVE_COMPASS
    do_steppers();
#endif
	
    return true;
}


bool FGATCOutput::close() {

#if defined( unix ) || defined( __CYGWIN__ )

    if ( !is_open ) {
        return true;
    }

    int result;

    result = ::close( lamps_fd );
    if ( result == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error closing %s", lamps_file );
	perror( msg );
	exit( -1 );
    }

    result = ::close( radio_display_fd );
    if ( result == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error closing %s", radio_display_file );
	perror( msg );
	exit( -1 );
    }

    result = ::close( stepper_fd );
    if ( result == -1 ) {
	SG_LOG( SG_IO, SG_ALERT, "errno = " << errno );
	char msg[256];
	snprintf( msg, 256, "Error closing %s", stepper_file );
	perror( msg );
	exit( -1 );
    }

#endif

    return true;
}
