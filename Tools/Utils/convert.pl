#!/usr/local/bin/perl

# do some star data file conversion

$PI = 3.14159265358979323846;
$DEG_TO_RAD = $PI / 180.0;

while ( <> ) {
    chop;
    ($name, $junk, $ra, $decl, $mag, $junk) = split(/,/);

    @RA = split(/:/, $ra);
    @DECL = split(/:/, $decl);

    $new_ra = $RA[0] + ( $RA[1] / 60.0 ) + ( $RA[2] / 3600.0 );
    $new_decl = $DECL[0] + ( $DECL[1] / 60.0 ) + ( $DECL[2] / 3600.0 );

    printf("%s,%.6f,%.6f,%.6f\n", $name, $new_ra * 15 * $DEG_TO_RAD, 
	   $new_decl * $DEG_TO_RAD, $mag);
}
