#!/usr/bin/perl -w

#use strict;
#use warnings;

# DEBUG
#  use Data::Dumper;
#  print Dumper($data) . "\n";
# END

if (scalar (@ARGV) == 1) {
    @files = glob("$ARGV[0]");
    print "Processing : ", @files, "\n";
} else {
    die "Usage : conf2xml.pl <inputfile> [ > outputfile ]\n";
}
$file = shift(@files);

use Switch;
use XML::LibXML;
my $parser = XML::LibXML->new();
my $doc = $parser->load_xml(location => $file);
my $data;

# reformatting days
# According to http://wiki.flightgear.org/index.php/Interactive_Traffic
# 0 = Sunday and 6 = saturday
# For convenience we switch here to "classical" numbering
# where 0 = Monday and 6 = sunday
sub parseDay {
  my $day;
  $day = substr($_[0],0,1);
  switch ($day) {
    case 0 {$day="......6"} # Sunday
    case 1 {$day="0......"} # Monday
    case 2 {$day=".1....."} # Tuesday
    case 3 {$day="..2...."} # Wednesday
    case 4 {$day="...3..."} # Thrusday
    case 5 {$day="....4.."} # Friday
    case 6 {$day=".....5."} # Saturday
    else   {$day="0123456"} # Daily
  };
  return $day;
}

# reformatting times
sub parseTime {
  return substr($_[0],2,5);
}

print "# AC Homeport Registration RequiredAC AcTyp Airline Livery Offset Radius Flighttype PerfClass Heavy Model\n";
# get aircraft data
foreach $data ($doc->findnodes('/trafficlist/aircraft')) {
  my $AcMdl = $data->findnodes('./model');
  my $AcLvy = $data->findnodes('./livery');
  my $AcAln = $data->findnodes('./airline');
  my $AcHp = $data->findnodes('./home-port');
  my $AcReq = $data->findnodes('./required-aircraft');
  my $AcTyp = $data->findnodes('./actype');
  my $AcO = $data->findnodes('./offset');
  my $AcRad = $data->findnodes('./radius');
  my $AcFt = $data->findnodes('./flighttype');
  my $AcPrf = $data->findnodes('./performance-class');
  my $AcReg = $data->findnodes('./registration');
  my $AcHvy = $data->findnodes('./heavy');
  print "AC $AcHp $AcReg $AcReq $AcTyp $AcAln $AcLvy $AcO $AcRad $AcFt $AcPrf $AcHvy $AcMdl\n";
}
print "\n# FLIGHT Callsign Flightrule Days DeparTime DepartPort ArrivalTime ArrivalPort Altitude RequiredAc\n# 0 = Monday, 6 = Sunday\n";
# get flight data
foreach $data ($doc->findnodes('/trafficlist/flight')) {
  my $FlRep = $data->findnodes('repeat');
  my $FlDepPrt = $data->findnodes('departure/port');
  my $FlArrPrt = $data->findnodes('arrival/port');
  my $FlCs = $data->findnodes('callsign');
  my $FlFr = $data->findnodes('fltrules');
  my $FlCa = $data->findnodes('cruise-alt');
  my $FlReq = $data->findnodes('required-aircraft');
  my $FlDepDay = $data->findnodes('departure/time');
  my $FlDepTime = $data->findnodes('departure/time');
  my $FlArrDay = $data->findnodes('arrival/time');
  my $FlArrTime = $data->findnodes('arrival/time');
  my $FlDays = ".......";
# handle flights depending on weekly or daily schedule
  if (lc($FlRep) eq "week") {
    $FlDays = parseDay($FlDepTime);
    $FlDepTime = parseTime($FlDepTime);
    $FlArrTime = parseTime($FlArrTime);
  } elsif (lc($FlRep) eq "24hr") {
    $FlDepDay = $data->findnodes('departure/time');
    $FlDepTime = substr($data->findnodes('departure/time'),0,5);
    $FlArrDay = $data->findnodes('arrival/time');
    $FlArrTime = substr($data->findnodes('arrival/time'),0,5);
    $FlDays = "0123456";
  } else {
    die "Error! No proper repetition found in XML!\n";
  }
# output data
  print "FLIGHT $FlCs $FlFr $FlDays $FlDepTime $FlDepPrt $FlArrTime $FlArrPrt $FlCa $FlReq\n";
}
