#!/usr/bin/perl
#
# environment.pl - Handle environment setup
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


sub set_timeofday {
    my( $fgfs ) = shift;
    my( $timeofday ) = shift;

    &send( $fgfs, "run timeofday $timeofday" );
}


sub set_env_layer {
    my( $fgfs ) = shift;
    my( $layer_type ) = shift;  # boundary or aloft
    my( $layer_num ) = shift;
    my( $wind_hdg_deg ) = shift;
    my( $wind_spd_kt ) = shift;
    my( $turb_norm ) = shift;
    my( $temp_degc ) = shift;
    my( $press_inhg ) = shift;
    my( $dew_degc ) = shift;
    my( $vis ) = shift;
    my( $elevation ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    my( $prefix ) = "/environment/config/$layer_type/entry[$layer_num]";
    $HASH{ "$prefix/wind-from-heading-deg" } = $wind_hdg_deg;
    $HASH{ "$prefix/wind-speed-kt" } = $wind_spd_kt;
    $HASH{ "$prefix/turbulence/magnitude-norm" } = $turb_norm;
    $HASH{ "$prefix/temperature-degc" } = $temp_degc;
    $HASH{ "$prefix/pressure-sea-level-inhg" } = $press_inhg;
    $HASH{ "$prefix/dewpoint-degc" } = $dew_degc;
    $HASH{ "$prefix/visibility-m" } = $vis;
    $HASH{ "$prefix/elevation-ft" } = $elevation;

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        # print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }
}


sub set_oat {
    my( $fgfs ) = shift;
    my( $oat ) = shift;

    # set the outside air temperature (simply)
    &send( $fgfs, "run set-outside-air-temp-degc $oat" );
}


sub set_pressure {
    my( $fgfs ) = shift;
    my( $pressure_inhg ) = shift;

    my( $layer_type ) = shift;  # boundary or aloft
    my( $layer_num ) = shift;
    my( $wind_hdg_deg ) = shift;
    my( $wind_spd_kt ) = shift;
    my( $turb_norm ) = shift;
    my( $temp_degc ) = shift;
    my( $press_inhg ) = shift;
    my( $dew_degc ) = shift;
    my( $vis ) = shift;
    my( $elevation ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    my( $i );

    for ( $i = 0; $i < 3; ++$i ) {
        my( $prefix ) = "/environment/config/boundary/entry[$i]";
        $HASH{ "$prefix/pressure-sea-level-inhg" } = $pressure_inhg;
    }

    for ( $i = 0; $i < 5; ++$i ) {
        my( $prefix ) = "/environment/config/aloft/entry[$i]";
        $HASH{ "$prefix/pressure-sea-level-inhg" } = $pressure_inhg;
    }

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        # print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }
}


sub set_cloud_layer {
    my( $fgfs ) = shift;
    my( $layer ) = shift;
    my( $coverage ) = shift;
    my( $elevation_ft ) = shift;
    my( $thickness_ft ) = shift;
    my( $transition_ft ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    $HASH{ "/environment/clouds/layer[$layer]/coverage" } = $coverage;
    $HASH{ "/environment/clouds/layer[$layer]/elevation-ft" } = $elevation_ft;
    $HASH{ "/environment/clouds/layer[$layer]/thickness-ft" } = $thickness_ft;
    $HASH{ "/environment/clouds/layer[$layer]/transition-ft" } = $transition_ft;

    foreach $prop ( keys(%HASH) ) {
        $value = $HASH{$prop};
        # print "setting $prop = $value\n";
        &set_prop( $fgfs, $prop, $value );
    }
}


sub reinit_environment() {
    my( $fgfs ) = shift;

    &send( $fgfs, "run reinit environment" );
}
