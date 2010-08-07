// do some test relating to the concept of "up"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <iostream>

#include <simgear/constants.h>
#include <simgear/math/sg_geodesy.hxx>

using std::cout;
using std::endl;

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
	sgGeodToGeoc( lat * SGD_DEGREES_TO_RADIANS, alt, &sl_radius, &lat_geoc );
	cout << "  geoc lat = " << lat_geoc * SGD_RADIANS_TO_DEGREES << endl;

	Point3D pgd( lon * SGD_DEGREES_TO_RADIANS, lat * SGD_DEGREES_TO_RADIANS, 0.0 );
	Point3D pc = sgGeodToCart( pgd );
	cout << "  cartesian = " << pc << endl;

	sgdMat4 GEOD_UP;
	sgdVec3 geod_up;
	sgdMakeRotMat4( GEOD_UP, lon, 0.0, -lat );
	sgdSetVec3( geod_up, GEOD_UP[0][0], GEOD_UP[0][1], GEOD_UP[0][2] );
	cout << "  geod up = " << geod_up[0] << ", " << geod_up[1] << ", "
	     << geod_up[2] << endl;

	sgdMat4 GEOC_UP;
	sgdVec3 geoc_up;
	sgdMakeRotMat4( GEOC_UP, lon, 0.0, -lat_geoc * SGD_RADIANS_TO_DEGREES );
	sgdSetVec3( geoc_up, GEOC_UP[0][0], GEOC_UP[0][1], GEOC_UP[0][2] );
	cout << "  geoc up = " << geoc_up[0] << ", " << geoc_up[1] << ", "
	     << geoc_up[2] << endl;

	double slope = geod_up[2] / geod_up[0];
	double intercept = pc.z() - slope * pc.x();
	cout << "  Z intercept (based on geodetic up) = " << intercept << endl;

	slope = geoc_up[2] / geoc_up[0];
	intercept = pc.z() - slope * pc.x();
	cout << "  Z intercept (based on geocentric up) = " << intercept << endl;

   }

    return 0;
}
