#! /usr/bin/perl -w

use strict;
use Symbol;

sub usage {
  print <<EoF;

EoF
}

my $fgroot = $ENV{'FG_ROOT'} || '.';

my $dir="$fgroot/ATC";
my %start=();
my %len=();

my $str = 'Tucson International airport_information 
Ryan automated_weather_observation
zero four one five zulu weather
 / Wind one one zero at one five
 / Visibility one zero
 / sky_condition two thousand four hundred scattered
 / Temperature one zero celsius dewpoint five celsius
 / Altimeter two niner niner two
 / Landing_and_departing_runway one one right
 / on_initial_contact_advise_you_have_information zulu ';

main: {
  setup();
  unlink 'tmp.raw';
  $str =~ s/\n/ /g;
  ##print "$start{'decimal'} ... $len{'decimal'}\n";
  my $didsome = 0;
  for my $arg (@ARGV) {
    if ($arg ne '-') {
      say1($arg);
      $didsome++;
    } else {
      for my $word (split(' ', $str)){
        say1($word);
        $didsome++;
      }
    }
  }
  if ($didsome) {
    my $cmd = 'sox -q -r 8000 -t raw -e signed-integer -b 16 tmp.raw'
        . ' tmp.wav';
#        . ' -t alsa';
    print "$cmd\n";
    system $cmd;
  }
}



sub say1{
  my ($arg) = @_;
  $arg = lc($arg);
  if (exists $start{$arg}) {
    my $cmd = "sox  -q $dir/voice.wav "
       . " -t raw -r 8000 -e signed-integer -b 16 - "
       .  " trim $start{$arg}s $len{$arg}s"
       .  " >> tmp.raw ";
    print "$cmd\n";
    system $cmd;
    my $end = $start{$arg} + $len{$arg};
    print "$start{$arg} + $len{$arg} = $end\n";
  } else {
    print "Can't find '$arg'\n";
  }
}


sub setup{
  my $inch = Symbol::gensym();
  my $file = "$dir/voice.vce";
  open($inch, "<$file") || die "Cannot open input file '$file'\n";
  my $header = <$inch>;
  chomp $header;
  my $ii=1;
  liner: while (my $line = <$inch>){
    chomp $line;
    my @word = split(" ", $line);
    my $nn = @word;
    if ($nn != 3) {
      next liner;
    }
    my $id = lc($word[0]);
    my $st = $word[1];
    my $ln = $word[2];
    if ($ln =~ s/^x//) {
      $ln = $ln - $st;
      print "$id $st $ln\n";
    }
    $start{$id} = $st;
    $len{$id} = $ln;
    ##print "$ii        $nn     '$line'\n";
    $ii++;
  }
  print "(($header)) --> $ii\n";
}