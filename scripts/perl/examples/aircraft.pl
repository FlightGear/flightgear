#!/usr/bin/perl
#
# aircraft.pl - Handle aircraft functions
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

sub start_engine {
    my( $fgfs ) = shift;
    my( $engine_num ) = shift;

    my( $prop, $value );
    my( %HASH ) = ();

    &set_prop( $fgfs, "/controls/engines/engine[$engine_num]/magnetos", "3" );
    &set_prop( $fgfs, "/controls/engines/engine[$engine_num]/starter", "true" );
    sleep(3);
    &set_prop( $fgfs, "/controls/engines/engine[$engine_num]/starter",
               "false" );
}


sub set_throttle {
    my( $fgfs ) = shift;
    my( $engine ) = shift;
    my( $throttle_norm ) = shift;

    &set_prop( $fgfs, "/controls/engines/engine[$engine]/throttle",
               $throttle_norm );
}


sub set_mixture {
    my( $fgfs ) = shift;
    my( $engine ) = shift;
    my( $mix_norm ) = shift;

    &set_prop( $fgfs, "/controls/engines/engine[$engine]/mixture", $mix_norm );
}


sub set_weight {
    my( $fgfs ) = shift;
    my( $lbs ) = shift;

    &set_prop( $fgfs, "/sim/aircraft-weight-lbs", $lbs );
}


sub set_cg {
    my( $fgfs ) = shift;
    my( $inches ) = shift;

    &set_prop( $fgfs, "/sim/aircraft-cg-offset-inches", $inches );
}


sub set_parking_brake {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/gear/brake-parking", $pos_norm );
}

sub set_flaps {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/flight/flaps", $pos_norm );
}

sub set_aileron {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/flight/aileron", $pos_norm );
}

sub set_elevator {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/flight/elevator", $pos_norm );
}

sub set_elevator_trim {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/flight/elevator-trim", $pos_norm );
}

sub set_rudder {
    my( $fgfs ) = shift;
    my( $pos_norm ) = shift;

    &set_prop( $fgfs, "/controls/flight/rudder", $pos_norm );
}
