// ATCutils.cxx - Utility functions for the ATC / AI system
//
// Written by David Luff, started March 2002.
//
// Copyright (C) 2002  David C Luff - david.luff@nottingham.ac.uk
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

#include <math.h>
#include <simgear/math/point3d.hxx>
#include <simgear/constants.h>
#include <plib/sg.h>
#include <iomanip.h>

#include "ATCutils.hxx"

// Convert a 2 digit rwy number to a spoken-style string
string convertNumToSpokenString(int n) {
    string nums[10] = {"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};
    // Basic error/sanity checking
    while(n < 0) {
	n += 36;
    }
    while(n > 36) {
	n -= 36;
    }
    if(n == 0) {
	n = 36;	// Is this right?
    }

    string str = "";
    int index = n/10;
    str += nums[index];
    n -= (index * 10);
    str += "-";
    str += nums[n];
    return(str);
}

// Return the phonetic letter of a letter represented as an integer 1->26
string GetPhoneticIdent(int i) {
// TODO - Check i is between 1 and 26 and wrap if necessary
    switch(i) {
    case 1 : return("Alpha");
    case 2 : return("Bravo");
    case 3 : return("Charlie");
    case 4 : return("Delta");
    case 5 : return("Echo");
    case 6 : return("Foxtrot");
    case 7 : return("Golf");
    case 8 : return("Hotel");
    case 9 : return("Indigo");
    case 10 : return("Juliet");
    case 11 : return("Kilo");
    case 12 : return("Lima");
    case 13 : return("Mike");
    case 14 : return("November");
    case 15 : return("Oscar");
    case 16 : return("Papa");
    case 17 : return("Quebec");
    case 18 : return("Romeo");
    case 19 : return("Sierra");
    case 20 : return("Tango");
    case 21 : return("Uniform");
    case 22 : return("Victor");
    case 23 : return("Whiskey");
    case 24 : return("X-ray");
    case 25 : return("Yankee");
    case 26 : return("Zulu");
    }
    // We shouldn't get here
    return("Error");
}

// Given two positions, get the HORIZONTAL separation (in meters)
double dclGetHorizontalSeparation(Point3D pos1, Point3D pos2) {
    double x;	//East-West separation
    double y;	//North-South separation
    double z;	//Horizontal separation - z = sqrt(x^2 + y^2)

    double lat1 = pos1.lat() * SG_DEGREES_TO_RADIANS;
    double lon1 = pos1.lon() * SG_DEGREES_TO_RADIANS;
    double lat2 = pos2.lat() * SG_DEGREES_TO_RADIANS;
    double lon2 = pos2.lon() * SG_DEGREES_TO_RADIANS;

    y = sin(fabs(lat1 - lat2)) * SG_EQUATORIAL_RADIUS_M;
    x = sin(fabs(lon1 - lon2)) * SG_EQUATORIAL_RADIUS_M * (cos((lat1 + lat2) / 2.0));
    z = sqrt(x*x + y*y);

    return(z);
}

// Given a point and a line, get the HORIZONTAL shortest distance from the point to a point on the line.
// Expects to be fed orthogonal co-ordinates, NOT lat & lon !
double dclGetLinePointSeparation(double px, double py, double x1, double y1, double x2, double y2) {
    double vecx = x2-x1;
    double vecy = y2-y1;
    double magline = sqrt(vecx*vecx + vecy*vecy);
    double u = ((px-x1)*(x2-x1) + (py-y1)*(y2-y1)) / (magline * magline);
    double x0 = x1 + u*(x2-x1);
    double y0 = y1 + u*(y2-y1);
    vecx = px - x0;
    vecy = py - y0;
    double d = sqrt(vecx*vecx + vecy*vecy);
    if(d < 0) {
	d *= -1;
    }
    return(d);
}

// Given a position (lat/lon/elev), heading, vertical angle, and distance, calculate the new position.
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters.
Point3D dclUpdatePosition(Point3D pos, double heading, double angle, double distance) {
    //cout << setprecision(10) << pos.lon() << ' ' << pos.lat() << '\n';
    heading *= DCL_DEGREES_TO_RADIANS;
    angle *= DCL_DEGREES_TO_RADIANS;
    double lat = pos.lat() * DCL_DEGREES_TO_RADIANS;
    double lon = pos.lon() * DCL_DEGREES_TO_RADIANS;
    double elev = pos.elev();
    //cout << setprecision(10) << lon*DCL_RADIANS_TO_DEGREES << ' ' << lat*DCL_RADIANS_TO_DEGREES << '\n';

    double horiz_dist = distance * cos(angle);
    double vert_dist = distance * sin(angle);

    double north_dist = horiz_dist * cos(heading);
    double east_dist = horiz_dist * sin(heading);

    //cout << distance << ' ' << horiz_dist << ' ' << vert_dist << ' ' << north_dist << ' ' << east_dist << '\n';

    double delta_lat = asin(north_dist / (double)SG_EQUATORIAL_RADIUS_M);
    double delta_lon = asin(east_dist / (double)SG_EQUATORIAL_RADIUS_M) * (1.0 / cos(lat));  // I suppose really we should use the average of the original and new lat but we'll assume that this will be good enough.
    //cout << delta_lon*DCL_RADIANS_TO_DEGREES << ' ' << delta_lat*DCL_RADIANS_TO_DEGREES << '\n';
    lat += delta_lat;
    lon += delta_lon;
    elev += vert_dist;
    //cout << setprecision(10) << lon*DCL_RADIANS_TO_DEGREES << ' ' << lat*DCL_RADIANS_TO_DEGREES << '\n';

    //cout << setprecision(15) << DCL_DEGREES_TO_RADIANS * DCL_RADIANS_TO_DEGREES << '\n';

    return(Point3D(lon*DCL_RADIANS_TO_DEGREES, lat*DCL_RADIANS_TO_DEGREES, elev));
}
    

#if 0
/* Determine location in runway coordinates */

	Radius_to_rwy = Sea_level_radius + Runway_altitude;
	cos_rwy_hdg = cos(Runway_heading*DEG_TO_RAD);
	sin_rwy_hdg = sin(Runway_heading*DEG_TO_RAD);
	
	D_cg_north_of_rwy = Radius_to_rwy*(Latitude - Runway_latitude);
	D_cg_east_of_rwy = Radius_to_rwy*cos(Runway_latitude)
		*(Longitude - Runway_longitude);
	D_cg_above_rwy	= Radius_to_vehicle - Radius_to_rwy;
	
	X_cg_rwy = D_cg_north_of_rwy*cos_rwy_hdg 
	  + D_cg_east_of_rwy*sin_rwy_hdg;
	Y_cg_rwy =-D_cg_north_of_rwy*sin_rwy_hdg 
	  + D_cg_east_of_rwy*cos_rwy_hdg;
	H_cg_rwy = D_cg_above_rwy;    
#endif
