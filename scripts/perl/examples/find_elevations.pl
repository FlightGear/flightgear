#!/usr/bin/perl
#
# Written by Curtis L. Olson, started January 2003
#
# This file is in the Public Domain and comes with no warranty.
#
# $Id$
# ----------------------------------------------------------------------------


# This script will calculate the flightgear ground elevation for a
# serious of lon/lat pairs, given one per line via stdin.  Result it
# written to stdout.  Lon/lat must be specified in decimal degrees,
# i.e. "-110.2324 39.872"
#
# This requires a copy of flightgear running with "--fdm=null" on the
# specified "$server" host name, at the specified "$port".
#
# I highly recommend that you if you plan to feed a large number of
# coordinates through this script that you presort your list by tile id #
# That will minimize the load on the FG tile pager since you will process
# all coordinates for a particular tile before moving on to the next.
# Also, there is a chance the next tile will already be loaded if it is near
# the previous (which it will tend to be if you sort by tile id.)

use strict;

use Time::HiRes qw( usleep );

require "telnet.pl";

my( $server ) = "localhost";
my( $port ) = 5401;
my( $timeout ) = 10;


# open the connection to the running copy of flightgear
my( $fgfs );
if ( !( $fgfs = &connect($server, $port, $timeout) ) ) {
    die "Error: can't open socket\n";
}
&send( $fgfs, "data" );     # switch to raw data mode


# elevate ourselves only to make the view more interesting, this
# doesn't affect the results
set_prop( $fgfs, "/position/altitude-ft", "5000" );

my( $last_lon ) = -1000.0;
my( $last_lat ) = -1000.0;
my( $last_elev ) = -1000.0;

# iterate through the requested coordinates
while ( <> ) {
    my( $lon, $lat ) = split;
    set_prop( $fgfs, "/position/longitude-deg", $lon );
    set_prop( $fgfs, "/position/latitude-deg", $lat );

    # wait 1 second for scenery to load
    usleep(500000);

    # then fetch ground elevation
    my( $elev ) = get_prop( $fgfs, "/position/ground-elev-m" );

    if ( $lon != $last_lon || $lat != $last_lat ) {
        my($waitcount) = 0;
        while ( $elev == $last_elev && $waitcount < 5 ) {
            print "(WARNING: waiting an addition 1 second and requerying.)\n";
            # same answer as last time, scenery is probably still loading,
            # let's wait 1 more seconds and hope we get it right the next
            # time, we bail after 5 seconds.
            usleep(1000000);
            $elev = get_prop( $fgfs, "/position/ground-elev-m" );
            $waitcount++;
        }
    }

    print "$lon $lat $elev\n";

    $last_elev = $elev;
    $last_lon = $lon;
    $last_lat = $lat;
}


# shutdown our connection (this leaves FG running)
&send( $fgfs, "quit");
close $fgfs;
