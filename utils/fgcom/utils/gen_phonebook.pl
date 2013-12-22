#!/usr/bin/perl -w
#
# gen_phonebook.pl
# Version 1.04 - 26 Sept 2013
# Version 1.03 - 04 Sept 2013
# Version 1.02 - 01 Sept 2013
#
# Modified by Clément de l'Hamaide - 20130926
#  * Disable phonebook.txt generation
#
# Modified by Geoff R. McLane - 20130904
#  * Added strict and warnings, fixing all variables to comply
#  * Added command line interface and 'help', but kept same defaults
#  * Removed numerous substr($txt,0,-1) which truncated some string
#  * Change 'chop' to the safer 'chomp'
#  * Added heliport(16) and seaport(17) under an option switch
#  * Added output of some stats of what was collected, and skipped
#  * Added $trim_to_20 option to align the phonebook if desired
#  * Replaced all tabs with '    ' to better align the code for readability
#  * Added a trim_all($txt) to removed some unwanted line endings
#  * Is compatible with FG 810 AND XP 1000 apt.dat files
#
# Modified by Clement de l'Hamaide - 20130901
#
use IO::Zlib;
use IO::File;
use File::Slurp;
use Data::Dumper;
use DateTime;
use Encode;
use strict;
use warnings;

my $VERS = "1.04 - 26 Sept 2013";
# default locations
my $FG_AIRPORTS = "./apt.dat.gz";
my $FG_NAVAIDS  = "./nav.dat.gz";
my $out_fil1  = "fgcom.conf";
my $out_pos   = "positions.txt";
#my $out_phon  = "phonebook.txt";

# options
my $trim_to_20 = 0;
my $add_heliports = 1;
my $add_seaports = 1;

my $phonebook_post="ZZZZ                        910.000  0190909090910000  Echo-Box
ZZZZ                        911.000  0190909090911000  Music-Box
ZZZZ                        700.000  0190909090700000  Radio-Box
ZZZZ                        123.450  0190909090123450  Air2Air
ZZZZ                        122.750  0190909090122750  Air2Air
ZZZZ                        121.500  0190909090121500  Air2Air
ZZZZ                        123.500  0190909090123500  Air2Air
ZZZZ                        121.000  0190909090121000  Emergency
ZZZZ                        723.340  0190909090723340  French Air Patrol
";

my $extensions_pre="[globals]
ATIS_RECORDINGS=/var/fgcom-server/atis
RADIO_FILE=/var/fgcom-server/radio
;Morse tone, 1020Hz for VOR and ILS but 1350Hz for DME, in Hz
MORSETONE=1020
;Dit length for morse code, in ms
;MORSEDITLEN=300

[macro-com]
exten => s,1,Answer()
exten => s,n,NoCDR
exten => s,n,MeetMe(\${MACRO_EXTEN},qd)
exten => s,n,Hangup()

[macro-echo]
exten => s,1,Answer()
exten => s,n,NoCDR
exten => s,n,Echo()
exten => s,n,Hangup()

[macro-atis]
exten => s,1,Answer()
; Check if audio file exists
exten => s,n,TrySystem(ls \${ATIS_RECORDINGS}/99\${MACRO_EXTEN:2}*)
exten => s,n,Goto(\${SYSTEMSTATUS})
; If audio file exists, play it
exten => s,n(SUCCESS),While(\$[1])
exten => s,n,Playback(\${ATIS_RECORDINGS}/99\${MACRO_EXTEN:2})
exten => s,n,Wait(3)
exten => s,n,EndWhile
exten => s,n,Hangup()
; If audio doesn't exist or TrySystem failed (why?), go to festival macro
;exten => s,n,(APPERROR),Macro(festival, \${MACRO_EXTEN}) ; DISABLED FOR NOW
;exten => s,n,(FAILURE),Macro(festival, \${MACRO_EXTEN}) ; DISABLED FOR NOW
exten => s,n,(APPERROR),Hangup()
exten => s,n,(FAILURE),Hangup()

[macro-record-atis]
exten => s,1,Answer()
exten => s,n,SendText(Record begin in 3s)
exten => s,n,Wait(1)
exten => s,n,SendText(Record begin in 2s)
exten => s,n,Wait(1)
exten => s,n,SendText(Record begin in 1s)
exten => s,n,Wait(1)
exten => s,n,Record(\${ATIS_RECORDINGS}/\${MACRO_EXTEN}:gsm,,90,k)
exten => s,n,Wait(2)
exten => s,n,Playback(\${ATIS_RECORDINGS}/\${MACRO_EXTEN})
exten => s,n,Hangup()

[macro-festival]
exten => s,1,Set(metar=\"\${SHELL(getmetar \$[1]):0:-1}\");
exten => s,n,While(\$[1])
exten => s,n,Festival(\${metar:1:-1})
exten => s,n,Wait(3)
exten => s,n,EndWhile

[macro-vor]
exten => s,1,Answer()
exten => s,n,While(\$[1])
exten => s,n,Morsecode(\${ARG1})
exten => s,n,Wait(2)
exten => s,n,EndWhile
exten => s,n,Hangup()

[macro-radio]
exten => s,1,Answer()
exten => s,n,Playback(\${RADIO_FILE})
exten => s,n,Hangup()

[fgcom]
; 910.000     Echo-Box
exten => 0190909090910000,1,SendText(Echo Box - For testing FGCOM)
exten => 0190909090910000,n,Macro(echo)
; 911.000     Music-Box
exten => 0190909090911000,1,Answer
exten => 0190909090911000,n,SendText(Music On Hold Box - For testing FGCOM)
exten => 0190909090911000,n,MusicOnHold(default)
; Radio Station: 700.000 MHz
exten => 0190909090700000,1,SendText(Radio Station - For testing FGCOM)
exten => 0190909090700000,n,Macro(radio)

; 121.500     Air2Air
exten => 0190909090121500,1,SendText(121.500 Auto-information frequency)
exten => 0190909090121500,n,Macro(com)
; 123.450     Air2Air
exten => 0190909090123450,1,SendText(123.450 Auto-information frequency)
exten => 0190909090123450,n,Macro(com)
; 123.500     Air2Air
exten => 0190909090123500,1,SendText(123.500 Auto-information frequency)
exten => 0190909090123500,n,Macro(com)
; 122.750     Air2Air
exten => 0190909090122750,1,SendText(122.750 Auto-information frequency)
exten => 0190909090122750,n,Macro(com)

; 121.000     emergency
exten => 0190909090121000,1,SendText(121.000 Emergency frequency)
exten => 0190909090121000,n,Macro(com)

; 723.340     Franch Air Patrol
exten => 0190909090723340,1,SendText(723.340 French Air Patrol frequency)
exten => 0190909090723340,n,Macro(com)

";

my $dt = DateTime->now;
my $pgmname = $0;
if ($pgmname =~ /(\\|\/)/) {
    my @tmpsp = split(/(\\|\/)/,$pgmname);
    $pgmname = $tmpsp[-1];
}
my $numberOfFrequenciesParsed = 0;
my $numberOfFrequenciesComputed = 0;
my $numberOfFrequenciesWritten = 0;
my $numberOfFrequenciesSkipped = 0;
my %APT = ();
my %NAV = ();
my %frq = ();
my ($fh,$z,$icao,$lat,$lon,$latW,$lonW,$com,$code,$text);
my ($vor,$icao_number,$f,$positions,$extensions,$phonebook);
my ($i,$tmp,$airport,$nav,$freq,$ssf,$type);

my $land_apcnt = 0;
my $heli_apcnt = 0;
my $sea_apcnt = 0;
my $no_latloncnt = 0;
my $no_freqcnt = 0;
my $airportcount = 0;
my $navlinecount = 0;
my $vordmeadded = 0;

sub prt($) { print shift; }

sub icao2number($) {
    my ($icao) = @_;
    my ($number,$n);
    $icao = " ".$icao while(length($icao) < 4);
    $number = '';
    for ($i = 0; $i< length($icao); $i++) {
        $n = ord(substr($icao,$i,1));
        $number .= sprintf("%02d",$n);
    }
    return($number);
}

sub trim_tailing($) {
    my ($ln) = shift;
	$ln = substr($ln,0, length($ln) - 1) while ($ln =~ /\s$/g); # remove all TRAILING space
    return $ln;
}

sub trim_leading($) {
    my ($ln) = shift;
	$ln = substr($ln,1) while ($ln =~ /^\s/); # remove all LEADING space
    return $ln;
}

sub trim_ends($) {
    my ($ln) = shift;
    $ln = trim_tailing($ln); # remove all TRAINING space
	$ln = trim_leading($ln); # remove all LEADING space
    return $ln;
}

sub trim_all {
	my ($ln) = shift;
	$ln =~ s/\n/ /gm;	# replace CR (\n)
	$ln =~ s/\r/ /gm;	# replace LF (\r)
	$ln =~ s/\t/ /g;	# TAB(s) to a SPACE
    $ln = trim_ends($ln);
	$ln =~ s/\s{2}/ /g while ($ln =~ /\s{2}/);	# all double space to SINGLE
	return $ln;
}

##############################################################################
# Main program
##############################################################################
parse_args(@ARGV);
# read airport data in hash
$fh = new IO::Zlib;
if ($fh->open($FG_AIRPORTS, "r")) {
    my $hadlines = 0;
    printf("Parsing ".$FG_AIRPORTS." ...\n");
    while ($z = <$fh>) {
        chomp($z);
        if ($z =~ /^\s*$/) {
            if ($icao) {
                if (scalar(keys(%frq)) > 0) {
                    if (!$lat && !$lon) {
                        if ($latW && $lonW) {
                            $lat = $latW;
                            $lon = $lonW;
                        } else {
                            print($icao." :: LAT/LON not found\n");
                        }
                    }
                    if ($lat && $lon) {
                        $APT{$icao}{'text'} = $text;
                        $APT{$icao}{'lat'}  = $lat;
                        $APT{$icao}{'lon'}  = $lon;
                        foreach $f (keys(%frq)) {
                            if (length($frq{$f}) <= 1) {
                                $frq{$f} = "TWR";
                            }
                            $APT{$icao}{'com'}{$f} = $frq{$f};
                        }
                        $airportcount++;
                    } else {
                        $no_latloncnt++;
                    }
                } else {
                    $no_freqcnt++;
                }
            } elsif ($hadlines) {
                $numberOfFrequenciesSkipped++;
            }

            undef($icao);
            undef($text);
            undef($lon);
            undef($lat);
            undef($com);
            %frq=();
            $hadlines = 0;
            next;
        }
        elsif ($z=~/^1\s+-?\d+\s+[01]\s+[01]\s+([A-Z0-9]+)\s+(.+)\s*$/)
        {
            # Airport Header
            $icao = $1;
            $text = trim_all($2);
            $land_apcnt++;
        }
        elsif ($z=~/^16\s+\d+\s+[01]\s+[01]\s+([A-Z0-9]+)\s+(.+)\s*$/)
        {
            $heli_apcnt++;
            # Heliport Header
            if ($add_seaports) {
                $icao = $1;
                $text = trim_all($2);
            }
        }
        elsif ($z=~/^17\s+\d+\s+[01]\s+[01]\s+([A-Z0-9]+)\s+(.+)\s*$/)
        {
            # Seaport Header
            $sea_apcnt++;
            if ($add_seaports) {
                $icao = $1;
                $text = trim_all($2);
            }
        }
        elsif ($z=~/^14\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)/)
        {
            # TWR Position
            $lat=$1;
            $lon=$2;
        }
        elsif ($z=~/^19\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)/)
        {
             # WS Position
             $latW=$1;
             $lonW=$2;
        }
        elsif ($z=~/^5[0-6]\s+(\d{5})\d*\s+(.+)\s*$/)
        {
            # COM data
            $freq = $1;
            $type = trim_all($2);
            ###prt("$freq $type\n");
            $ssf = substr($freq, -1);
            if ( $ssf == 2 || $ssf == 7) {
                $com =  sprintf("%3.2f5", $freq/100);
            } else {
                $com = sprintf("%3.3f", $freq/100);
            }
            $frq{$com} = $type;
        }
        $hadlines++;
    }
}
else
{
    die("Cannot open $FG_AIRPORTS :$!\n");
}
$fh->close;
prt("Got $airportcount airports, $land_apcnt land(1), $heli_apcnt heliports(16), and $sea_apcnt seaports(17), skipped $no_latloncnt no lat/lon, $no_freqcnt no freqs\n");

# read nav data in hash
$nav=new IO::Zlib;
if($nav->open($FG_NAVAIDS, "r"))
{

    printf("Parsing ".$FG_NAVAIDS." ...\n");
    while ($z=<$nav>) {

        chomp($z);
        last if ($z =~ /^99/);
        $navlinecount++;

        if ($z =~ /^3\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+\d+\s+(\d+)\s+\d+\s+-?\d+\.\d+\s+([A-Z]+)\s+(.*)\s*$/) {
            # VOR/DME Nav
            $lat=$1;
            $lon=$2;
            $freq=sprintf("%3.3f",$3/100);
            $code=$4;
            $text = trim_all($5);

            $NAV{$code}{'lat'}=$lat;
            $NAV{$code}{'lon'}=$lon;
            $NAV{$code}{'frq'}=$freq;
            $NAV{$code}{'text'}=$text;
            $vordmeadded++;
        }
    }
}
else
{
    die("Cannot open $FG_NAVAIDS :$!\n");
}

$nav->close;
prt("Done $navlinecount nav.dat lines, adding $vordmeadded VOR/DME(3) records\n");

# get output files open

# open positions file
$positions = new IO::File;
$positions->open($out_pos, "w") || die("Cannot open $out_pos for writing: $!\n");
 
# open phonebook file
#$phonebook = new IO::File;
#$phonebook->open($out_phon, "w") || die("Cannot open $out_phon for writing: $!\n");

# open fgcom.conf
$extensions = new IO::File;
$extensions->open($out_fil1, "w") || die("Cannot open $out_fil1 for writing: $!\n");


#print $phonebook "File generated by $pgmname - ".join ' ', $dt->ymd, $dt->hms." UTC\n";
#print $phonebook keys(%APT)." airports and ".keys(%NAV)." navaids are present in this file (".$numberOfFrequenciesSkipped." freq skipped) \n\n";
#print $phonebook "ICAO  Decription            FRQ      Phone no.          Name\n";
#print $phonebook "-" x 79,"\n";

# read pre data for fgcom.conf;
print $extensions $extensions_pre;

printf("Writing airports data ...\n");

# Print all known airports
foreach $airport (sort(keys(%APT))) {
    foreach $f (keys(%{$APT{$airport}{'com'}})) {
        $ssf = substr($f, -2);
        $type = $APT{$airport}{'com'}{$f};
        if ($trim_to_20 && (length($type) > 20)) {
            $type = substr($type,0,20);
        }
        if ($ssf == 30 || $ssf == 80) {
           next;
           printf("Found a 8.33KHz freq !!! $airport\n");
        }

        $icao_number = icao2number($airport);

        # write positions APT
        print $positions Encode::encode( "utf8", $airport.",".$f.",".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                
        # write phonebook
        #$tmp = sprintf("%4s  %-20s  %3.3f  %-.16s  %-20s\n",$airport,$type,$f,"01".$icao_number.$f*1000,$APT{$airport}{'text'});
        #print $phonebook Encode::encode( "utf8", $tmp);

        # write extensions.conf
        $tmp = "; $airport $type $f - ".$APT{$airport}{'text'}."\n";
        ###prt($tmp);
        print $extensions Encode::encode( "utf8", $tmp);
        if ( $APT{$airport}{'com'}{$f} =~ /ATIS$/ ) {
            # ATIS playback extension
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",1,SendText($airport ".$APT{$airport}{'text'}." $f ".$APT{$airport}{'com'}{$f}.")\n");
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",n,SendURL(http://www.the-airport-guide.com/search.php?by=icao&search=$airport)\n");
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",n,Macro(atis)\n");
            if ($ssf == 00) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005).",".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif ($ssf == 25) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $positions Encode::encode( "utf8", $airport.",".($f-0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000-5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif ($ssf == 50) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005).",".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif($ssf == 75) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $positions Encode::encode( "utf8", $airport.",".($f-0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000-5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            }

            # ATIS record extension
            print $extensions Encode::encode( "utf8", "exten => 99$icao_number".($f*1000).",1,SendText($airport ".$APT{$airport}{'text'}." $f Record-".$APT{$airport}{'com'}{$f}.")\n");
            print $extensions Encode::encode( "utf8", "exten => 99$icao_number".($f*1000).",n,Macro(record-atis)\n");
        }
        else
        {
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",1,SendText($airport ".$APT{$airport}{'text'}." $f ".$APT{$airport}{'com'}{$f}.")\n");
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",n,SendURL(http://www.the-airport-guide.com/search.php?by=icao&search=$airport)\n");
            print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000).",n,Macro(com)\n");
            if ($ssf == 00) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005).",".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif ($ssf == 25) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $positions Encode::encode( "utf8", $airport.",".($f-0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000-5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif ($ssf == 50) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005).",".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            } elsif ($ssf == 75) {
                print $positions Encode::encode( "utf8", $airport.",".($f+0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $positions Encode::encode( "utf8", $airport.",".($f-0.005)."0,".$APT{$airport}{'lat'}.",".$APT{$airport}{'lon'}.",".$APT{$airport}{'com'}{$f}.",".$APT{$airport}{'text'}."\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000-5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
                print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($f*1000+5).",1,Dial(Local/01$icao_number".($f*1000).")\n");
            }

        }

        print $extensions ";\n";

    }
}

printf("Writing navaids data ...\n");
# write VORs to files
foreach $vor (sort(keys(%NAV))) {

    $icao_number = icao2number($vor);

    # write positions NAV
    print $positions Encode::encode( "utf8", $vor.",".$NAV{$vor}{'frq'}.",".$NAV{$vor}{'lat'}.",".$NAV{$vor}{'lon'}.",VOR,".$NAV{$vor}{'text'}."\n");
                
    # write phonebook
    #$tmp = sprintf("%4s  %-20s  %3.3f  %-.16s  %-20s\n",$vor,"",$NAV{$vor}{'frq'},"01".$icao_number.$NAV{$vor}{'frq'}*1000,$NAV{$vor}{'text'});
    #print $phonebook Encode::encode( "utf8", $tmp);

    # write extensions.conf
    print $extensions Encode::encode( "utf8", "; VOR $vor $NAV{$vor}{'frq'} - $NAV{$vor}{'text'}\n");
    print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($NAV{$vor}{'frq'}*1000).",1,SendText($vor ".$NAV{$vor}{'text'}." ".$NAV{$vor}{'frq'}.")\n");
    print $extensions Encode::encode( "utf8", "exten => 01$icao_number".($NAV{$vor}{'frq'}*1000).",n,Macro(vor,$vor)\n");
}

# close positions
$positions->close;

# close extensions
$extensions->close;

# close phonebook
#print $phonebook $phonebook_post;
#$phonebook->close;

prt("Done file outputs... $out_fil1 and $out_pos...\n");
exit(0);

###############################################
sub give_help() {
    prt("\n");
    prt("$pgmname, version $VERS\n");
    prt("Usage:\n");
    prt(" --help    (-h, -?) = This help and exit(2)\n");
    prt("\n");
    prt("Input files:\n");
    prt(" --air <file>  (-a) = Name of airports file. (def=$FG_AIRPORTS)\n");
    prt(" --nav <file>  (-n) = Name of navaids file. (def=$FG_NAVAIDS)\n");
    prt("\n");
    prt("Output files:\n");
    prt(" --conf <file> (-c) = Name of conf file. (def=$out_fil1)\n");
    prt(" --pos <file>  (-p) = Name of position file. (def=$out_pos)\n");
    #prt(" --book <file> (-b) = Name of phonebook file. (def=$out_phon)\n");
    prt("\n");
    prt("Purpose:\n");
    prt("Read the input files, and output the extensions (conf),\n");
    prt("used to configure the asterisk voip server, and output the positions file,\n");
    prt("used by standalone fgcom to establish the location of the caller.\n");
    prt("\n");
    exit(2);
}

sub need_arg {
    my ($arg,@av) = @_;
    die("ERROR: [$arg] must have a following argument!\n") if (!@av);
}

sub parse_args {
    my @av = @_;
    my ($arg,$sarg);
    my $bad = 0;
    while (@av) {
        $arg = $av[0];
        ### prt("ARG [$arg]\n");
        if ($arg =~ /^-/) {
            $sarg = substr($arg,1);
            $sarg = substr($sarg,1) while ($sarg =~ /^-/);
            if ($sarg =~ /^a/) {
                need_arg(@av);
                shift @av;
                $sarg = $av[0];
                $FG_AIRPORTS = $sarg;
            } elsif ($sarg =~ /^n/) {
                need_arg(@av);
                shift @av;
                $sarg = $av[0];
                $FG_NAVAIDS = $sarg;
            } elsif ($sarg =~ /^c/) {
                need_arg(@av);
                shift @av;
                $sarg = $av[0];
                $out_fil1 = $sarg;
            } elsif ($sarg =~ /^p/) {
                need_arg(@av);
                shift @av;
                $sarg = $av[0];
                $out_pos = $sarg;
#            } elsif ($sarg =~ /^b/) {
#                need_arg(@av);
#                shift @av;
#                $sarg = $av[0];
#                $out_phon = $sarg;
            } elsif (($sarg =~ /^h/)||($sarg eq '?')) {
                give_help();
            } else {
                die("ERROR: Unknown argument [$arg]\n");
            }
        } else {
            die("ERROR: Unknown argument [$arg]\n");
        }
        shift @av;
    }
    $arg = '';
    if (! -f $FG_AIRPORTS ) {
        $arg .= "Error: Unable to locate [$FG_AIRPORTS]!\n";
        $bad++;
    }
    if (! -f $FG_NAVAIDS ) {
        $arg .= "Error: Unable to locate [$FG_NAVAIDS]!\n";
        $bad++;
    }
    die("Invalid input file!\n$arg") if ($bad);

}

# eof
