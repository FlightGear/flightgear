#!/usr/bin/perl

########################################################################
# Convert DAFIFT ARPT/ILS.TXT to FlightGear format.
########################################################################

use strict;

my($faa_ils_file) = shift(@ARGV);
my($arpt_file) = shift(@ARGV);
my($dafift_ils_file) = shift(@ARGV);
my($fgfs_ils_file) = shift(@ARGV);

die "Usage: $0 " .
    "<faa_ils_file> <dafift_arpt_file> <dafift_ils_file> <fgfs_ils_file>\n"
    if !defined($faa_ils_file) || !defined($arpt_file)
       || !defined($dafift_ils_file) || !defined($fgfs_ils_file);

my( %Airports );
my( %ILS );
my( $record );

my( $id, $rwy, $type );
my( $has_dme, $has_gs, $has_loc, $has_im, $has_mm, $has_om );
my( $dme_lon, $dme_lat, $dme_elev, $dme_bias );
my( $gs_lon, $gs_lat, $gs_elev, $gs_angle );
my( $loc_type, $loc_lon, $loc_lat, $loc_elev, $loc_freq, $loc_hdg, $loc_width,
    $loc_id );
my( $im_lon, $im_lat, $mm_lon, $mm_lat, $om_lon, $om_lat );


########################################################################
# Process FAA data
########################################################################

# Load the DAFIFT ils file

my( $rec_type, $faa_id, $faa_date, $faa_apt_name, $faa_city, $faa_st,
    $faa_state, $faa_region, $id, $faa_len, $faa_wid, $faa_cat, $faa_owner,
    $faa_operator, $faa_bearing, $faa_magvar,
    $faa_loc_latd, $faa_loc_lats, $faa_loc_lond, $faa_loc_lons,
    $faa_stop_dist, $faa_app_dist,
    $faa_gs_type, $faa_gs_freq, $faa_gs_latd, $faa_gs_lats, $faa_gs_lond,
    $faa_gs_lons, $faa_gs_dist,
    $faa_im_type, $faa_im_latd, $faa_im_lats, $faa_im_lond, $faa_im_lons,
    $faa_im_dist,
    $faa_mm_type, $faa_mm_id, $faa_mm_name, $faa_mm_freq,
    $faa_mm_latd, $faa_mm_lats, $faa_mm_lond, $faa_mm_lons, $faa_mm_dist,
    $faa_om_type, $faa_om_id, $faa_om_name, $faa_om_freq,
    $faa_om_latd, $faa_om_lats, $faa_om_lond, $faa_om_lons, $faa_om_dist,
    $faa_om_backcourse);
open( FAA_ILS, "<$faa_ils_file" ) || die "Cannot open $faa_ils_file\n";

<FAA_ILS>;                          # skip header line

while ( <FAA_ILS> ) {
    chomp;
    ( $rec_type, $faa_id, $rwy, $type, $faa_date, $faa_apt_name, $faa_city,
      $faa_st, $faa_state, $faa_region, $id, $faa_len, $faa_wid,
      $faa_cat, $faa_owner, $faa_operator, $faa_bearing, $faa_magvar,
      $loc_type, $loc_id, $loc_freq, $faa_loc_latd, $faa_loc_lats,
      $faa_loc_lond, $faa_loc_lons, $loc_width, $faa_stop_dist, $faa_app_dist,
      $faa_gs_type, $gs_angle, $faa_gs_freq, $faa_gs_latd, $faa_gs_lats,
      $faa_gs_lond, $faa_gs_lons, $faa_gs_dist, $gs_elev,
      $faa_im_type, $faa_im_latd, $faa_im_lats, $faa_im_lond, $faa_im_lons,
      $faa_im_dist,
      $faa_mm_type, $faa_mm_id, $faa_mm_name, $faa_mm_freq,
      $faa_mm_latd, $faa_mm_lats, $faa_mm_lond, $faa_mm_lons, $faa_mm_dist,
      $faa_om_type, $faa_om_id, $faa_om_name, $faa_om_freq,
      $faa_om_latd, $faa_om_lats, $faa_om_lond, $faa_om_lons,
      $faa_om_dist, $faa_om_backcourse
    )
        = $_ =~ m/^(.{4})(.{11})(.{3})(.{10})(.{10})(.{42})(.{26})(.{2})(.{20})(.{3})(.{4})(.{5})(.{4})(.{9})(.{50})(.{50})(.{3})(.{3})(.{15})(.{5})(.{6})(.{14})(.{11})(.{14})(.{11})(.{5})(.{5})(.{6})(.{15})(.{4})(.{6})(.{14})(.{11})(.{14})(.{11})(.{6})(.{7})(.{15})(.{14})(.{11})(.{14})(.{11})(.{6})(.{15})(.{2})(.{5})(.{3})(.{14})(.{11})(.{14})(.{11})(.{6})(.{15})(.{2})(.{5})(.{3})(.{14})(.{11})(.{14})(.{11})(.{6})(.{9})/;

    $loc_hdg = $faa_bearing + make_dmagvar($faa_magvar);
    $loc_lat = make_dcoord($faa_loc_lats) / 3600.0;
    $loc_lon = make_dcoord($faa_loc_lons) / 3600.0;
    $gs_lat = make_dcoord($faa_gs_lats) / 3600.0;
    $gs_lon = make_dcoord($faa_gs_lons) / 3600.0;
    $im_lat = make_dcoord($faa_im_lats) / 3600.0;
    $im_lon = make_dcoord($faa_im_lons) / 3600.0;
    $mm_lat = make_dcoord($faa_mm_lats) / 3600.0;
    $mm_lon = make_dcoord($faa_mm_lons) / 3600.0;
    $om_lat = make_dcoord($faa_om_lats) / 3600.0;
    $om_lon = make_dcoord($faa_om_lons) / 3600.0;

    # just hist the start of the next record, dump the current data
    $record = sprintf( "I ILS   %-4s %-3s  %06.2f %-4s %06.2f %10.6f %11.6f ",
                       $id, $rwy,
                       $loc_freq, $loc_id, $loc_hdg, $loc_lat, $loc_lon );
    $record .= sprintf( "%5d %5.2f %10.6f %11.6f ",
                        $gs_elev, $gs_angle, $gs_lat, $gs_lon );
    $record .= sprintf( "%10.6f %11.6f ", $dme_lat, $dme_lon );
    $record .= sprintf( "%10.6f %11.6f ", $om_lat, $om_lon );
    $record .= sprintf( "%10.6f %11.6f ", $mm_lat, $mm_lon );
    $record .= sprintf( "%10.6f %11.6f ", $im_lat, $im_lon );
    if ( $rec_type eq "ILS1" && $ILS{$id . $rwy} eq "" ) {
        print "FAA Adding: $id - $rwy $faa_bearing, $faa_magvar - $loc_hdg \n";
        $ILS{$id . $rwy} = $record;
    }
}


########################################################################
# Process DAFIFT data
########################################################################

# Load the DAFIFT airport file

open( ARPT, "<$arpt_file" ) || die "Cannot open $arpt_file\n";

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
    $Airports{$F[0]} = $icao;
    # print "$F[0] - $icao\n";
}


# Load the DAFIFT ils file

my( $last_id, $last_rwy ) = ("", "");

open( DAFIFT_ILS, "<$dafift_ils_file" ) || die "Cannot open $dafift_ils_file\n";

<DAFIFT_ILS>;                   # skip header line

while ( <DAFIFT_ILS> ) {
    chomp;
    my @F = split(/\t/);
    $id = $F[0];
    $rwy = $F[1];
    if ( $last_id ne "" && ($last_id ne $id || $last_rwy ne $rwy) ) {
        # just hist the start of the next record, dump the current data
        $record
            = sprintf( "I ILS   %-4s %-3s  %06.2f %-4s %06.2f %10.6f %11.6f ",
                       $Airports{$last_id}, $last_rwy,
                       $loc_freq, $loc_id, $loc_hdg, $loc_lat, $loc_lon );
        if ( $has_gs ) {
            $record .= sprintf( "%5d %5.2f %10.6f %11.6f ",
                                $gs_elev, $gs_angle, $gs_lat, $gs_lon );
        } else {
            $record .= sprintf( "%5d %5.2f %10.6f %11.6f ", 0, 0, 0, 0 );
        }
        if ( $has_dme ) {
            $record .= sprintf( "%10.6f %11.6f ", $dme_lat, $dme_lon );
        } else {
            $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
        }
        if ( $has_om ) {
            $record .= sprintf( "%10.6f %11.6f ", $om_lat, $om_lon );
        } else {
            $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
        }
        if ( $has_mm ) {
            $record .= sprintf( "%10.6f %11.6f ", $mm_lat, $mm_lon );
        } else {
            $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
        }
        if ( $has_im ) {
            $record .= sprintf( "%10.6f %11.6f ", $im_lat, $im_lon );
        } else {
            $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
        }
        if ( $ILS{$Airports{$last_id} . $last_rwy} eq "" ) {
            print "DAFIFT Adding: $Airports{$last_id} - $last_rwy\n";
            $ILS{$Airports{$last_id} . $last_rwy} = $record;
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

$record = sprintf( "I ILS   %-4s %-3s  %06.2f %-4s %06.2f %10.6f %11.6f ",
                   $Airports{$last_id}, $last_rwy,
                   $loc_freq, $loc_id, $loc_hdg, $loc_lat, $loc_lon );
if ( $has_gs ) {
    $record .= sprintf( "%5d %5.2f %10.6f %11.6f ",
                        $gs_elev, $gs_angle, $gs_lat, $gs_lon );
} else {
    $record .= sprintf( "%5d %5.2f %10.6f %11.6f ", 0, 0, 0, 0 );
}
if ( $has_dme ) {
    $record .= sprintf( "%10.6f %11.6f ", $dme_lat, $dme_lon );
} else {
    $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
}
if ( $has_om ) {
    $record .= sprintf( "%10.6f %11.6f ", $om_lat, $om_lon );
} else {
    $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
}
if ( $has_mm ) {
    $record .= sprintf( "%10.6f %11.6f ", $mm_lat, $mm_lon );
} else {
    $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
}
if ( $has_im ) {
    $record .= sprintf( "%10.6f %11.6f ", $im_lat, $im_lon );
} else {
    $record .= sprintf( "%10.6f %11.6f ", 0, 0 );
}

if ( $ILS{$Airports{$last_id} . $last_rwy} eq "" ) {
    print "DAFIFT Adding: $Airports{$last_id} - $last_rwy\n";
    $ILS{$Airports{$last_id} . $last_rwy} = $record;
}


########################################################################
# Process FlightGear ILS data
########################################################################

# Load the FlightGear ils file and fill in any missing ILS approachs

open( FGILS, "zcat $fgfs_ils_file|" ) || die "Cannot open $fgfs_ils_file\n";

<FGILS>;                          # skip header line

while ( <FGILS> ) {
    chomp;
    if ( ! m/\[End\]/ && length($_) > 1 ) {
        # print "$_\n";
        my( $type_code, $type_name, $icao, $rwy, $loc_freq, $loc_id, $loc_hdg,
            $loc_lat, $loc_lon, $gs_elev, $gs_angle, $gs_lat, $gs_lon,
            $dme_lat, $dme_lon, $om_lat, $om_lon, $mm_lat, $mm_lon,
            $im_lat, $im_lon ) = split(/\s+/);
        if ( $ILS{$icao . $rwy} eq "" ) {
            print "X-Plane Adding: $icao $rwy\n";
            $ILS{$icao . $rwy} = $_;
        }
    }
}


# dump out the final results
print "// FlightGear ILS data, generated from DAFIFT ARPT/ILS.TXT\n";
my($key);
foreach $key ( sort (keys %ILS) ) {
    print "$ILS{$key}\n";
}
print "[End]\n";


sub make_dcoord() {
    my($coord) = shift;
    my( $dir, $deg, $min, $sec );
    my( $value );
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
        ($value, $dir) = m/(\d{6}\.\d{3})(.)/;
        if ( $value eq "W" ) {
            $value = -$value;
        }
    } elsif ( $coord =~ m/[NS]$/ ) {
        ($value, $dir) = m/(\d{6}\.\d{3})(.)/;
        if ( $value eq "S" ) {
            $value = -$value;
        }
    }
    # print "$dir $deg:$min:$sec = $value\n";
    return $value;
}

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

# end of dafif2fix.pl
