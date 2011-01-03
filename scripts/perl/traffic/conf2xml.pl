#!/usr/bin/perl -w

sub parseTime {
  # print "Parsing time @_\n";
  #die;
  my $timeStr = $_[0];
  @timeArray = split(":", $timeStr);
  # print STDERR "TimeArray: @timeArray\n";
  return ($timeArray[0] + $timeArray[1]/60.0);
}

  sub writeFlight {
  print XMLFILE "  <flight>\n";
  print XMLFILE "    <callsign>$_[0]</callsign>\n";
  print XMLFILE "    <required-aircraft>$_[1]</required-aircraft>\n";
  print XMLFILE "    <fltrules>$_[2]</fltrules>\n";
  print XMLFILE "    <departure>\n";
  print XMLFILE "      <port>$_[3]</port>\n";
  if ($_[4] =~ /[0-6]/) { print XMLFILE "      <time>$_[4]/$_[5]:00</time>\n" }
  else { print XMLFILE "      <time>$_[5]:00</time>\n" };
  print XMLFILE "    </departure>\n";
  print XMLFILE "    <cruise-alt>$_[6]</cruise-alt>\n";
  print XMLFILE "    <arrival>\n";
  print XMLFILE "      <port>$_[7]</port>\n";
  if ($_[8] =~ /[0-6]/) { print XMLFILE "      <time>$_[8]/$_[9]:00</time>\n" }
  else { print XMLFILE "      <time>$_[9]:00</time>\n" };
  print XMLFILE "    </arrival>\n";
  if (($_[4] =~ /[0-6]/) && ($_[8] =~ /[0-6]/)) { print XMLFILE "    <repeat>WEEK</repeat>\n" }
  else { print XMLFILE "    <repeat>24Hr</repeat>\n" };
  print XMLFILE "  </flight>\n";
  return;
}

@inputfiles = glob("???.conf");
while ($infile = shift(@inputfiles)) {
  open (CONF, $infile) or die "Unable to open input configuration file";
  ($outname = $infile) =~ s/conf/xml/;
  print "Opening $outname\n";
  open XMLFILE, ">$outname";
  while ($buf = readline(CONF)) {
    push @DataList, $buf;
  }
  close (CONF);
  print XMLFILE "<?xml version=\"1.0\"?>\n";
  print XMLFILE "<trafficlist>\n";
  while ($dataline = shift(@DataList)) {
    # print STDERR "Dataline: $dataline\n";
    @token = split(" ", $dataline);
    if (scalar(@token) > 0) {
     #  print STDERR "Token: @token\n";
      if ($token[0] eq "AC")
      {
        print XMLFILE "  <aircraft>\n";
        print XMLFILE "    <model>$token[12]</model>\n";
        print XMLFILE "    <livery>$token[6]</livery>\n";
        print XMLFILE "    <airline>$token[5]</airline>\n";
        print XMLFILE "    <home-port>$token[1]</home-port>\n";
        print XMLFILE "    <required-aircraft>$token[3]$token[5]</required-aircraft>\n";
        print XMLFILE "    <actype>$token[4]</actype>\n";
        print XMLFILE "    <offset>$token[7]</offset>\n";
        print XMLFILE "    <radius>$token[8]</radius>\n";
        print XMLFILE "    <flighttype>$token[9]</flighttype>\n";
        print XMLFILE "    <performance-class>$token[10]</performance-class>\n";
        print XMLFILE "    <registration>$token[2]</registration>\n";
        print XMLFILE "    <heavy>$token[11]</heavy>\n";
        print XMLFILE "  </aircraft>\n";
      }
      if ($token[0] eq "FLIGHT") {
        $weekdays = $token[3];
        if (!(($weekdays =~ /^(0|\.)?(1|\.)?(2|\.)?(3|\.)?(4|\.)?(5|\.)?(6|\.)?$/) || ($weekdays eq "DAILY"))) {
          die "Syntax Error! Check days $weekdays for flight no. $token[1]!\n";
        }
        if ($token[4] !~ /^(([0-1]?[0-9])|([2][0-3])):([0-5]?[0-9])$/) {
          die "Syntax Error! Check departure time $token[4] for flight no. $token[1]!\n"
        }
        if ($token[6] !~ /^(([0-1]?[0-9])|([2][0-3])):([0-5]?[0-9])$/) {
          die "Syntax Error! Check arrival time $token[6] for flight no. $token[1]!\n"
        }
        # print STDERR "Weekdays: $weekdays\n";
        if ($weekdays =~ /^(0|\.)?(1|\.)?(2|\.)?(3|\.)?(4|\.)?(5|\.)?(6|\.)?$/) {
          # print STDERR "Weekly for flight no. $token[1]\n";
          # print STDERR "Day: $_\n";
          @day = split(//, $weekdays);
          foreach (@day) {
            if ($_ eq "\.") {
              next;
            } else {
              $depTime = parseTime($token[4]);
              # print STDERR "depTime: $depTime\n";
              $arrTime = parseTime($token[6]);
              # print STDERR "arrTime: $arrTime\n";
              $depDay = $_ + 1;
              if ($depDay > 6) { $depDay = 0 };
              $arrDay = $depDay;
              if ($depTime > $arrTime) { $arrDay++ };
              if ($arrDay > 6) { $arrDay = 0 };
              # print STDERR "depDay: $depDay, arrDay: $arrDay\n";
              writeFlight ($token[1], $token[9], $token[2], $token[5], $depDay, $token[4], $token[8], $token[7], $arrDay, $token[6]);
            }
          }
        }
        elsif ($weekdays eq "DAILY") {
          # print STDERR  "Daily flights for flight no. $token[1]\n";
          $depTime = parseTime($token[4]);
          $arrTime = parseTime($token[6]);
          writeFlight ($token[1], $token[9], $token[2], $token[5], 7, $token[4], $token[8], $token[7], 7, $token[6]);
        }
        else {
          die "System Error! Can't find days to place a flight!\n";
        }
      }
    }
  }
  print XMLFILE "</trafficlist>\n";
  close XMLFILE;
#  print "Closing $outname\n";
}
