#include <Include/fg_constants.h>
#include <Math/fg_geodesy.h>

#include <stdio.h>

void
main( void )
{
    double Lon, Alt, sl_radius;
    double geodetic_Lat;
    double geocentric_Lat;

    Lon          = -87.75 * DEG_TO_RAD;
    geodetic_Lat =  41.83 * DEG_TO_RAD;
    Alt          = 1.5; /* km */

    printf("Geodetic position = (%.8f, %.8f, %.8f)\n", Lon, geodetic_Lat, Alt); 

    fgGeodToGeoc( geodetic_Lat, Alt, &sl_radius, &geocentric_Lat );

    printf("Geocentric position = (%.8f, %.8f, %.8f)\n", Lon, geocentric_Lat, 
	   sl_radius + Alt); 
    printf("new sl_radius = %.8f\n", sl_radius); 

    fgGeocToGeod( geocentric_Lat, sl_radius + Alt, &geodetic_Lat,
		     &Alt, &sl_radius );

    printf("Geodetic position = (%.8f, %.8f, %.8f)\n", Lon, geodetic_Lat, Alt); 
    printf("new sl_radius = %.8f\n", sl_radius); 
}
