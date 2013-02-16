// ATCutils.hxx - Utility functions for the ATC / AI subsytem
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

#include <Airports/simple.hxx>
#include <Airports/runways.hxx>

#include <math.h>
#include <string>
using std::string;

// These are defined here because I had a problem with SG_DEGREES_TO_RADIANS
#define DCL_PI  3.1415926535f
#define DCL_DEGREES_TO_RADIANS  (DCL_PI/180.0)
#define DCL_RADIANS_TO_DEGREES  (180.0/DCL_PI)

/*******************************
*
*  Communication functions
*
********************************/

// Convert any number to spoken digits
string ConvertNumToSpokenDigits(const string &n);

// Convert an integer to spoken digits
string ConvertNumToSpokenDigits(const int& n);
string decimalNumeral(const int& n);

// Convert rwy number string to a spoken-style string
// eg "15L" to "one five left"
// Assumes we get a string of digits optionally appended with R, L, or C
// eg 1 7L 29R 36
string ConvertRwyNumToSpokenString(const string &s);

const int LTRS(26);
// Return the phonetic letter of a letter represented as an integer 0..25
string GetPhoneticLetter(const int i);

// Return the phonetic letter of a character in the range a-z or A-Z.
// Currently always returns prefixed by lowercase.
string GetPhoneticLetter(char c);

// Get the compass direction associated with a heading in degrees
// Currently returns 8 direction resolution (N, NE, E etc...)
// Might be modified in future to return 4, 8 or 16 resolution but defaulting to 8. 
string GetCompassDirection(double h);

/*******************************
*
*  Positional functions
*
********************************/

// Given two positions (lat & lon in degrees), get the HORIZONTAL separation (in meters)
double dclGetHorizontalSeparation(const SGGeod& pos1, const SGGeod& pos2);

// Given a point and a line, get the HORIZONTAL shortest distance from the point to a point on the line.
// Expects to be fed orthogonal co-ordinates, NOT lat & lon !
double dclGetLinePointSeparation(double px, double py, double x1, double y1, double x2, double y2);

// Given a position (lat/lon/elev), heading, vertical angle, and distance, calculate the new position.
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters.
SGGeod dclUpdatePosition(const SGGeod& pos, double heading, double angle, double distance);

// Get a heading from one lat/lon to another (in degrees)
double GetHeadingFromTo(const SGGeod& A, const SGGeod& B);

// Given a heading (in degrees), bound it from 0 -> 360
void dclBoundHeading(double &hdg);

// smallest difference between two angles in degrees
// difference is negative if a1 > a2 and positive if a2 > a1
double GetAngleDiff_deg( const double &a1, const double &a2);

/****************
*
*   Runways
*
****************/

// Given (lon/lat/elev) and an FGRunway struct, determine if the point lies on the runway
bool OnRunway(const SGGeod& pt, const FGRunwayBase* rwy);

