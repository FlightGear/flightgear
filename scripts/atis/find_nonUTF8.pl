#! /usr/bin/perl

my($content, $length);

open(FILE, "< atis.list") || die "Unable to open file small. <$!>\n";

while( chomp($content = <FILE>) ) {
    $length = length($content);
    
    for( $i = 0; $i < $length; $i++ ) {
     
        if( ord(substr($content, $i, 1)) > 127 )
        {
            print "$content\n";
            last;
        }        
    }
}
close(FILE);

exit 0
