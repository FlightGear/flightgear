#!/usr/bin/perl

########################################################################
# Convert DAFIFT ARPT/ILS.TXT to FlightGear format.
########################################################################

use strict;

my($faa_ils_file) = shift(@ARGV);
my($dafift_arpt_file) = shift(@ARGV);
my($dafift_ils_file) = shift(@ARGV);
my($fgfs_ils_file) = shift(@ARGV);
my($fgfs_apt_file) = shift(@ARGV);
my($output_file) = shift(@ARGV);

my($calc_loc)
    = "/home/curt/projects/FlightGear-0.9/source/src/Airports/calc_loc";

die "Usage: $0 " .
    "<faa_ils_file> <dafift_arpt_file> <dafift_ils_file> <xplane_ils_file> <fgfs_apt_file> <output_file>\n"
    if !defined($faa_ils_file) || !defined($dafift_arpt_file)
       || !defined($dafift_ils_file) || !defined($fgfs_ils_file)
       || !defined($fgfs_apt_file) || !defined($output_file);

my( %CODES );
my( %CodesByICAO );
my( %Elevations );
my( %ILS );
my( %AIRPORTS );
my( %RUNWAYS );


&load_dafift( $dafift_arpt_file, $dafift_ils_file );
&load_faa( $faa_ils_file );
&load_fgfs( $fgfs_ils_file );
&load_fgfs_airports( $fgfs_apt_file );
&fix_localizer();
&fudge_missing_gs_elev();

&write_result( $output_file );

exit;


########################################################################
# Process DAFIFT data
########################################################################

sub load_dafift() {
    my( $arpt_file ) = shift;
    my( $ils_file ) = shift;

    my( $record );

    my( $id, $rwy, $type );
    my( $has_dme, $has_gs, $has_loc, $has_im, $has_mm, $has_om );
    my( $dme_lon, $dme_lat, $dme_elev, $dme_bias );
    my( $gs_lon, $gs_lat, $gs_elev, $gs_angle );
    my( $loc_type, $loc_lon, $loc_lat, $loc_elev, $loc_freq, $loc_hdg,
        $loc_width, $loc_id );
    my( $im_lon, $im_lat, $mm_lon, $mm_lat, $om_lon, $om_lat );

    # load airport file so we can lookup ICAO from internal ID

    open( ARPT, "<$arpt_file" ) || die "Cannot open DAFIFT: $arpt_file\n";

    <ARPT>;                          # skip header line

    while ( <ARPT> ) {
        chomp;
        my(@F) = split(/\t/);
        my($icao) = $F[3];
        if ( length($icao) < 3 ) {
            if ( length( $F[4] ) >= 3 ) {
                $icao = $F[4];
            } else {
                $icao = "[none]";
            }
        }
        $CODES{$F[0]} = $icao;
        $CodesByICAO{$icao} = 1;
        # print "$F[0] - $icao\n";
        $Elevations{$icao} = $F[11];
        # print "$icao $F[11]\n";
    }

    # Load the DAFIFT ils file

    my( $last_id, $last_rwy ) = ("", "");

    open( DAFIFT_ILS, "<$ils_file" ) || die "Cannot open DAFIFT: $ils_file\n";

    <DAFIFT_ILS>;                   # skip header line

    while ( <DAFIFT_ILS> ) {
        chomp;
        my @F = split(/\t/);
        $id = $F[0];
        $rwy = $F[1];

        if ( $last_id ne "" && ($last_id ne $id || $last_rwy ne $rwy) ) {
            # just hist the start of the next record, dump the current data

            if ( ! $has_gs ) {
                ( $gs_elev, $gs_angle, $gs_lat, $gs_lon ) = ( 0, 0, 0, 0 );
            }
            if ( ! $has_dme ) {
                ( $dme_lat, $dme_lon ) = ( 0, 0 );
            }
            if ( ! $has_om ) {
                ( $om_lat, $om_lon ) = ( 0, 0 );
            }
            if ( ! $has_mm ) {
                ( $mm_lat, $mm_lon ) = ( 0, 0 );
            }
            if ( ! $has_im ) {
                ( $im_lat, $im_lon ) = ( 0, 0 );
            }
            if ( $ILS{$CODES{$last_id} . $last_rwy} eq "" ) {
                print "DAFIFT adding: $CODES{$last_id} - $last_rwy\n";
                &safe_add_record( $CODES{$last_id}, $last_rwy, "ILS", 
                                  $loc_freq, $loc_id, $loc_hdg, $loc_lat,
                                  $loc_lon, $gs_elev, $gs_angle, $gs_lat,
                                  $gs_lon, $dme_lat, $dme_lon, $om_lat,
                                  $om_lon, $mm_lat, $mm_lon, $im_lat,
                                  $im_lon );
            }

            $has_dme = 0;
            $has_gs = 0;
            $has_loc = 0;
            $has_im = 0;
            $has_mm = 0;
            $has_om = 0;
        }

        $type = $F[2];
        if ( $type eq "D" ) {
            # DME entry
            $has_dme = 1;
            $dme_lon = make_dcoord( $F[16] );
            $dme_lat = make_dcoord( $F[14] );
            $dme_elev = $F[10];
            if ( $dme_elev !~ m/\d/ ) {
                $dme_elev = "";
            } else {
                $dme_elev += 0;
            }
            $dme_bias = $F[27];
            # print "$id DME $dme_lon $dme_lat $dme_elev $dme_bias\n";
        } elsif ( $type eq "G" ) {
            # GlideSlope entry
            $has_gs = 1;
            $gs_lon = make_dcoord( $F[16] );
            $gs_lat = make_dcoord( $F[14] );
            $gs_elev = $F[10];
            if ( $gs_elev !~ m/\d/ ) {
                $gs_elev = "";
            } else {
                $gs_elev += 0;
            }
            $gs_angle = $F[7];
            # print "$id GS $gs_lon $gs_lat $gs_elev $gs_angle\n";
        } elsif ( $type eq "Z" ) {
            # Localizer entry
            $has_loc = 1;
            $loc_lon = make_dcoord( $F[16] );
            $loc_lat = make_dcoord( $F[14] );
            $loc_elev = $F[10];
            if ( $loc_elev !~ m/\d/ ) {
                $loc_elev = "";
            } else {
                $loc_elev += 0;
            }
            ($loc_freq) = $F[5] =~ m/(\d\d\d\d\d\d)/;
            $loc_freq /= 1000.0;
            my( $magvar ) = make_dmagvar( $F[22] );
            # print "mag var = $F[22] (" . $magvar . ")\n";
            $loc_hdg = $F[24] + make_dmagvar( $F[22] );
            $loc_width = $F[25];
            $loc_id = $F[18];
            if ( length( $loc_id ) >= 4 ) {
                $loc_id =~ s/^I//;
            }
            # print "$id LOC $loc_lon $loc_lat $loc_elev $loc_freq $loc_hdg $loc_width\n";
        } elsif ( $type eq "I" ) {
            # Inner marker entry
            $has_im = 1;
            $im_lon = make_dcoord( $F[16] );
            $im_lat = make_dcoord( $F[14] );
            # print "$id IM $im_lon $im_lat\n";
        } elsif ( $type eq "M" ) {
            # Middle marker entry
            $has_mm = 1;
            $mm_lon = make_dcoord( $F[16] );
            $mm_lat = make_dcoord( $F[14] );
            # print "$id MM $mm_lon $mm_lat\n";
        } elsif ( $type eq "O" ) {
            # Outer marker entry
            $has_om = 1;
            $om_lon = make_dcoord( $F[16] );
            $om_lat = make_dcoord( $F[14] );
            # print "$id OM $om_lon $om_lat\n";
        }

        $last_id = $id;
        $last_rwy = $rwy;
        # printf("%-5s %10.6f %11.6f\n",  $F[0], $F[14], $F[16]);
    }

    if ( ! $has_gs ) {
        ( $gs_elev, $gs_angle, $gs_lat, $gs_lon ) = ( 0, 0, 0, 0 );
    }
    if ( ! $has_dme ) {
        ( $dme_lat, $dme_lon ) = ( 0, 0 );
    }
    if ( ! $has_om ) {
        ( $om_lat, $om_lon ) = ( 0, 0 );
    }
    if ( ! $has_mm ) {
        ( $mm_lat, $mm_lon ) = ( 0, 0 );
    }
    if ( ! $has_im ) {
        ( $im_lat, $im_lon ) = ( 0, 0 );
    }
    if ( $ILS{$CODES{$last_id} . $last_rwy} eq "" ) {
        print "DAFIFT adding (last): $CODES{$last_id} - $last_rwy\n";
        &safe_add_record( $CODES{$last_id}, $last_rwy, "ILS", $loc_freq,
                          $loc_id, $loc_hdg, $loc_lat, $loc_lon,
                          $gs_elev, $gs_angle, $gs_lat, $gs_lon,
                          $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat,
                          $mm_lon, $im_lat, $im_lon );
    }
}


########################################################################
# Process FAA data
########################################################################

sub load_faa() {
    my( $file ) = shift;

    open( FAA_ILS, "<$file" ) || die "Cannot open FAA data: $file\n";

    <FAA_ILS>;                          # skip header line

    while ( <FAA_ILS> ) {
        chomp;

        my ( $rec_type, $faa_id, $rwy, $type, $faa_date,
             $faa_apt_name, $faa_city, $faa_st, $faa_state,
             $faa_region, $id, $faa_len, $faa_wid, $faa_cat,
             $faa_owner, $faa_operator, $faa_bearing, $faa_magvar,
             $loc_type, $loc_id, $loc_freq, $faa_loc_latd,
             $faa_loc_lats, $faa_loc_lond, $faa_loc_lons, $loc_width,
             $faa_stop_dist, $faa_app_dist, $faa_gs_type, $gs_angle,
             $faa_gs_freq, $faa_gs_latd, $faa_gs_lats, $faa_gs_lond,
             $faa_gs_lons, $faa_gs_dist, $gs_elev, $faa_im_type,
             $faa_im_latd, $faa_im_lats, $faa_im_lond, $faa_im_lons,
             $faa_im_dist, $faa_mm_type, $faa_mm_id, $faa_mm_name,
             $faa_mm_freq, $faa_mm_latd, $faa_mm_lats, $faa_mm_lond,
             $faa_mm_lons, $faa_mm_dist, $faa_om_type, $faa_om_id,
             $faa_om_name, $faa_om_freq, $faa_om_latd, $faa_om_lats,
             $faa_om_lond, $faa_om_lons, $faa_om_dist,
             $faa_om_backcourse, $faa_dme_channel, $faa_dme_latd,
             $faa_dme_lats, $faa_dme_lond, $faa_dme_lons, $faa_dme_app_dist,
             $faa_dme_stop_dist, $blank)
            = $_ =~
            m/^(.{4})(.{11})(.{3})(.{10})(.{10})(.{42})(.{26})(.{2})(.{20})(.{3})(.{4})(.{5})(.{4})(.{9})(.{50})(.{50})(.{3})(.{3})(.{15})(.{5})(.{6})(.{14})(.{11})(.{14})(.{11})(.{5})(.{5})(.{6})(.{15})(.{4})(.{6})(.{14})(.{11})(.{14})(.{11})(.{6})(.{7})(.{15})(.{14})(.{11})(.{14})(.{11})(.{6})(.{15})(.{2})(.{5})(.{3})(.{14})(.{11})(.{14})(.{11})(.{6})(.{15})(.{2})(.{5})(.{3})(.{14})(.{11})(.{14})(.{11})(.{6})(.{9})(.{4})(.{14})(.{11})(.{14})(.{11})(.{6})(.{5})(.{34})/;

        $id = &strip_ws( $id );
        $rwy = &strip_ws( $rwy );
        $rwy =~ s/\/$//;
        $rwy =~ s/\/$//;
        $loc_id =~ s/^I-//;
        my( $loc_hdg ) = $faa_bearing + make_dmagvar($faa_magvar);
        my( $loc_lat ) = make_dcoord($faa_loc_lats) / 3600.0;
        my( $loc_lon ) = make_dcoord($faa_loc_lons) / 3600.0;
        # print "$loc_lon $loc_lat $faa_loc_lons $faa_loc_lats\n";
        my( $gs_lat ) = make_dcoord($faa_gs_lats) / 3600.0;
        my( $gs_lon ) = make_dcoord($faa_gs_lons) / 3600.0;
        my( $im_lat ) = make_dcoord($faa_im_lats) / 3600.0;
        my( $im_lon ) = make_dcoord($faa_im_lons) / 3600.0;
        my( $mm_lat ) = make_dcoord($faa_mm_lats) / 3600.0;
        my( $mm_lon ) = make_dcoord($faa_mm_lons) / 3600.0;
        my( $om_lat ) = make_dcoord($faa_om_lats) / 3600.0;
        my( $om_lon ) = make_dcoord($faa_om_lons) / 3600.0;
        my( $dme_lat ) = make_dcoord($faa_dme_lats) / 3600.0;
        my( $dme_lon ) = make_dcoord($faa_dme_lons) / 3600.0;

        # my( $key );
        # print "$id - $rwy\n";
        # $key = $id . $rwy;
        # print "-> $key -> $ILS{$key}\n";
        # $key = "K" . $id . $rwy;
        # print "-> $key -> $ILS{$key}\n";

        if ( $rec_type eq "ILS1" ) {
            if ( length( $id ) < 4 ) {
                if ( $CodesByICAO{"K" . $id} ) {
                    $id = "K" . $id;
                }
            }
            if ( $ILS{$id . $rwy} ne "" ) {
                print "FAA updating: $id - $rwy $type\n";
                &update_type( $id, $rwy, $type );
                if ( $gs_elev > 0 ) {
                    &maybe_update_gs_elev( $id . $rwy, $gs_elev );
                }
            } else {
                print "FAA adding: $id - $rwy\n";
                &safe_add_record( $id, $rwy, $type, $loc_freq, $loc_id,
                                  $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                                  $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                                  $dme_lon, $om_lat, $om_lon, $mm_lat,
                                  $mm_lon, $im_lat, $im_lon );
            }
        }
    }
}


########################################################################
# Process FlightGear ILS data
########################################################################

sub load_fgfs() {
    my( $ils_file ) = shift;

    open( FGILS, "zcat $ils_file|" ) || die "Cannot open FGFS: $ils_file\n";

    <FGILS>;                          # skip header line

    while ( <FGILS> ) {
        chomp;
        if ( ! m/\[End\]/ && length($_) > 1 ) {
            # print "$_\n";
            my( $type_code, $type_name, $icao, $rwy, $loc_freq, $loc_id,
                $loc_hdg, $loc_lat, $loc_lon, $gs_elev, $gs_angle, $gs_lat,
                $gs_lon, $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat, $mm_lon,
                $im_lat, $im_lon ) = split(/\s+/);
            my( $code ) = $icao;
            $code =~ s/^K//;
            if ( $ILS{$icao . $rwy} ne "" ) {
                print "FGFS: Skipping $icao - $rwy - already exists\n";
                # skip approaches already in FAA or DAFIFT data
            } elsif ( length( $icao ) < 4 || $icao =~ m/^K/ ) {
                print "FGFS: Skipping $icao - $rwy - USA\n";
                # skip USA approaches not found in FAA or DAFIFT data
            } else {
                print "FGFS adding: $icao $rwy\n";
                &safe_add_record( $icao, $rwy, $type_name, $loc_freq, $loc_id,
                                  $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                                  $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                                  $dme_lon, $om_lat, $om_lon, $mm_lat,
                                  $mm_lon, $im_lat, $im_lon );
            }
        } else {
            print "FGFS discarding: $_\n";
        }
    }
}


########################################################################
# Load the x-plane/flightgear airport/runway information
########################################################################

sub load_fgfs_airports() {
    my( $infile ) = shift;

    my( $id );

    open( IN, "zcat $infile|" ) || die "Cannot open: $infile\n";

    <IN>;                       # skip header line

    while ( <IN> ) {
        chomp;
        my(@F) = split(/\s+/);
        if ( $F[0] eq "A" ) {
            $id = $F[1];
        } elsif ( $F[0] eq "R" ) {
            my($recip) = &rwy_recip( $F[1] );
            $RUNWAYS{ $id . $F[1] } = "FOR $_";
            $RUNWAYS{ $id . $recip } = "REV $_";
        }
    }
}


########################################################################
# Run through the final ils list and adjust the heading to match the runway
########################################################################

sub fix_localizer() {
    my( $key );
    foreach $key ( sort (keys %ILS) ) {
        my(@F) = split( /\s+/, $ILS{$key} );
        print "FIXING: $key $F[2] $F[3] $F[4]\n";
        if ( $RUNWAYS{$key} ) {
            my(@G) = split( /\s+/, $RUNWAYS{$key} );
            print "  LocPos = $F[7] $F[8]\n";
            print "  RwyInfo = $G[0] $G[3] $G[4] $G[5] $G[6] $G[7]\n";
            my($cmd)
                = "$calc_loc $F[7] $F[8] $G[0] $G[3] $G[4] $G[5] $G[6] $G[7]";
            open( CALC, "$cmd |" ) || die "Cannot open $calc_loc\n";
            my( $j, $lat, $lon );
            while ( <CALC> ) {
                if ( m/New localizer/ ) {
                    ($j, $j, $j, $lat, $lon) = split( /\s+/ );
                }
            }
            if ( $G[0] eq "REV" ) {
                $G[5] += 180.0;
                if ( $G[5] >= 360.0 ) {
                    $G[5] -= 360.0;
                }
            }
            &update_loc( $F[2], $F[3], $lat, $lon, $G[5]);
        }
    }
}


########################################################################
# Run through the final ils list and fixup any missing glide slope elevations
# to match that of the field elevation.
########################################################################

sub fudge_missing_gs_elev() {
    my( $key );
    foreach $key ( sort (keys %ILS) ) {
        my(@F) = split( /\s+/, $ILS{$key} );
        if ( $F[9] <= 0 ) {            
            print "FUDGING GS: $key $F[2] $F[3] $F[4] - $F[9]\n";
            &maybe_update_gs_elev( $key, $Elevations{$F[2]});
        }
    }
}


########################################################################
# Write out the accumulated combined result
########################################################################

sub write_result() {
    my( $outfile ) = shift;

    open( OUT, ">$outfile" )  || die "Cannot write to: $outfile\n";

    # dump out the final results
    print OUT "// FlightGear ILS data, generated from DAFIFT ARPT/ILS.TXT and FAA data\n";

    my( $key );
    foreach $key ( sort (keys %ILS) ) {
        print OUT "$ILS{$key}\n";
    }
    print OUT "[End]\n";
}


########################################################################
# Utility functions
########################################################################


# add a record to the master list if it doesn't already exist

sub safe_add_record() {
    my( $apt_id ) = shift;
    my( $rwy ) = shift;
    my( $type ) = shift;
    my( $loc_freq ) = shift;
    my( $loc_id ) = shift;
    my( $loc_hdg ) = shift;
    my( $loc_lat ) = shift;
    my( $loc_lon ) = shift;
    my( $gs_elev ) = shift;
    my( $gs_angle ) = shift;
    my( $gs_lat ) = shift;
    my( $gs_lon ) = shift;
    my( $dme_lat ) = shift;
    my( $dme_lon ) = shift;
    my( $om_lat ) = shift;
    my( $om_lon ) = shift;
    my( $mm_lat ) = shift;
    my( $mm_lon ) = shift;
    my( $im_lat ) = shift;
    my( $im_lon ) = shift;

    if ( $ILS{$apt_id . $rwy} eq "" ) {
        # print "Safe adding (common): $apt_id - $rwy\n";
        &update_record( $apt_id, $rwy, $type, $loc_freq, $loc_id,
                        $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                        $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                        $dme_lon, $om_lat, $om_lon, $mm_lat,
                        $mm_lon, $im_lat, $im_lon );
    }
}


# replace a record in the master list (or add it if it doesn't exist)

sub update_record() {
    my( $apt_id ) = shift;
    my( $rwy ) = shift;
    my( $type ) = shift;
    my( $loc_freq ) = shift;
    my( $loc_id ) = shift;
    my( $loc_hdg ) = shift;
    my( $loc_lat ) = shift;
    my( $loc_lon ) = shift;
    my( $gs_elev ) = shift;
    my( $gs_angle ) = shift;
    my( $gs_lat ) = shift;
    my( $gs_lon ) = shift;
    my( $dme_lat ) = shift;
    my( $dme_lon ) = shift;
    my( $om_lat ) = shift;
    my( $om_lon ) = shift;
    my( $mm_lat ) = shift;
    my( $mm_lon ) = shift;
    my( $im_lat ) = shift;
    my( $im_lon ) = shift;

    my( $record );

    # remap $type as needed
    $type = &strip_ws( $type );
    if ( $type eq "LOCALIZER" ) {
        $type = "LOC";
    } elsif ( $type eq "ILS/DME" ) {
        $type = "ILS";
    } elsif ( $type eq "SDF/DME" ) {
        $type = "SDF";
    } elsif ( $type eq "LOC/DME" ) {
        $type = "ILS";
    } elsif ( $type eq "LOC/GS" ) {
        $type = "LOC";
    } elsif ( $type eq "LDA/DME" ) {
        $type = "LDA";
    }

    $record = sprintf( "%1s %-5s %-4s %-3s  %06.2f %-4s %06.2f %10.6f %11.6f ",
                       substr( $type, 0, 1 ), $type, $apt_id, $rwy,
                       $loc_freq, $loc_id, $loc_hdg, $loc_lat, $loc_lon );
    $record .= sprintf( "%5d %5.2f %10.6f %11.6f ",
                        $gs_elev, $gs_angle, $gs_lat, $gs_lon );
    $record .= sprintf( "%10.6f %11.6f ", $dme_lat, $dme_lon );
    $record .= sprintf( "%10.6f %11.6f ", $om_lat, $om_lon );
    $record .= sprintf( "%10.6f %11.6f ", $mm_lat, $mm_lon );
    $record .= sprintf( "%10.6f %11.6f ", $im_lat, $im_lon );

    # print "Updating (common): $apt_id - $rwy\n";
    $ILS{$apt_id . $rwy} = $record;
    $AIRPORTS{$apt_id} = 1;
}


# update the $type of the record
sub update_type() {
    my( $apt_id ) = shift;
    my( $rwy ) = shift;
    my( $new_type ) = shift;

    my( $record );

    if ( $ILS{$apt_id . $rwy} ne "" ) {
        my( $type_code, $type_name, $apt_id, $rwy, $loc_freq, $loc_id,
            $loc_hdg, $loc_lat, $loc_lon, $gs_elev, $gs_angle, $gs_lat,
            $gs_lon, $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat, $mm_lon,
            $im_lat, $im_lon ) = split( /\s+/, $ILS{$apt_id . $rwy} );
        # print "Updating type: $apt_id $rwy: $type_name -> $new_type\n";
        $type_name = $new_type;
        &update_record( $apt_id, $rwy, $type_name, $loc_freq, $loc_id,
                        $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                        $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                        $dme_lon, $om_lat, $om_lon, $mm_lat,
                        $mm_lon, $im_lat, $im_lon );
    } else {
        die "Error, trying to update $apt_id - $rwy which doesn't exist\n";
    }
}


# update the glide slope elevation of the record (but only if it is 0)
sub maybe_update_gs_elev() {
    my( $key ) = shift;
    my( $new_gs_elev ) = shift;

    my( $record );

    if ( $ILS{$key} ne "" ) {
        my( $type_code, $type_name, $apt_id, $rwy, $loc_freq, $loc_id,
            $loc_hdg, $loc_lat, $loc_lon, $gs_elev, $gs_angle, $gs_lat,
            $gs_lon, $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat, $mm_lon,
            $im_lat, $im_lon ) = split( /\s+/, $ILS{$key} );
        if ( $gs_elev == 0 ) {
            print "Updating gs elev: $apt_id $rwy: $gs_elev -> $new_gs_elev\n";
            $gs_elev = $new_gs_elev;
            &update_record( $apt_id, $rwy, $type_name, $loc_freq, $loc_id,
                            $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                            $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                            $dme_lon, $om_lat, $om_lon, $mm_lat,
                            $mm_lon, $im_lat, $im_lon );
        }
    } else {
        die "Error, trying to update $key which doesn't exist\n";
    }
}


# update the localizer position of the record
sub update_loc() {
    my( $apt_id ) = shift;
    my( $rwy ) = shift;
    my( $nloc_lat ) = shift;
    my( $nloc_lon ) = shift;
    my( $nloc_hdg ) = shift;

    my( $record );

    if ( $ILS{$apt_id . $rwy} ne "" ) {
        my( $type_code, $type_name, $apt_id, $rwy, $loc_freq, $loc_id,
            $loc_hdg, $loc_lat, $loc_lon, $gs_elev, $gs_angle, $gs_lat,
            $gs_lon, $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat, $mm_lon,
            $im_lat, $im_lon ) = split( /\s+/, $ILS{$apt_id . $rwy} );
        print "    Old pos = " . $loc_lat . "," . $loc_lon . " " . $loc_hdg . "\n";
        print "    New pos = " . $nloc_lat . "," . $nloc_lon . " " . $nloc_hdg . "\n";
        $loc_lat = $nloc_lat;
        $loc_lon = $nloc_lon;
        $loc_hdg = $nloc_hdg;
        &update_record( $apt_id, $rwy, $type_name, $loc_freq, $loc_id,
                        $loc_hdg, $loc_lat, $loc_lon, $gs_elev,
                        $gs_angle, $gs_lat, $gs_lon, $dme_lat,
                        $dme_lon, $om_lat, $om_lon, $mm_lat,
                        $mm_lon, $im_lat, $im_lon );
    } else {
        die "Error, trying to update $apt_id - $rwy which doesn't exist\n";
    }
}


# convert a lon/lat coordinate in various formats to signed decimal

sub make_dcoord() {
    my($coord) = shift;
    my( $dir, $deg, $min, $sec );
    my( $value ) = 0.0;

    $coord = &strip_ws( $coord );

    if ( $coord =~ m/^[WE]/ ) {
        ( $dir, $deg, $min, $sec )
            = $coord =~ m/^([EW])(\d\d\d)(\d\d)(\d\d\d\d)/;
        $value = $deg + $min/60.0 + ($sec/100)/3600.0;
        if ( $dir eq "W" ) {
            $value = -$value;
        }
    } elsif ( $coord =~ m/^[NS]/ ) {
        ( $dir, $deg, $min, $sec )
            = $coord =~ m/^([NS])(\d\d)(\d\d)(\d\d\d\d)/;
        $value = $deg + $min/60.0 + ($sec/100)/3600.0;
        if ( $dir eq "S" ) {
            $value = -$value;
        }
    } elsif ( $coord =~ m/[EW]$/ ) {
        ($value, $dir) = $coord =~ m/([\d\s\.]+)([EW])/;
        if ( $dir eq "W" ) {
            $value = -$value;
        }
    } elsif ( $coord =~ m/[NS]$/ ) {
        ($value, $dir) = $coord =~ m/([\d\s\.]+)([NS])/;
        if ( $dir eq "S" ) {
            $value = -$value;
        }
    }
    # print "$dir $deg:$min:$sec = $value\n";
    return $value;
}

# convert a magnetic variation in various formats to signed decimal

sub make_dmagvar() {
    my( $coord ) = shift;
    my( $value );

    if ( $coord =~ m/^[EW]/ ) {
        my( $dir, $deg, $min, $date )
            = $coord =~ m/^([EW])(\d\d\d)(\d\d\d) (\d\d\d\d)/;
        $value = $deg + ($min/10)/60.0;
        if ( $dir eq "W" ) {
            $value = -$value;
        }
    } elsif ( $coord =~ m/[EW]$/ ) {
        my( $deg, $dir )
            = $coord =~ m/^(\d\d)([EW])/;
        $value = $deg;
        if ( $dir eq "W" ) {
            $value = -$value;
        }
    }
    # print "$dir $deg:$min = $value\n";

    return $value;
}

# strip white space off front and back of string

sub strip_ws() {
    my( $string ) = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}


# return the reciprical runway number
sub rwy_recip() {
    my( $input ) = shift;

    my($num, $letter);

    if ( length($input) == 3 ) {
        ($num, $letter) = $input =~ m/(\d\d)(.)/;
    } elsif ( length($input) == 2 ) {
        $num = $input;
        $letter = "";
    } else {
        $num = "";
        $letter = $input;
    }
    # print "RWY: $num - $letter <==> ";

    if ( $num ne "" ) {
        $num += 18;
        if ( $num > 35 ) {
            $num -= 36;
        }
        if ( $num < 10 ) {
            $num = "0$num";
        }
    }

    if ( $letter eq "R" ) {
        $letter = "L";
    } elsif ( $letter eq "L" ) {
        $letter = "R";
    } elsif ( $letter eq "C" ) {
        $letter = "C";
    } elsif ( $letter eq "N" ) {
        $letter = "S";
    } elsif ( $letter eq "S" ) {
        $letter = "N";
    } elsif ( $letter eq "E" ) {
        $letter = "W";
    } elsif ( $letter eq "W" ) {
        $letter = "E";
    }
    # print "$num - $letter\n";

    return "$num$letter";
}
