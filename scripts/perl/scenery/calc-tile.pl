#!/usr/bin/perl -w
########################################################################
# calc-tile.pl
#
# Synopsis: Calculate a FlightGear tile base on longitude and latitude.
# Usage: perl calc-tile.pl <lon> <lat>
########################################################################

use strict;
use POSIX;



########################################################################
# Constants.
########################################################################

my $EPSILON = 0.0000001;
my $DIRSEP = '/';



########################################################################
# Functions.
########################################################################

#
# Calculate the number of columns of tiles in a degree of longitude.
#
sub bucket_span {
  my ($lat) = (@_);
  if ($lat>= 89.0 ) {
    return 360.0;
  } elsif ($lat>= 88.0 ) {
    return 8.0;
  } elsif ($lat>= 86.0 ) {
    return 4.0;
  } elsif ($lat>= 83.0 ) {
    return 2.0;
  } elsif ($lat>= 76.0 ) {
    return 1.0;
  } elsif ($lat>= 62.0 ) {
    return 0.5;
  } elsif ($lat>= 22.0 ) {
    return 0.25;
  } elsif ($lat>= -22.0 ) {
    return 0.125;
  } elsif ($lat>= -62.0 ) {
    return 0.25;
  } elsif ($lat>= -76.0 ) {
    return 0.5;
  } elsif ($lat>= -83.0 ) {
    return 1.0;
  } elsif ($lat>= -86.0 ) {
    return 2.0;
  } elsif ($lat>= -88.0 ) {
    return 4.0;
  } elsif ($lat>= -89.0 ) {
    return 8.0;
  } else {
    return 360.0;
  }
}

#
# Format longitude as e/w.
#
sub format_lon {
  my ($lon) = (@_);
  if ($lon < 0) {
    return sprintf("w%03d", int(0-$lon));
  } else {
    return sprintf("e%03d", int($lon));
  }
}

#
# Format latitude as n/s.
#
sub format_lat {
  my ($lat) = (@_);
  if ($lat < 0) {
    return sprintf("s%02d", int(0-$lat));
  } else {
    return sprintf("n%02d", int($lat));
  }
}

#
# Generate the directory name for a location.
#
sub directory_name {
  my ($lon, $lat) = (@_);
  my $lon_floor = POSIX::floor($lon);
  my $lat_floor = POSIX::floor($lat);
  my $lon_chunk = POSIX::floor($lon/10.0) * 10;
  my $lat_chunk = POSIX::floor($lat/10.0) * 10;
  return format_lon($lon_chunk) . format_lat($lat_chunk) . $DIRSEP
    . format_lon($lon_floor) . format_lat($lat_floor);
}

#
# Generate the tile index for a location.
#
sub tile_index {
  my ($lon, $lat) = (@_);
  my $lon_floor = POSIX::floor($lon);
  my $lat_floor = POSIX::floor($lat);
  my $span = bucket_span($lat);

  my $x;
  if ($span < $EPSILON) {
    $lon = 0;
    $x = 0;
  } elsif ($span <= 1.0) {
    $x = int(($lon - $lon_floor) / $span);
  } else {
    if ($lon >= 0) {
      $lon = int(int($lon/$span) * $span);
    } else {
      $lon = int(int(($lon+1)/$span) * $span - $span);
      if ($lon < -180) {
        $lon = -180;
      }
    }
    $x = 0;
  }

  my $y;
  $y = int(($lat - $lat_floor) * 8);


  my $index = 0;
  $index += ($lon_floor + 180) << 14;
  $index += ($lat_floor + 90) << 6;
  $index += $y << 3;
  $index += $x;

  return $index;
}



########################################################################
# Main program.
########################################################################

my ($lon, $lat) = (@ARGV);

my $dir = directory_name($lon, $lat);
my $index = tile_index($lon, $lat);
my $path = "$dir$DIRSEP$index.stg";

print "Longitude: $lon\n";
print "Latitude:  $lat\n";
print "Tile:      $index\n";
print "Path:      \"$path\"\n";

1;
