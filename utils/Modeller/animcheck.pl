#!/usr/bin/perl -w

# Quick & dirty script to find booboos in FlightGear animation files
# May 2004 Joshua Babcock jbabcock@atlantech.net

use strict;

if ( $#ARGV != 1 ) {
    print "Usage: $0 <xml file> <ac3d file>\n";
    exit;
}

my %Objects;
my %References;
my $Key;
my $Num;
my $First;
my $ObjCount=0;
my $CheckForm=1;

# Put whatever you want in here to check for poorly formatted object names.
sub CheckForm {
    my $Bad=0;
    $_[0] !~ /^[A-Z].*/ && ($Bad=1);
    $_[0] =~ /\W/ && ($Bad=1);
    print "$_[0] poorly formatted\n" if $Bad;
}

# Make a hash of all the object names in the AC3D file.
open (AC3D, $ARGV[1]) or die "Could not open $ARGV[1]";
while (<AC3D>) {
    /^name \"(.*)\"/ && ($Objects{$1}+=1);
}
close AC3D;

# Check for duplicates and proper format.
foreach $Key (keys %Objects) {
    print "$Objects{$Key} instances of $1\n" if ($Objects{$Key}>1);
    &CheckForm($Key) if $CheckForm;
    ++$ObjCount;
}
print "$ObjCount objects found.\n\n";

# Make a hash of objects in the XML file that do not reference an object in the AC3D file.
open (XML, $ARGV[0]) or die "Could not open $ARGV[0]";
while (<XML>) {
    if (m|<object-name>(.*)</object-name>| && ! exists($Objects{$1})) {
        # voodoo, "Perl Cookbook", p140
        push( @{$References{$1}}, $.);
    }
}
close XML;

# List all the bad referencees.
foreach $Key (keys %References) {
    $First=1;
    print "Non-existant object $Key at line";
    print "s" if (scalar( @{$References{$Key}}) > 1);
    print ":";
    foreach $Num (@{$References{$Key}}) {
        $First ? ($First=0) : (print ",");
        print " $Num"
    }
    print "\n";
}

exit 0;

