// Utility functions for the ATC / AI system

#include <math.h>
#include <simgear/math/point3d.hxx>
#include <simgear/constants.h>
#include <plib/sg.h>

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

// Given a position (lat/lon/elev), heading, vertical angle, and distance, calculate the new position.
// Assumes that the ground is not hit!!!  Expects heading and angle in degrees, distance in meters.
Point3D dclUpdatePosition(Point3D pos, double heading, double angle, double distance) {
    double lat = pos.lat() * SG_DEGREES_TO_RADIANS;
    double lon = pos.lon() * SG_DEGREES_TO_RADIANS;
    double elev = pos.elev();

    double horiz_dist = distance * cos(angle);
    double vert_dist = distance * sin(angle);

    double north_dist = horiz_dist * cos(heading);
    double east_dist = horiz_dist * sin(heading);

    lat += asin(north_dist / SG_EQUATORIAL_RADIUS_M);
    lon += asin(east_dist / SG_EQUATORIAL_RADIUS_M) * (1.0 / cos(lat));  // I suppose really we should use the average of the original and new lat but we'll assume that this will be good enough.
    elev += vert_dist;

    return(Point3D(lon*SG_RADIANS_TO_DEGREES, lat*SG_RADIANS_TO_DEGREES, elev));
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