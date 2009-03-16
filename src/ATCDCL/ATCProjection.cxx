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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "ATCProjection.hxx"
#include <math.h>
#include <simgear/constants.h>

FGATCAlignedProjection::FGATCAlignedProjection() {
    _origin.setLatitudeRad(0);
    _origin.setLongitudeRad(0);
    _origin.setElevationM(0);
    _correction_factor = cos(_origin.getLatitudeRad());
}

FGATCAlignedProjection::FGATCAlignedProjection(const SGGeod& centre, double heading) {
    _origin = centre;
    _theta = heading * SG_DEGREES_TO_RADIANS;
    _correction_factor = cos(_origin.getLatitudeRad());
}

FGATCAlignedProjection::~FGATCAlignedProjection() {
}

void FGATCAlignedProjection::Init(const SGGeod& centre, double heading) {
    _origin = centre;
    _theta = heading * SG_DEGREES_TO_RADIANS;
    _correction_factor = cos(_origin.getLatitudeRad());
}

SGVec3d FGATCAlignedProjection::ConvertToLocal(const SGGeod& pt) {
    // convert from lat/lon to orthogonal
    double delta_lat = pt.getLatitudeRad() - _origin.getLatitudeRad();
    double delta_lon = pt.getLongitudeRad() - _origin.getLongitudeRad();
    double y = sin(delta_lat) * SG_EQUATORIAL_RADIUS_M;
    double x = sin(delta_lon) * SG_EQUATORIAL_RADIUS_M * _correction_factor;

    // Align
    if(_theta != 0.0) {
        double xbar = x;
        x = x*cos(_theta) - y*sin(_theta);
        y = (xbar*sin(_theta)) + (y*cos(_theta));
    }

    return SGVec3d(x, y, pt.getElevationM());
}

SGGeod FGATCAlignedProjection::ConvertFromLocal(const SGVec3d& pt) {
    // de-align
    double thi = _theta * -1.0;
    double x = pt.x()*cos(thi) - pt.y()*sin(thi);
    double y = (pt.x()*sin(thi)) + (pt.y()*cos(thi));

    // convert from orthogonal to lat/lon
    double delta_lat = asin(y / SG_EQUATORIAL_RADIUS_M);
    double delta_lon = asin(x / SG_EQUATORIAL_RADIUS_M) / _correction_factor;

    return SGGeod::fromRadM(_origin.getLongitudeRad()+delta_lon, _origin.getLatitudeRad()+delta_lat, pt.z());
}
