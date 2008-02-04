#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h> // exit()

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/sg_geodesy.hxx>


int make_model( const char *texture,
                double ll_lon, double ll_lat,
                double lr_lon, double lr_lat,
                double ur_lon, double ur_lat,
                double ul_lon, double ul_lat,
                const char *model )
{
    // open the output file
    FILE *out = fopen( model, "w" );
    if ( out == NULL ) {
        printf("Error: cannot open %s for writing\n", model);
        return 0;
    }

    // compute polygon boundaries in local XY projection
    double cen_lon = (ll_lon + lr_lon + ur_lon + ul_lon) / 4.0;
    double cen_lat = (ll_lat + lr_lat + ur_lat + ul_lat) / 4.0;

    double az1, az2, dist;
    double angle;

    geo_inverse_wgs_84( cen_lat, cen_lon, ll_lat, ll_lon, &az1, &az2, &dist );
    angle = az1 * SGD_DEGREES_TO_RADIANS;
    double llx = sin(angle) * dist;
    double lly = cos(angle) * dist;
    printf( "az1 = %.3f dx = %.3f dy = %.3f\n",
            az1, sin(angle) * dist, cos(angle) * dist );
    
    geo_inverse_wgs_84( cen_lat, cen_lon, lr_lat, lr_lon, &az1, &az2, &dist );
    angle = az1 * SGD_DEGREES_TO_RADIANS;
    double lrx = sin(angle) * dist;
    double lry = cos(angle) * dist;
    printf( "az1 = %.3f dx = %.3f dy = %.3f\n",
            az1, sin(angle) * dist, cos(angle) * dist );

    geo_inverse_wgs_84( cen_lat, cen_lon, ur_lat, ur_lon, &az1, &az2, &dist );
    angle = az1 * SGD_DEGREES_TO_RADIANS;
    double urx = sin(angle) * dist;
    double ury = cos(angle) * dist;
    printf( "az1 = %.3f dx = %.3f dy = %.3f\n",
            az1, sin(angle) * dist, cos(angle) * dist );

    geo_inverse_wgs_84( cen_lat, cen_lon, ul_lat, ul_lon, &az1, &az2, &dist );
    angle = az1 * SGD_DEGREES_TO_RADIANS;
    double ulx = sin(angle) * dist;
    double uly = cos(angle) * dist;
    printf( "az1 = %.3f dx = %.3f dy = %.3f\n",
            az1, sin(angle) * dist, cos(angle) * dist );

    fprintf( out, "AC3Db\n" );
    fprintf( out, "MATERIAL \"DefaultWhite\" rgb 1 1 1  amb 1 1 1  emis 0 0 0  spec 0.5 0.5 0.5  shi 64  trans 0\n" );
    fprintf( out, "OBJECT world\n" );
    fprintf( out, "kids 1\n" );
    fprintf( out, "OBJECT poly\n" );
    fprintf( out, "name \"terrain\"\n" );
    fprintf( out, "data 9\n" );
    fprintf( out, "Plane.073\n" );
    fprintf( out, "texture \"%s\"\n", texture );
    fprintf( out, "texrep 1 1\n" );
    fprintf( out, "crease 30.000000\n" );
    fprintf( out, "numvert 4\n" );
    fprintf( out, "%.3f 0 %.3f\n", lry, lrx );
    fprintf( out, "%.3f 0 %.3f\n", lly, llx );
    fprintf( out, "%.3f 0 %.3f\n", uly, ulx );
    fprintf( out, "%.3f 0 %.3f\n", ury, urx );
    fprintf( out, "numsurf 1\n" );
    fprintf( out, "SURF 0x00\n" );
    fprintf( out, "mat 0\n" );
    fprintf( out, "refs 4\n" );
    fprintf( out, "0 0.0 1.0\n" );
    fprintf( out, "3 0.0 0.0\n" );
    fprintf( out, "2 1.0 0.0\n" );
    fprintf( out, "1 1.0 1.0\n" );
    fprintf( out, "kids 0\n" );

    fclose( out );

    SGBucket b( cen_lon, cen_lat );

    printf("\n");
    printf("Model is created as %s\n", model);
    printf("\n");
    printf("Now copy the .ac and .rgb file to your Model directory\n");
    printf("and add the following line to your .stg file:\n");
    printf("\n");
    printf("    %s/%s.stg\n",
           b.gen_base_path().c_str(), b.gen_index_str().c_str());
    printf("\n");
    printf("OBJECT_SHARED Models/MNUAV/%s %.6f %.6f ALT_IN_METERS 0.00\n",
           model, cen_lon, cen_lat);
    return 1;
}


void usage( char *prog ) {
    printf("Usage: %s photo.rgb ll_lon ll_lat lr_lon lr_lat ur_lon ur_lat ul_lon ul_lat model.ac\n", prog);
    printf("Usage: %s photo.rgb left_lon right_lon lower_lat upper_lat model.ac\n", prog);

    exit( 0 );
}


int main( int argc, char **argv ) {
    if ( argc == 11 ) {
        make_model( argv[1],
                    atof(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]),
                    atof(argv[6]), atof(argv[7]), atof(argv[8]), atof(argv[9]),
                    argv[10] );
    } else if ( argc == 7 ) {
        make_model( argv[1],
                    atof(argv[2]), atof(argv[4]), atof(argv[3]), atof(argv[4]),
                    atof(argv[3]), atof(argv[5]), atof(argv[2]), atof(argv[5]),
                    argv[6] );
    } else {
        usage( argv[0] );
    }
 
    return 0;
}
