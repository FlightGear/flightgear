#!/usr/bin/perl
########################################################################
# Convert DAFIFT WPT.TXT to FlightGear format.
########################################################################

use strict;

<>;                             # skip header line

print "// FlightGear intersection data, generated from DAFIFT WPT.TXT\n";

while (<>)
{
  chop;
  my @F = split(/\t/);
  printf("%-5s %10.6f %11.6f\n",  $F[0], $F[14], $F[16]);
}

print "[End]\n";

# end of dafif2fix.pl
