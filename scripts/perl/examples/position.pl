#!/usr/bin/perl
#
# position.pl - Handle repositioning aircraft (in air/on ground)
#
# Written by Curtis L. Olson, started January 2004
#
# Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
#
# This code is placed in the public domain by Curtis L. Olson.
# There is no warranty, etc. etc. etc.
#
# $Id$
# ----------------------------------------------------------------------------


require "telnet.pl";

use strict;

my( $airport_id ) = "KSNA";
my( $rwy_no ) = "19R";
my( $reset_sec ) = 300;

my( $server ) = "localhost";
my( $port ) = 5401;
my( $timeout ) = 5;


sub reset_in_air {
    my( $fgfs ) = shift;
    my( $aptid ) = shift;
    my( $rwy ) = shift;
    my( $offset_dist ) = shift;
    my( $glideslope_deg ) = shift;
    my( $altitude_ft ) = shift;
    my( $airspeed_kt ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    $HASH{ "/sim/presets/airport-id" } = $aptid;
    $HASH{ "/sim/presets/runway" } = $rwy;
    $HASH{ "/sim/presets/offset-distance" } = $offset_dist;
    if ( $glideslope_deg > 0 ) {
        $HASH{ "/sim/presets/glideslope-deg" } = $glideslope_deg;
        $HASH{ "/sim/presets/altitude-ft" } = "";
    } else {
        $HASH{ "/sim/presets/glideslope-deg" } = "";
        $HASH{ "/sim/presets/altitude-ft" } = $altitude_ft;
    }

    $HASH{ "/sim/presets/airspeed-kt" } = $airspeed_kt;
    $HASH{ "/sim/presets/vor-id" } = "";
    $HASH{ "/sim/presets/vor-freq" } = "";
    $HASH{ "/sim/presets/ndb-id" } = "";
    $HASH{ "/sim/presets/ndb-freq" } = "";
    $HASH{ "/sim/presets/fix" } = "";
    $HASH{ "/sim/presets/longitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/latitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/offset-azimuth" } = "";
    $HASH{ "/sim/presets/heading-deg" } = "-9999.0";

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }

    &send( $fgfs, "run presets-commit" );
}


sub reset_on_ground {
    my( $fgfs ) = shift;
    my( $aptid ) = shift;
    my( $rwy ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    $HASH{ "/sim/presets/airport-id" } = $aptid;
    $HASH{ "/sim/presets/runway" } = $rwy;
    $HASH{ "/sim/presets/offset-distance" } = "";
    $HASH{ "/sim/presets/glideslope-deg" } = "";
    $HASH{ "/sim/presets/altitude-ft" } = "";
    $HASH{ "/sim/presets/airspeed-kt" } = "";
    $HASH{ "/sim/presets/vor-id" } = "";
    $HASH{ "/sim/presets/vor-freq" } = "";
    $HASH{ "/sim/presets/ndb-id" } = "";
    $HASH{ "/sim/presets/ndb-freq" } = "";
    $HASH{ "/sim/presets/fix" } = "";
    $HASH{ "/sim/presets/longitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/latitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/offset-azimuth" } = "";
    $HASH{ "/sim/presets/heading-deg" } = "-9999.0";

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }

    &send( $fgfs, "run presets-commit" );
}



