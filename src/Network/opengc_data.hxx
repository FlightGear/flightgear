// opengc.hxx -- define shared flight model parameters
//
// Version 1.0 by J. Wojnaroski for interface to Open Glass Displays
// Date: Nov 18,2001

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

#ifndef _OPENGC_HXX
#define _OPENGC_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

class ogcFGData {

public:

	// flight parameters
	
	double	pitch;
	double	bank;
	double	heading;
	double	altitude;
	double	altitude_agl;
	double 	v_kcas;
	double	groundspeed;
	double	vvi;

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

	// navigation data

	double	nav1_freq;
	double	nav1_radial;
	double	nav1_course_dev;

	double	nav2_freq;
	double	nav2_radial;
	double	nav2_course_dev;

	// some other locations to add stuff in   
    double d_ogcdata[16];
    float  f_ogcdata[16];
    int    i_ogcdata[16];
};

#endif // _OPENGC_HXX
