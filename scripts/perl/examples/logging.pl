#!/usr/bin/perl
#
# logging.pl - Handle logging
#
# Written by Curtis L. Olson, started February 2004
#
# Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
#
# This code is placed in the public domain by Curtis L. Olson.
# There is no warranty, etc. etc. etc.
#
# $Id$
# ----------------------------------------------------------------------------


use strict;

require "telnet.pl";

my( $lognum ) = 1;

my( @FIELDS, @FIELDS_E );
my( $tmp_dir ) = "/tmp";

my( $field_index ) = 0;


sub clear_logging {
    my( $fgfs ) = shift;

    my( $done ) = 0;
    my( $i ) = 0;
    my( $prop );
    while ( !$done ) {
        $prop = &get_prop( $fgfs, "/logging/log[$lognum]/entry[$i]/property" );
        if ( $prop ne "" ) {
            &set_prop( $fgfs,
                       "/logging/log[$lognum]/entry[$i]/enabled",
                       "false" );
        } else {
            $done = 1;
        }
        $i++;
    }

    $field_index = 0;
}


sub add_field {
    my( $fgfs ) = shift;
    my( $title ) = shift;
    my( $prop ) = shift;

    # spaces seem to not work well.
    $title =~ s/ /\_/g;

    # print "$title - $prop\n";
    &set_prop( $fgfs,
               "/logging/log[$lognum]/entry[$field_index]/title", $title );
    &set_prop( $fgfs,
               "/logging/log[$lognum]/entry[$field_index]/property", $prop );
    &set_prop( $fgfs,
               "/logging/log[$lognum]/entry[$field_index]/enabled", "true" );

    $field_index++;
}


sub add_default_fields() {
    my( $fgfs ) = shift;

    push( @FIELDS, ( "Longitude (deg)", "/position/longitude-deg" ) );
    push( @FIELDS, ( "Latitude (deg)", "/position/latitude-deg" ) );
    push( @FIELDS, ( "Altitude (ft MSL)", "/position/altitude-ft" ) );
    push( @FIELDS, ( "Altitude (ft AGL)", "/position/altitude-agl-ft" ) );
    push( @FIELDS, ( "Roll (deg)", "/orientation/roll-deg" ) );
    push( @FIELDS, ( "Pitch (deg)", "/orientation/pitch-deg" ) );
    push( @FIELDS, ( "Heading (deg)", "/orientation/heading-deg" ) );
    push( @FIELDS, ( "Calibrated Air Speed (kt)", "/velocities/airspeed-kt" ) );
    push( @FIELDS, ( "Vertical Speed (fps)",
                     "/velocities/vertical-speed-fps" ) );
    push( @FIELDS, ( "Roll Rate (degps)", "/orientation/roll-rate-degps" ) );
    push( @FIELDS, ( "Pitch Rate (degps)", "/orientation/pitch-rate-degps" ) );
    push( @FIELDS, ( "Yaw Rate (degps)", "/orientation/yaw-rate-degps" ) );
    push( @FIELDS, ( "Alpha (deg)", "/orientation/alpha-deg" ) );
    push( @FIELDS, ( "Beta (deg)", "/orientation/side-slip-deg" ) );
    push( @FIELDS, ( "Prop (RPM)", "/engines/engine[0]/rpm" ) );
    push( @FIELDS, ( "Left Aileron Pos (norm)",
                     "/surface-positions/left-aileron-pos-norm" ) );
    push( @FIELDS, ( "Right Aileron Pos (norm)",
                     "/surface-positions/right-aileron-pos-norm" ) );
    push( @FIELDS, ( "Elevator Pos (norm)",
                     "/surface-positions/elevator-pos-norm" ) );
    push( @FIELDS, ( "Elevator Trim Tab Pos (norm)",
                     "/surface-positions/elevator-trim-tab-pos-norm" ) );
    push( @FIELDS, ( "Rudder Pos (norm)",
                     "/surface-positions/rudder-pos-norm" ) );
    push( @FIELDS, ( "Flap Pos (norm)",
                     "/surface-positions/flap-pos-norm" ) );
    push( @FIELDS, ( "Nose Wheel Pos (norm)",
                     "/surface-positions/nose-wheel-pos-norm" ) );
    push( @FIELDS, ( "Wheel Pos (norm)", "/controls/flight/aileron" ) );
    push( @FIELDS, ( "Column Pos (norm)", "/controls/flight/elevator" ) );
    push( @FIELDS, ( "Trim Wheel Pos (norm)",
                     "/controls/flight/elevator-trim" ) );
    push( @FIELDS, ( "Pedal Pos (norm)", "/controls/flight/rudder" ) );
    push( @FIELDS, ( "Throttle Pos (norm)",
                     "/controls/engines/engine[0]/throttle" ) );

    # initially enable all fields
    for ( my($i) = 0; $i <= ($#FIELDS / 2); ++$i ) {
        &add_field( $fgfs, $FIELDS[2*$i], $FIELDS[2*$i+1] );
    }
}


sub start_logging {
    my( $fgfs ) = shift;
    my( $log_file ) = shift;

    &set_prop( $fgfs, "/logging/log[$lognum]/filename", $log_file );
    &set_prop( $fgfs, "/logging/log[$lognum]/interval-ms", "100" );
    &set_prop( $fgfs, "/logging/log[$lognum]/enabled", "true" );
    &send( $fgfs, "run data-logging-commit" );
}


sub stop_logging {
    my( $fgfs ) = shift;

    &set_prop( $fgfs, "/logging/log[$lognum]/enabled", "false" );
    &send( $fgfs, "run data-logging-commit" );
}


sub quick_plot_vs_time {
    my( $data_file ) = shift;
    my( $plot_file ) = shift;
    my( $title ) = shift;
    my( $column ) = shift;

    print "quick plot -> $plot_file\n";

    my( $tmpcmd ) = "$tmp_dir/plot_cmd_tmp.$$";
    my( $tmpdata ) = "$tmp_dir/plot_data_tmp.$$";
    my( $png_image ) = "$plot_file.png";

    # strip the leading header off the file so gnuplot doesn't squawk
    system( "tail -n +2 $data_file | sed -e \"s/,/ /g\" > $tmpdata" );

    # create the gnuplot command file
    open( CMD, ">$tmpcmd" );
    print CMD "set terminal png\n";
    print "png_image = $png_image\n";
    print CMD "set output \"$png_image\"\n";
    print CMD "set xlabel \"Time (sec)\"\n";
    print CMD "set ylabel \"$title\"\n";
    print CMD "plot \"$tmpdata\" using 1:$column title \"$title\" with lines\n";
    print CMD "quit\n";
    close( CMD );

    # plot the graph
    system( "gnuplot $tmpcmd" );

    # clean up all our droppings
    unlink( $tmpcmd );
    unlink( $tmpdata );
}


sub quick_plot {
    my( $data_file ) = shift;
    my( $plot_file ) = shift;
    my( $xtitle ) = shift;
    my( $ytitle ) = shift;
    my( $xcolumn ) = shift;
    my( $ycolumn ) = shift;

    print "quick plot -> $plot_file\n";

    my( $tmpcmd ) = "$tmp_dir/plot_cmd_tmp.$$";
    my( $tmpdata ) = "$tmp_dir/plot_data_tmp.$$";
    my( $png_image ) = "$plot_file.png";

    # strip the leading header off the file so gnuplot doesn't squawk
    system( "tail -n +2 $data_file | sed -e \"s/,/ /g\" > $tmpdata" );

    # create the gnuplot command file
    open( CMD, ">$tmpcmd" );
    print CMD "set terminal png\n";
    print "png_image = $png_image\n";
    print CMD "set output \"$png_image\"\n";
    print CMD "set xlabel \"$xtitle\"\n";
    print CMD "set ylabel \"$ytitle\"\n";
    print CMD "plot \"$tmpdata\" using $xcolumn:$ycolumn title \"$xtitle vs. $ytitle\" with lines\n";
    print CMD "quit\n";
    close( CMD );

    # plot the graph
    system( "gnuplot $tmpcmd" );

    # clean up all our droppings
    unlink( $tmpcmd );
    unlink( $tmpdata );
}


return 1;                    # make perl happy
