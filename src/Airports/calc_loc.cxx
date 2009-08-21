// Calculate ILS heading

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

#include <simgear/math/sg_geodesy.hxx>


using std::string;
using std::cout;
using std::endl;

int main( int argc, char **argv ) {

    if ( argc != 9 ) {
        cout << "Wrong usage" << endl;
    }

    double loc_lat = atof( argv[1] );
    double loc_lon = atof( argv[2] );
    string dir = argv[3];
    double rwy_lat = atof( argv[4] );
    double rwy_lon = atof( argv[5] );
    double rwy_hdg = atof( argv[6] );
    double rwy_len = atof( argv[7] ) * SG_DEGREES_TO_RADIANS;
    // double rwy_wid = atof( argv[8] );

    if ( dir == "FOR" ) {
        rwy_hdg += 180.0;
        if ( rwy_hdg > 360.0 ) {
            rwy_hdg -= 360.0;
        }
    }

    // calculate runway threshold point
    double thresh_lat = 0.0, thresh_lon = 0.0, return_az = 0.0;
    geo_direct_wgs_84 ( 0.0, rwy_lat, rwy_lon, rwy_hdg, 
                        rwy_len / 2.0, &thresh_lat, &thresh_lon, &return_az );
    cout << "Threshold = " << thresh_lat << "," << thresh_lon << endl;

    // calculate distance from threshold to localizer
    double az1, az2, dist_m;
    geo_inverse_wgs_84( 0.0, loc_lat, loc_lon, thresh_lat, thresh_lon,
                        &az1, &az2, &dist_m );
    cout << "Distance = " << dist_m << endl;

    // back project that distance along the runway center line
    double nloc_lat = 0.0, nloc_lon = 0.0;
    geo_direct_wgs_84 ( 0.0, thresh_lat, thresh_lon, rwy_hdg + 180.0, 
                        dist_m, &nloc_lat, &nloc_lon, &return_az );
    printf("New localizer = %.6f %.6f\n", nloc_lat, nloc_lon );

    return 0;
}
