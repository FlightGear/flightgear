#! /usr/bin/perl -w

sub usage {
  print <<EoF;
Read the apt.dat (or apt.dat.gz) file.
Remap airport names to remove ugly abbreviations.
Print airport names, one per line.

Remapping is done by reference to the atis_remap.hxx file.

Typical usage:
  FG_ROOT=whatever FG_SRC=whatever ATIS_ONLY=yes ./list-airports.pl | ./words_per_line.sh > airports.vlist
EoF
}

use strict;
use Symbol;

  my $noparen = 1;
  my $verbose = 0;
  my $apt_name = '';
  my $lat;
  my $lon;
  my $atis;
  my $country = '';
  my $elev;
  my $tower;
  my $bldgs;
  my $apt_id;
  my $shapefile;
  my $namer = 'NAME';
  my $skipping = 0;
  my $tot_apts = 0;

  my %states = ();
  my %short_country = ();


  my $fgroot = $ENV{'FG_ROOT'} || '.';
  my $atis_only = $ENV{'ATIS_ONLY'} || 0;
  my $mapfn = "$ENV{'FG_SRC'}/src/ATCDCL/atis_remap.hxx";

sub process_apt {
  if ($atis_only && ! $atis) {
    return;
  }
  my $str .= $apt_name;

  $str =~ s' *$'';              ## remove trailing spaces
  if ($noparen) {
    $str =~ s/[(][^)]*[)]?//g;
  }
  print "$str\n";
  $tot_apts++;
}

my %remap = ();

sub get_remap {

# Note: in this context, GKI probably stands for Gereja Kristen Indonesia
# I guess the church builds lots of airports.

  my $mapch = Symbol::gensym;
  if (!open($mapch, '<', $mapfn)) {
    print STDERR "Could not open abbreviation file '$mapfn'\n";
    print STDERR "Maybe you need to set FG_ROOT\n";
    exit(1);
  }
  while (my $line = <$mapch>) {
    chomp $line;
    if ($line =~ s/[ \t]*REMAP[(]//) {
      $line =~ s/[)][ \t]*$//;
      my @stuff = split(',', $line, 2);
      my $from = $stuff[0];
      my $to = $stuff[1];
      $to =~ s/^[ \t]*//;
      if ($to eq 'NIL') {
        $to = '';
      }
      $remap{$from} = $to;
    }
  }
}

main: {
  if (@ARGV) {
    usage;
    exit;
  }

  get_remap;

  my $delim = '-';
  my $fgroot = $ENV{'FG_ROOT'} || 0;
  my $incmd = "zcat $fgroot/Airports/apt.dat.gz";
  my $inch = Symbol::gensym;
  open ($inch, '-|', $incmd)
        || die "Couldn't open pipe from '$incmd'\n";


  my $junk = <$inch>;
  $junk = <$inch>;
  liner: while (my $line = <$inch>) {
    chomp $line;
    my @stuff = split(' ', $line);
    my $type = shift @stuff || 0;
###    print "..$type ... $line ...\n";

    if ($type == 1) {
## Here if new airport.
##
## First, print results of previous work, i.e. airport
## stanzas already seen ... since the apt.dat file
## doesn't have a clear way of signaling the end of a stanza.
      if ($apt_name) {
        process_apt();
      }
      $apt_name = '';
      $atis = '';
      $lat = 0;
      $lon = 0;
      $country = '';

      $elev = shift @stuff;
      $tower = shift @stuff;
      $bldgs = shift @stuff;
      $apt_id = shift @stuff;
      my $name = join $delim, @stuff;

      for my $from (keys %remap) {
        my $to = $remap{$from};
        $name =~ s/\b$from\b/$to/gi;
      }

## option for plain words, not hyphenated phrases
      if (1) {
        $name =~ s/$delim/ /g;
      }

      $apt_name = "$name";
    }

    if ($type == 10) {
      $lat = $stuff[0];
      $lon = $stuff[1];
    }

    if ($type == 50) {
      $atis = join(' ', @stuff);
    }
  }
  process_apt();          ## flush out the very last one
  print STDERR "Total airports: $tot_apts\n";
}
