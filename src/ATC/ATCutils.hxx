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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <math.h>
#include <simgear/math/point3d.hxx>
#include <string>
SG_USING_STD(string);

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
string ConvertNumToSpokenDigits(string n);

// Convert a 2 digit rwy number to a spoken-style string
string ConvertRwyNumToSpokenString(int n);

// Return the phonetic letter of a letter represented as an integer 1->26
string GetPhoneticIdent(int i);


/*******************************
*
*  Positional functions
*
********************************/

// Given two positions, get the HORIZONTAL separation (in meters)
double dclGetHorizontalSeparation(Point3D pos1, Point3D pos2);

// Given a point and a line, get the HORIZONTAL shortest distance from the point to a point on the line.
// Expects to be fed orthogonal co-ordinates, NOT lat & lon !
double dclGetLinePointSeparation(double px, double py, double x1, double y1, double x2, double y2);

// Given a position (lat/lon/elev), heading, vertical angle, and distance, calculate the new position.
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters.
Point3D dclUpdatePosition(Point3D pos, double heading, double angle, double distance);

// Get a heading from one lat/lon to another (in degrees)
double GetHeadingFromTo(Point3D A, Point3D B);

// Given a heading (in degrees), bound it from 0 -> 360
void dclBoundHeading(double &hdg);
