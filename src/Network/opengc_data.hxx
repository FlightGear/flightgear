// opengc.hxx -- Define structure of OpenGC/FG interface parameters
//
//  Version by J. Wojnaroski for interface to Open Glass Displays
//
//  Modified 02/12/01 - Update engine structure for multi-engine models
//		      - Added data preamble to id msg types
//
// This file defines the class/structure of the UDP packet that sends
// the simulation data created by FlightGear to the glass displays. It
// is required to "sync" the data types contained in the packet
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

#ifndef _OPENGC_DATA_HXX
#define _OPENGC_DATA_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <string>

class ogcFGData {

public:

    // defines msg types and contents. typedefs & classes are TBS
    unsigned int	version_id;
    unsigned int	msg_id;
    unsigned int	msg_content;
    unsigned short	autopilot_mode;
    unsigned short	afds_armed_mode;

    // flight parameters
	
    double	pitch;
    double	bank;
    double	heading;
    double	altitude;
    double	altitude_agl;  // this can also be the radar altimeter
    double 	v_kcas;
    double	groundspeed;
    double	vvi;
    double  	mach;

    // position

    double	latitude;
    double	longitude;
    double	magvar;

    // engine data

    double	rpm[4];
    double	epr[4];
    double	egt[4];
    double	fuel_flow[4];
    double	oil_pressure[4];

    double n1_turbine[4];
    double n2_turbine[4];
	
    // navigation data
    // This will require a lot of related work to build the nav
    // database for the ND and some msg traffic both ways to display
    // and configure nav aids, freqs, courses, etc etc.  OpenGC will
    // most likely define an FMC to hold the database and do nav stuff
    double	nav1_freq;
    double	nav1_radial;
    double	nav1_course_dev;
    double	nav1_gs_dev;
    double	nav1_dme;

    double	nav2_freq;
    double	nav2_radial;
    double	nav2_course_dev;
    double 	nav2_gs_dev;
    double	nav2_dme;

    // some other locations serving as place holders for user defined
    // variables reduces the need to update FG or OpenGC code
 
    double d_ogcdata[16];
    float  f_ogcdata[16];
    int    i_ogcdata[16];
};

#endif // _OPENGC_HXX
