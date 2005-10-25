// ATCProjection.cxx - A convienience projection class for the ATC/AI system.
//
// Written by David Luff, started 2002.
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

#include "ATCProjection.hxx"
#include <math.h>
#include <simgear/constants.h>

#include <iostream>
SG_USING_STD(cout);

#define DCL_PI  3.1415926535f
//#define SG_PI  ((SGfloat) M_PI)
#define DCL_DEGREES_TO_RADIANS  (DCL_PI/180.0)
#define DCL_RADIANS_TO_DEGREES  (180.0/DCL_PI)

FGATCProjection::FGATCProjection() {
    origin.setlat(0.0);
    origin.setlon(0.0);
    origin.setelev(0.0);
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

FGATCProjection::FGATCProjection(const Point3D& centre) {
    origin = centre;
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

FGATCProjection::~FGATCProjection() {
}

void FGATCProjection::Init(const Point3D& centre) {
    origin = centre;
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

Point3D FGATCProjection::ConvertToLocal(const Point3D& pt) {
    double delta_lat = pt.lat() - origin.lat();
    double delta_lon = pt.lon() - origin.lon();

    double y = sin(delta_lat * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M * correction_factor;

    return(Point3D(x,y,0.0));
}

Point3D FGATCProjection::ConvertFromLocal(const Point3D& pt) {
	double delta_lat = asin(pt.y() / SG_EQUATORIAL_RADIUS_M) * DCL_RADIANS_TO_DEGREES;
	double delta_lon = (asin(pt.x() / SG_EQUATORIAL_RADIUS_M) * DCL_RADIANS_TO_DEGREES) / correction_factor;
	
    return(Point3D(origin.lon()+delta_lon, origin.lat()+delta_lat, 0.0));
}

/**********************************************************************************/

FGATCAlignedProjection::FGATCAlignedProjection() {
    origin.setlat(0.0);
    origin.setlon(0.0);
    origin.setelev(0.0);
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

FGATCAlignedProjection::~FGATCAlignedProjection() {
}

void FGATCAlignedProjection::Init(const Point3D& centre, double heading) {
    origin = centre;
    theta = heading * DCL_DEGREES_TO_RADIANS;
    correction_factor = cos(origin.lat() * DCL_DEGREES_TO_RADIANS);
}

Point3D FGATCAlignedProjection::ConvertToLocal(const Point3D& pt) {
    // convert from lat/lon to orthogonal
    double delta_lat = pt.lat() - origin.lat();
    double delta_lon = pt.lon() - origin.lon();
    double y = sin(delta_lat * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon * DCL_DEGREES_TO_RADIANS) * SG_EQUATORIAL_RADIUS_M * correction_factor;

    // Align
    double xbar = x;
    x = x*cos(theta) - y*sin(theta);
    y = (xbar*sin(theta)) + (y*cos(theta));

    return(Point3D(x,y,pt.elev()));
}

Point3D FGATCAlignedProjection::ConvertFromLocal(const Point3D& pt) {
	//cout << "theta = " << theta << '\n';
	//cout << "origin = " << origin << '\n';
    // de-align
    double thi = theta * -1.0;
    double x = pt.x()*cos(thi) - pt.y()*sin(thi);
    double y = (pt.x()*sin(thi)) + (pt.y()*cos(thi));

    // convert from orthogonal to lat/lon
    double delta_lat = asin(y / SG_EQUATORIAL_RADIUS_M) * DCL_RADIANS_TO_DEGREES;
    double delta_lon = (asin(x / SG_EQUATORIAL_RADIUS_M) * DCL_RADIANS_TO_DEGREES) / correction_factor;

    return(Point3D(origin.lon()+delta_lon, origin.lat()+delta_lat, pt.elev()));
}
