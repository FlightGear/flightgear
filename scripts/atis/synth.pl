#! /usr/bin/perl -w

use strict;
use Symbol;
use Audio::Wav;

sub usage {
  print <<EoF;
Run a bunch of words through the speech synthesizer
and collect the results.
Typical usage:
  ./synth.pl [options] sport.vlist sport.vce sport.wav

  Options include:
    -skip       # skip a line from the .vlist file;
    -one        # take only the first word from each line

Note: -skip -one is useful if the .vlist file is actually in .vce format.

Other usages:
  ./synth.pl -dump foo.wav    # dump headers 'foo.wav'
  ./synth.pl -help            # print this message

where sport.vlist is a file containing a list of words, we use the
first word on each line and ignore anything else on that line
(which means a .vce file is acceptable as input, using -skip -one).

Note that "atis-lex.pl" and "list-airports.pl" are useful for 
creating the .vlist file.

Note that you may also need to:
  cpan Audio::Wav
  apt-get install festival mbrola sox festlex-oald
  cd \$tars
    wget http://tcts.fpms.ac.be/synthesis/mbrola/dba/en1/en1-980910.zip
    wget http://www.cstr.ed.ac.uk/downloads/festival/1.95/festvox_en1.tar.gz
  cd /usr/share/festival/voices/english
  mkdir en1_mbrola
  cd en1_mbrola
  unzip \$tars/en1-980910.zip
  cd /usr/share/festival
  mkdir lib
  cd lib
  ln -s ../voices ./
  cd /usr/share
  tar -xpzvf \$tars/festvox_en1.tar.gz

You may also need to scrounge a non-buggy version of
  /usr/local/share/perl/5.10.1/Audio/Wav/Read.pm
EoF
}

my $proto = <<EoF;
 (voice_en1_mbrola) ;; best voice I know of
 (setq voice_rab_diphone voice_en1_mbrola)
 (setq voice_en1 voice_en1_mbrola)
 (setq voice_el_diphone voice_en1_mbrola)
 (setq voice_ked_diphone voice_en1_mbrola)
 (utt.save.wave (utt.synth (Utterance Text "XXX")) "YYY" 'riff)
EoF

my %list;
my %count;

sub doit {
    my ($word) = @_;
    if (exists $list{lc $word}
      && $list{lc $word} =~ m'^[A-Z][a-z]') {
        ; # tasteful capitalization; leave as is
    } else {
      $list{lc $word} = $word;
    }
    $count{lc $word}++;
}

## This is an ugly way to fix festival's bad guess as to 
## pronunciation in special cases.
## In the case of wind (windh versus wynd), 
## nominally there are ways of tagging parts-of-speech,
## i.e. noun versus verb,
## but they don't seem to be reliable.
my %fixup = (
 'wind'  => 'windh',
 'romeo' => 'Rome E O',
 'xray'  => 'X ray',
);

main: {

  my $skip = 0;
  my $fmtcheck = 1;
  my $oneword = 0;
  my $gripe = 0;
  my $out_bits_sample = 8;              ## this is what FGFS expects
  my @plain_args = ();
  argx: while (@ARGV) {
    my $arg = shift @ARGV;
    $arg =~ s/^--/-/;
    if ($arg eq '-help' || $arg eq '-h') {
      usage;
      exit(0);
    }
    if ($arg eq '-dump') {
      my $ifn = shift @ARGV || 'foo.wav';
      my $wav = new Audio::Wav;
      print "About to open '$ifn' ...\n";
      my $waver = $wav -> read( $ifn );
      print "xxxxxxxxxxxxxxxx\n";
      for my $detail (keys %{$waver->details()}) {
        printf("%-20s %s\n", $detail, ${$waver->details()}{$detail});
      }
      exit(0);
    }
    if ($arg eq '-skip') {
      $skip++;
      next argx;
    }
    if ($arg eq '-gripe') {
      $gripe++;
      next argx;
    }
    if ($arg eq '-one' || $arg eq '-1') {
      $oneword++;
      next argx;
    }
    if ($arg eq '-nocheck') {
      $fmtcheck=0;
      next argx;
    }
    if ($arg =~ '^-') {
      die "Unrecognized option '$arg'\n";
    }
    push @plain_args, $arg;
  }

  my $nargs = @plain_args;
  if ($nargs != 3) {
    die "Wrong number of arguments ($nargs); for help try:\n $0 -help\n";
  }
  my @todo = ();
  my ($ifn, $indexfn, $out_wav) = @plain_args;

  my $inch = Symbol::gensym;
  open ($inch, '<', $ifn)
        || die "Couldn't open input file '$ifn'\n";

# Skip some lines from the input list, as requested:
  for (my $ii = 0; $ii < $skip; $ii++) {
    my $ignore = <$inch>;
  }

# Read the rest of the input file:
  while (my $line = <$inch>) {
    chomp($line);
    if ($oneword) {
      my @stuff = split(' ', $line, 2);
      doit($stuff[0]);
    } else {
      for my $word (split(' ', $line)) { 
        doit($word);
      }      
    }
  }
  close $inch;

# Optionally print a list of things that the input file
# requested more than once.
  if ($gripe) {
    foreach my $thing (sort keys %count) {
      if ($count{$thing} > 1) {
        printf("%4d  %s\n", $count{$thing}, $list{$thing});
      }
    }
  }
  my $nsnip = (keys %list);

  if (0 && $nsnip > 10) {
    $nsnip = 10;
  }
  print STDERR "nsnip: $nsnip\n";

  my $index = Symbol::gensym;
  open ($index, '>', $indexfn)
        || die "Couldn't write index file '$indexfn'\n";

  print $index "$nsnip\n";
  if (! -d 'snip') {
    mkdir('snip')
      || die "Could not create directory 'snip' : $!\n";
  }

  my $wav = new Audio::Wav;
  my $waver = $wav -> read("quiet0.500.wav");
  my $sample_rate = -1;
  my $channels = -1;
  my $bits_sample = -1;
  $sample_rate = ${$waver->details()}{'sample_rate'};
  $channels    = ${$waver->details()}{'channels'};
  $bits_sample = ${$waver->details()}{'bits_sample'};

##############  system "/bin/cp nothing.wav t1.wav";
  my $where = 0;
  my $ii = 0;

  snipper: for my $thing (sort keys %list) {
    $ii++;
    my $iix = sprintf('%05d', $ii);
    my $xfn = "./snip/x$iix";
    print( "$xfn\n");

    my $fraise = lc($thing);
    if (exists $fixup{$fraise}) {
      #xxxx print "fixing $fraise\n";
      $fraise = $fixup{$fraise};
    }

## This turns dashes and other funny stuff into spaces
## in the phrase to be processed:
    $fraise =~ s%[^a-z']+% %gi;
    if ($thing eq '/' || $thing eq '/_') {
      system("/bin/cp quiet0.500.wav $xfn.wav");
    } else {
      my $script = $proto;
      $script =~ s/XXX/$fraise/;
      $script =~ s|YYY|$xfn.wav|;
      #xxxx print "$fraise ... $script\n";

      my $cmd = '/usr/bin/festival';
      my $pipe = Symbol::gensym;
      open ($pipe, '|-', $cmd)
            || die "Couldn't open pipe to '$cmd'\n";
      print $pipe $script;
      close $pipe;
      if ($? != 0){
        print STDERR "Error in festival script: '$script'\n";
        next snipper;
      }
    }
  }

  $ii = 0;
  snipper: for my $thing (sort keys %list) {
    $ii++;
    my $iix = sprintf('%05d', $ii);
    my $xfn = "./snip/x$iix";

    if ($fmtcheck == 1) {
      my $wav = new Audio::Wav;
      my $waver = $wav -> read("$xfn.wav");
      if ($sample_rate < 0) {
        $sample_rate = ${$waver->details()}{'sample_rate'};
        $channels    = ${$waver->details()}{'channels'};
        $bits_sample = ${$waver->details()}{'bits_sample'};
      } else {
           $sample_rate == ${$waver->details()}{'sample_rate'}
        && $channels    == ${$waver->details()}{'channels'}
        && $bits_sample == ${$waver->details()}{'bits_sample'}
        || die "audio format not the same: $xfn.wav";
      }
    }

    my $statcmd = "2>&1 sox $xfn.wav -n stat";
    my $stat = Symbol::gensym;
    open ($stat, '-|', $statcmd)
          || die "Couldn't open pipe from '$statcmd'\n";
    my $vol = 0;
    my $size = 0;

    my $lastline;
    while (my $line = <$stat>) {
      chomp $line;
      $lastline = $line;
      my @stuff = split ':', $line;
      my $nw = @stuff;
      #### print STDERR "$nw +++ $line\n";
      if ($nw == 2) {
        if ($stuff[0] eq 'Volume adjustment') {
          $vol = 0+$stuff[1];
        }
        elsif ($stuff[0] eq 'Samples read') {
          $size = 0+$stuff[1];
        }
      }
    }
    my $status = close $stat;
    if ($?) {
      print STDERR "Stat command failed: $statcmd\n" . ": $lastline \n";
      next snipper;
    }
    if ($size == 0) {
      print STDERR "?Warning! Zero-size audio file for $iix '$thing'\n";
    }

    if ($vol > 20) {
      ## unreasonable volume, happens with 'silent' files
      $vol = 0;
    }
    printf("%s %6.3f %6d '%s'\n", $iix, $vol, $size, $thing);
    my $subsize = int($size/2);
    printf $index ("%-45s %10d %10d\n", $thing, $where, $subsize);
    $where += $subsize;

    my $volume_cmd = sprintf("sox -v %6.3f %s.wav %s.raw",
                $vol*0.9, $xfn, $xfn);
    ########## print "+ $volume_cmd\n";
    if (1) {
      my $vol_handle = Symbol::gensym;
      open ($vol_handle, '|-', $volume_cmd)
            || die "Couldn't open pipe to command '$volume_cmd'\n";

      close $vol_handle;
      if ($?) {
        die "Volume command failed: $statcmd\n" . ": $lastline";
      }
    }
    push @todo, "$xfn.raw";
  }
  close $index;

  my $cat_cmd = "cat " . join(' ', @todo) . " > ./snip/everything.raw";
  my $cat_handle = Symbol::gensym;
  open ($cat_handle, '|-', $cat_cmd)
        || die "Couldn't open pipe to command '$cat_cmd'\n";
  close $cat_handle;
  if ($?) {
    die "Cat command failed: $cat_cmd";
  }

  ## Convert RAW to WAVE format
  my $wav_cmd = "sox --rate $sample_rate --bits $bits_sample"
   . " --encoding signed-integer"
   . " ./snip/everything.raw --rate 8000 --bits $out_bits_sample $out_wav";

  my $wav_handle = Symbol::gensym;
  open ($wav_handle, '|-', $wav_cmd)
        || die "Couldn't open pipe to command '$wav_cmd'\n";
  close $wav_handle;
  if ($?) {
    die ".wav command failed: $wav_cmd";
  }

  ## Compress WAVE file
  my $gz_cmd = "gzip -f $out_wav";
  my $gz_handle = Symbol::gensym;
  open ($gz_handle, '|-', $gz_cmd)
        || die "Couldn't open pipe to command '$gz_cmd'\n";
  close $gz_handle;
  system("rm snip/*; rmdir snip");
}
