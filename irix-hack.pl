#!/usr/bin/perl

$file = shift(@ARGV);

print "Fixing $file\n";

open(IN, "<$file") || die "cannot open $file for reading\n";
open(OUT, ">$file.new") || die "cannot open $file.new for writting\n";

while (<IN>) {
    s/^AR = ar$/AR = CC -ar/;
    s/\$\(AR\) cru /\$\(AR\) -o /;
    print OUT $_;
}

rename("$file.new", "$file") || die "cannot rename $file.new to $file\n";
