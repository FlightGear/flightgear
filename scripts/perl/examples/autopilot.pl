#!/usr/bin/perl
#
# autopilot.pl - Handle autopilot functions
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


sub autopilot_off {
    my( $fgfs ) = shift;

    &set_prop( $fgfs, "/autopilot/locks/heading", "" );
    &set_prop( $fgfs, "/autopilot/locks/altitude", "" );
    &set_prop( $fgfs, "/autopilot/locks/speed", "" );
}


sub wing_leveler {
    my( $fgfs ) = shift;
    my( $state ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/heading", "wing-leveler" );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/heading", "" );
    }
}


sub bank_hold {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $bank_deg ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/heading", "bank-hold" );
        &set_prop( $fgfs, "/autopilot/settings/target-bank-deg", $bank_deg );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/heading", "" );
    }
}


sub heading_hold {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $hdg_deg ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/heading", "dg-heading-hold" );
        &set_prop( $fgfs, "/autopilot/settings/heading-bug-deg", $hdg_deg );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/heading", "" );
    }
}


sub pitch_hold_trim {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $pitch_deg ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "pitch-hold" );
        &set_prop( $fgfs, "/autopilot/settings/target-pitch-deg", $pitch_deg );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "" );
    }
}


sub pitch_hold_yoke {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $pitch_deg ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "pitch-hold-yoke" );
        &set_prop( $fgfs, "/autopilot/settings/target-pitch-deg", $pitch_deg );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "" );
    }
}


sub altitude_hold {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $alt_ft ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "altitude-hold" );
        &set_prop( $fgfs, "/autopilot/settings/target-altitude-ft", $alt_ft );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/altitude", "" );
    }
}


sub auto_speed_throttle {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $kts ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/speed", "speed-with-throttle" );
        &set_prop( $fgfs, "/autopilot/settings/target-speed-kt", $kts );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/speed", "" );
    }
}


sub auto_speed_pitch_trim {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $kts ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/speed", "speed-with-pitch-trim" );
        &set_prop( $fgfs, "/autopilot/settings/target-speed-kt", $kts );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/speed", "" );
    }
}

sub auto_speed_pitch_yoke {
    my( $fgfs ) = shift;
    my( $state ) = shift;
    my( $kts ) = shift;

    if ( $state ) {
        &set_prop( $fgfs, "/autopilot/locks/speed", "speed-with-pitch-yoke" );
        &set_prop( $fgfs, "/autopilot/settings/target-speed-kt", $kts );
    } else {
        &set_prop( $fgfs, "/autopilot/locks/speed", "" );
    }
}




