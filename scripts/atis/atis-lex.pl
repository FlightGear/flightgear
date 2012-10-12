#! /usr/bin/perl -w

sub usage {
  print <<EoF;
Read the atis_lexicon.hxx file and print
the vocabulary words ... plus phonetic digits and letters.

See also list-airports.pl

Typical usage:
   FG_ROOT=/home/whatever/fgdata FG_SRC=/home/whatever/flightgear ./atis-lex.pl > phraseology.vlist
EoF
}

use strict;
use Symbol;

  my $fgroot = $ENV{'FG_ROOT'} || '.';

main: {
  if (@ARGV) {
    usage;
    exit;
  }
  my $mapfn = "$ENV{'FG_SRC'}/src/ATCDCL/atis_lexicon.hxx";
  my $mapch = Symbol::gensym;
  if (!open($mapch, '<', $mapfn)) {
    print STDERR "Could not open abbreviation file '$mapfn'\n";
    print STDERR "Maybe you need to set FG_ROOT\n";
    exit(1);
  }
  print "/\n";
  while (my $line = <$mapch>) {
    chomp $line;
    if ($line =~ s/^[ \t]*Q[(]//) {
      $line =~ s/[)][ \t]*$//;
      print "$line\n";
    }
  }
  print <<EoF;
zero
one
two
three
four
five
six
seven
eight
nine
niner
alpha
bravo
charlie
delta
echo
foxtrot
golf
hotel
india
juliet
kilo
lima
mike
november
oscar
papa
quebec
romeo
sierra
tango
uniform
victor
whiskey
xray
yankee
zulu
decimal
EoF
}
