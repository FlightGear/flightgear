// do some test relating to the concept of "up"

#include <iostream>

#include <plib/sg.h>

#include <simgear/constants.h>
#include <simgear/math/sg_geodesy.hxx>

int main() {
    // for each lat/lon given in goedetic coordinates, calculate
    // geocentric coordinates, cartesian coordinates, the local "up"
    // vector (based on original geodetic lat/lon), as well as the "Z"
    // intercept (for which 0 = center of earth)


    double lon = 0;
    double alt = 0;

    for ( double lat = 0; lat <= 90; lat += 5.0 ) {
	cout << "lon = " << lon << "  geod lat = " << lat;

	double sl_radius, lat_geoc;
	sgGeodToGeoc( lat * DEG_TO_RAD, alt, &sl_radius, &lat_geoc );
	cout << "  geoc lat = " << lat_geoc * RAD_TO_DEG << endl;

	Point3D pgd( lon * DEG_TO_RAD, lat * DEG_TO_RAD, 0.0 );
	Point3D pc = sgGeodToCart( pgd );
	cout << "  cartesian = " << pc << endl;

	sgMat4 UP;
	sgVec3 geod_up;
	sgMakeRotMat4( UP, lon, 0.0, -lat );
	sgSetVec3( geod_up, UP[0][0], UP[0][1], UP[0][2] );
	cout << "  geod up = " << geod_up[0] << ", " << geod_up[1] << ", "
	     << geod_up[2] << endl;

	double slope = geod_up[2] / geod_up[0];
	double intercept = pc.z() - slope * pc.x();
	cout << "  Z intercept (based on geodetic up) = " << intercept << endl;

	slope = pc.z() / pc.x();
	intercept = pc.z() - slope * pc.x();
	cout << "  Z intercept (based on geocentric up) = " << intercept << endl;

   }

    return 0;
}
