#!/usr/bin/perl

require "telnet.pl";

use strict;

my( $airport_id ) = "KSNA";
my( $rwy_no ) = "19R";
my( $reset_sec ) = 300;

my( $server ) = "localhost";
my( $port ) = 5401;
my( $timeout ) = 5;

while ( 1 ) {
    print "Reseting to $airport_id $rwy_no\n";
    reset_position( $airport_id, $rwy_no );
    sleep( $reset_sec );
}


sub reset_position {
    my( $aptid ) = shift;
    my( $rwy ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    $HASH{ "/sim/presets/airport-id" } = $aptid;
    $HASH{ "/sim/presets/runway" } = $rwy;
    $HASH{ "/sim/presets/vor-id" } = "";
    $HASH{ "/sim/presets/vor-freq" } = "";
    $HASH{ "/sim/presets/ndb-id" } = "";
    $HASH{ "/sim/presets/ndb-freq" } = "";
    $HASH{ "/sim/presets/fix" } = "";
    $HASH{ "/sim/presets/longitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/latitude-deg" } = "-9999.0";
    $HASH{ "/sim/presets/offset-distance" } = "";
    $HASH{ "/sim/presets/offset-azimuth" } = "";
    $HASH{ "/sim/presets/heading-deg" } = "-9999.0";
    $HASH{ "/sim/presets/altitude-ft" } = "";
    $HASH{ "/sim/presets/glideslope-deg" } = "";
    $HASH{ "/sim/presets/airspeed-kt" } = "";

    my( $fgfs );

    if ( !( $fgfs = &connect($server, $port, $timeout) ) ) {
        print "Error: can't open socket\n";
        return;
    }

    &send( $fgfs, "data" );     # switch to raw data mode

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        # if ( $value eq "" ) {
        #    $value = 0;
        # }
        print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }

    &send( $fgfs, "run presets-commit" );

    # set time of day to noon
    &send( $fgfs, "run timeofday noon" );

    # start the engine
    &set_prop( $fgfs, "/controls/engines/engine[0]/magnetos", "3" );
    &set_prop( $fgfs, "/controls/engines/engine[0]/starter", "true" );
    sleep(2);
    &set_prop( $fgfs, "/controls/engines/engine[0]/starter", "false" );

    &send( $fgfs, "quit" );
    close $fgfs;
}



