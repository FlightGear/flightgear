#!/usr/bin/perl
########################################################################
# Convert DAFIFT NAV.TXT to FlightGear format.
########################################################################

use strict;

my @TYPES = (
             '',                # Unknown
             'V',               # VOR
             'V',               # VORTAC
             'D',               # TACAN
             'V',               # VOR/DME
             'N',               # NDB
             '',
             'N',               # NDB/DME
             '',
             'D'                # DME
            );

my @TYPE_NAMES = (
                  '',
                  'VOR',
                  'VORTAC',
                  'TACAN',
                  'VOR/DME',
                  'NDB',
                  '',
                  'NDB/DME',
                  '',
                  'DME'
                 );

my @HAS_DME = (
               'N',             # Unknown
               'N',             # VOR
               'Y',             # VORTAC
               'Y',             # TACAN
               'Y',             # VOR/DME
               'N',             # NDB
               'N',
               'Y',             # NDB/DME (not used, though)
               'N',
               'Y'              # DME
              );

# Make a frequency from a DME channel
sub make_freq {
  my ($type, $channel) = (@_);
  my $offset = 0;
  $offset = 0.05 if ($channel =~ /Y$/);
  if ($channel < 67) {
    return 108 + (($channel - 17)/10.0) + $offset;
  } else {
    return 112 + (($channel - 67)/10.0) + $offset;
  }
}

# Make a range based on navaid type and purpose
sub make_range {
  my ($type, $usage) = (@_);
  if ($type == 1 || $type == 2 || $type ==4) { # VOR
    if ($usage == 'H' || $usage == 'B') {
      return 200;
    } elsif ($usage == 'T') {
      return 20;
    } else {
      return 50;
    }
  } elsif ($type == 3 || $type == 7) {        # DME
    if ($usage == 'T') {
      return 50;
    } else {
      return 200;
    }
  } else {                      # NDB
    if ($usage == 'T') {
      return 50;
    } else {
      return 200;
    }
  }
}

sub write_navaid {
  my ($type, $lat, $lon, $elev, $freq, $range, $dme, $id, $magvar, $name)
    = (@_);
      
  printf("%s %10.6f %11.6f %6d %7.2f %4d %s %-4s %s %s %s\n",
         $TYPES[$type], $lat, $lon, $elev, $freq, $range, $dme, $id,
         $magvar, $name, $TYPE_NAMES[$type]);
}

sub make_dmagvar {
    my($coord) = shift;
    my( $value );
    my( $dir, $deg, $date ) = $coord =~ m/^([EW])(\d\d\d\d)(\d\d\d\d)/;
    $value = $deg / 10.0;
    if ( $dir eq "W" ) {
        $value = -$value;
    }

    return $value;
}


<>;                             # skip header line

print "// FlightGear navaid data, generated from DAFIFT NAV.TXT\n";

while (<>)
{
  chop;
  my @F = split(/\t/);

  my $type = $F[1];
  if ($TYPES[$type] eq '') {
    warn("Bad type for " . $F[0] . "(" . $F[5] . ")\n");
    next;
  }
  my $lat = $F[18];
  my $lon = $F[20];
  my $elev = $F[23];
  my $freq = $F[8]/1000;
  if ($type == 3 || $type == 9) {
    $freq = make_freq($type, $F[10]);
  }
  my $range = 0 + $F[14];
  if ($range == 0) {
    $range = make_range($type, $F[9]);
  }
  my $id = $F[0];
  my $magvar = $F[21];
  if ($magvar eq '') {
    $magvar = 'XXX';
  } else {
    my $tmp = make_dmagvar( $magvar );
    # print "$magvar $tmp\n";
    if ( $tmp <= 0 ) {
        $magvar = sprintf("%02.0fW", -$tmp );
    } else {
        $magvar = sprintf("%02.0fE", $tmp );
    }
  }
  my $name = $F[5];

  if ($type == 7) {             # NDB/DME
    write_navaid(9, $lat, $lon, $elev, make_freq(9, $F[10]), $range,
                 'Y', $id, $magvar, $name);
    $type = 5;
  }
  write_navaid($type, $lat, $lon, $elev, $freq, $range,
               $HAS_DME[$F[1]], $id, $magvar, $name);


}

print "[End]\n";

# end of dafif2fix.pl
