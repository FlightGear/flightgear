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

# iterate through the requested coordinates
while ( <> ) {
    my( $lon, $lat ) = split;
    set_prop( $fgfs, "/position/longitude-deg", $lon );
    set_prop( $fgfs, "/position/latitude-deg", $lat );

    # wait 1 second for scenery to load
    usleep(500000);

    # then fetch ground elevation
    my( $elev ) = get_prop( $fgfs, "/position/ground-elev-m" );

    print "$lon $lat $elev\n";
}


# shutdown our connection (this leaves FG running)
&send( $fgfs, "quit");
close $fgfs;
