// polar.cxx -- routines to deal with polar math and transformations
//
// Written by Curtis Olson, started June 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#include <math.h>
#include <stdio.h>

#include <Include/fg_constants.h>

#include "polar3d.hxx"


// Find the Altitude above the Ellipsoid (WGS84) given the Earth
// Centered Cartesian coordinate vector Distances are specified in
// meters.
double fgGeodAltFromCart(const Point3D& cp)
{
    double t_lat, x_alpha, mu_alpha;
    double lat_geoc, radius;
    double result;

    lat_geoc = FG_PI_2 - atan2( sqrt(cp.x()*cp.x() + cp.y()*cp.y()), cp.z() );
    radius = sqrt( cp.x()*cp.x() + cp.y()*cp.y() + cp.z()*cp.z() );
	
    if( ( (FG_PI_2 - lat_geoc) < ONE_SECOND )        // near North pole
	|| ( (FG_PI_2 + lat_geoc) < ONE_SECOND ) )   // near South pole
    {
	result = radius - EQUATORIAL_RADIUS_M*E;
    } else {
	t_lat = tan(lat_geoc);
	x_alpha = E*EQUATORIAL_RADIUS_M/sqrt(t_lat*t_lat + E*E);
	mu_alpha = atan2(sqrt(RESQ_M - x_alpha*x_alpha),E*x_alpha);
	if (lat_geoc < 0) {
	    mu_alpha = - mu_alpha;
	}
	result = (radius - x_alpha/cos(lat_geoc))*cos(mu_alpha - lat_geoc);
    }

    return(result);
}


