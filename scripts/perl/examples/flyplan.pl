#!/usr/bin/perl
#
# Written by Curtis L. Olson, started January 2003
#
# This file is in the Public Domain and comes with no warranty.
#
# $Id$
# ----------------------------------------------------------------------------


use strict;

require "telnet.pl";

my( $server ) = "localhost";
my( $port ) = 5401;
my( $timeout ) = 10;

my( %Route );
$Route{0} = "OAK:116.80:020";
$Route{1} = "OAK:116.80:019:27";
$Route{2} = "SAC:115.20:020";
$Route{3} = "SAC:115.20:080:43";
$Route{4} = "ECA:116.0:209";

my( $i );

foreach $i ( keys(%Route) ) {
    &fly_to( $Route{$i} );
}


sub fly_to() {
    my( $waypoint ) = shift;

    # decode waypoint
    my( $id, $freq, $radial, $dist ) = split( /:/, $waypoint );

    print "Next way point is $id - $freq\n";
    print "  Target radial is $radial\n";
    if ( $dist ne "" ) {
        print "  Flying outbound for $dist nm\n";
    } else {
        print "  Flying inbound to station\n";
    }

    # tune radio and set autopilot
    my( $fgfs );
    if ( !( $fgfs = &connect($server, $port, $timeout) ) ) {
        print "Error: can't open socket\n";
        return;
    }
    &send( $fgfs, "data" );     # switch to raw data mode
    set_prop( $fgfs, "/radios/nav[0]/frequencies/selected-mhz", $freq );
    set_prop( $fgfs, "/radios/nav[0]/radials/selected-deg", $radial );
    set_prop( $fgfs, "/radios/dme/switch-position", "1" );
    set_prop( $fgfs, "/autopilot/locks/nav", "true" );

    # monitor progress until goal is achieved
    my( $done ) = 0;
    my( $last_range ) = 9999.0;
    while ( !$done ) {
        my( $inrange ) = get_prop( $fgfs, "/radios/nav[0]/in-range" );
        if ( $inrange eq "false" ) {
            print "Warning, VOR not in range, we are lost!\n";
        }
        my( $cur_range ) = get_prop( $fgfs, "/radios/dme/distance-nm" );
        print "  range = $cur_range\n";
        if ( $dist ne "" ) {
            # a target dist is specified so assume we are flying outbound
            if ( $cur_range > $dist ) {
                $done = 1;
            }
        } else {
            # no target dist is specified, assume we are flying
            # inbound to the station
            if ( $cur_range < 0.25 && $cur_range > 0.0 ) {
                $done = 1;
            } elsif ( $last_range < $cur_range ) {
                $done = 1;
            }
        }
        $last_range = $cur_range;

        # loop once per second
        sleep(1);
    }

    &send( $fgfs, "quit");
    close $fgfs;
}
