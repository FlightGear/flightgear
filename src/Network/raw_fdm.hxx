// raw_fdm.hxx -- defines a common raw I/O interface to the flight
//                dynamics model
//
// Written by Curtis Olson, started September 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - curt@flightgear.com
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


#ifndef _RAW_FDM_HXX
#define _RAW_FDM_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

const int FG_RAW_FDM_VERSION = 1;

// Define a structure containing the top level flight dynamics model
// parameters

class FGRawFDM {

public:

    int version;		// increment when data values change
    double longitude;		// radians
    double latitude;		// radians
    double altitude;		// meters (above sea level)
    double agl;			// meters (altitude above ground level)
    double phi;			// radians
    double theta;		// radians
    double psi;			// radians
};


#endif // _RAW_FDM_HXX


