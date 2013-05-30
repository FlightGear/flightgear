#!/usr/bin/perl

$IN = $ARGV[0];
$OUT = $ARGV[1];

open (RAW, "sox $IN -t .sw -|");

open (OUTF, ">$OUT");

$var = $OUT;
$var =~ s/\.c$//;

print OUTF <<EOM;
/* Converted sound file, from $IN
 * sound is 16 bit signed samples, at 8khz.
 */

short $var \[\] \= \{ 
EOM

printf(OUTF "\t");	
$n = 0;

while(sysread(RAW,$word,2))
{
	#print "WORD is $word";
	printf(OUTF "0x%04hx, ", unpack("s",$word));	
	if((++$n) % 8 == 0) {
		printf(OUTF "\n\t");	
	}
}

printf(OUTF "0 \n};\n");

close OUTF;


