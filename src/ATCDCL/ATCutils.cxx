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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sstream>
#include <cstdlib>

#include <simgear/constants.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include <Airports/runways.hxx>
#include <Main/globals.hxx>

#include "ATCutils.hxx"
#include "ATCProjection.hxx"

static const string nums[10] = {"zero", "one", "two", "three", "four",
				"five", "six", "seven", "eight", "niner"}; 

static const string letters[LTRS] = {
    "alpha",    "bravo",    "charlie",   "delta",     "echo",
    "foxtrot",  "golf",     "hotel",     "india",     "juliet",
    "kilo",     "lima",     "mike",      "november",  "oscar",
    "papa",     "quebec",   "romeo",     "sierra",    "tango",
    "uniform",  "victor",   "whiskey",   "xray",      "yankee",    "zulu"
};

// Convert any number to spoken digits
string ConvertNumToSpokenDigits(const string &n) {
	//cout << "n = " << n << endl;
	static const string pt = "decimal";
	string str = "";
	
	for(unsigned int i=0; i<n.length(); ++i) {
		//cout << "n.substr(" << i << ",1 = " << n.substr(i,1) << endl;
		if(n.substr(i,1) == " ") {
			// do nothing
		} else if(n.substr(i,1) == ".") {
			str += pt;
		} else {
			str += nums[atoi((n.substr(i,1)).c_str())];
		}
		if(i != (n.length()-1)) {	// ie. don't add a space at the end.
			str += " ";
		}
	}
	return(str);
}

// Convert an integer to a decimal numeral string
string decimalNumeral(const int& n) {
	std::ostringstream buf;
	buf << n;
	return buf.str();
}

// Convert an integer to spoken digits
string ConvertNumToSpokenDigits(const int& n) {
	return ConvertNumToSpokenDigits(decimalNumeral(n));
}


// Assumes we get a string of digits optionally appended with L, R or C
// eg 1 7L 29R 36
// Anything else is not guaranteed to be handled correctly!
string ConvertRwyNumToSpokenString(const string &rwy) {
  string rslt;
  for (size_t ii = 0; ii < rwy.length(); ii++){
    if (rslt.length()) rslt += " ";
    string ch = rwy.substr(ii,1);
    if (isdigit(ch[0])) rslt += ConvertNumToSpokenDigits(atoi(ch.c_str()));
    else if (ch == "R") rslt += "right";
    else if (ch == "C") rslt += "center";
    else if (ch == "L") rslt += "left";
    else {
      rslt += GetPhoneticLetter(ch[0]);
      SG_LOG(SG_ATC, SG_WARN, "WARNING: Unknown suffix '" << ch 
	  << "' in runway " << rwy << " in ConvertRwyNumToSpokenString(...)");
    }
  }
  return rslt;
}
	

// Return the phonetic letter of a letter represented as an integer 1->26
string GetPhoneticLetter(const int i) {
	return(letters[i % LTRS]);
}

// Return the phonetic letter of a character in the range a-z or A-Z.
// Currently always returns prefixed by lowercase.
string GetPhoneticLetter(const char c) {
	return GetPhoneticLetter(int(tolower(c) - 'a'));
}

// Get the compass direction associated with a heading in degrees
// Currently returns 8 direction resolution (N, NE, E etc...)
// Might be modified in future to return 4, 8 or 16 resolution but defaulting to 8. 
string GetCompassDirection(double h) {
	while(h < 0.0) h += 360.0;
	while(h > 360.0) h -= 360.0;
	if(h < 22.5 || h > 337.5) {
		return("North");
	} else if(h < 67.5) {
		return("North-East");
	} else if(h < 112.5) {
		return("East");
	} else if(h < 157.5) {
		return("South-East");
	} else if(h < 202.5) {
		return("South");
	} else if(h < 247.5) {
		return("South-West");
	} else if(h < 292.5) {
		return("West");
	} else {
		return("North-West");
	}
}

//================================================================================================================

// Given two positions (lat & lon in degrees), get the HORIZONTAL separation (in meters)
double dclGetHorizontalSeparation(const SGGeod& pos1, const SGGeod& pos2) {
	double x;	//East-West separation
	double y;	//North-South separation
	double z;	//Horizontal separation - z = sqrt(x^2 + y^2)
	
	double lat1 = pos1.getLatitudeRad();
	double lon1 = pos1.getLongitudeRad();
	double lat2 = pos2.getLatitudeRad();
	double lon2 = pos2.getLongitudeRad();
	
	y = sin(fabs(lat1 - lat2)) * SG_EQUATORIAL_RADIUS_M;
	x = sin(fabs(lon1 - lon2)) * SG_EQUATORIAL_RADIUS_M * (cos((lat1 + lat2) / 2.0));
	z = sqrt(x*x + y*y);
	
	return(z);
}

// Given a point and a line, get the HORIZONTAL shortest distance from the point to a point on the line.
// Expects to be fed orthogonal co-ordinates, NOT lat & lon !
// The units of the separation will be those of the input.
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

// Given a position (lat/lon/elev), heading and vertical angle (degrees), and distance (meters), calculate the new position.
// This function assumes the world is spherical.  If geodetic accuracy is required use the functions is sg_geodesy instead!
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters. 
SGGeod dclUpdatePosition(const SGGeod& pos, double heading, double angle, double distance) {
    // FIXME: use SGGeodesy instead ...

	//cout << setprecision(10) << pos.lon() << ' ' << pos.lat() << '\n';
	heading *= DCL_DEGREES_TO_RADIANS;
	angle *= DCL_DEGREES_TO_RADIANS;
	double lat = pos.getLatitudeRad();
	double lon = pos.getLongitudeRad();
	double elev = pos.getElevationM();
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
	
	return SGGeod::fromRadM(lon, lat, elev);
}

// Get a heading in degrees from one lat/lon to another.
// This function assumes the world is spherical.  If geodetic accuracy is required use the functions is sg_geodesy instead!
// Warning - at the moment we are not checking for identical points - currently it returns 0 in this instance.
double GetHeadingFromTo(const SGGeod& A, const SGGeod& B) {
	double latA = A.getLatitudeRad();
	double lonA = A.getLongitudeRad();
	double latB = B.getLatitudeRad();
	double lonB = B.getLongitudeRad();
	double xdist = sin(lonB - lonA) * (double)SG_EQUATORIAL_RADIUS_M * cos((latA+latB)/2.0);
	double ydist = sin(latB - latA) * (double)SG_EQUATORIAL_RADIUS_M;
	double heading = atan2(xdist, ydist) * DCL_RADIANS_TO_DEGREES;
	return heading < 0.0 ? heading + 360 : heading;
}

// Given a heading (in degrees), bound it from 0 -> 360
void dclBoundHeading(double &hdg) {
	while(hdg < 0.0) {
		hdg += 360.0;
	}
	while(hdg > 360.0) {
		hdg -= 360.0;
	}
}

// smallest difference between two angles in degrees
// difference is negative if a1 > a2 and positive if a2 > a1
double GetAngleDiff_deg( const double &a1, const double &a2) {
  
  double a3 = a2 - a1;
  while (a3 < 180.0) a3 += 360.0;
  while (a3 > 180.0) a3 -= 360.0;

  return a3;
}

// Runway stuff
// Given (lon/lat/elev) and an FGRunway struct, determine if the point lies on the runway
bool OnRunway(const SGGeod& pt, const FGRunwayBase* rwy) {
	FGATCAlignedProjection ortho;
	SGGeod centre = SGGeod::fromDegM(rwy->longitude(), rwy->latitude(), 0);	// We don't need the elev
	ortho.Init(centre, rwy->headingDeg());
	
	SGVec3d xyc = ortho.ConvertToLocal(centre);
	SGVec3d xyp = ortho.ConvertToLocal(pt);
	
	//cout << "Length offset = " << fabs(xyp.y() - xyc.y()) << '\n';
	//cout << "Width offset = " << fabs(xyp.x() - xyc.x()) << '\n';
	
	if((fabs(xyp.y() - xyc.y()) < ((rwy->lengthFt()/2.0) + 5.0)) 
		&& (fabs(xyp.x() - xyc.x()) < (rwy->widthFt()/2.0))) {
		return(true);
	}
	
	return(false);
}

