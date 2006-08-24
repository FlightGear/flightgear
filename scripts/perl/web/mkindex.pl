#!/usr/bin/perl

#
# adapted from a script by Bob Hain, 11/30/99
#

#
# default values
#

$outfile = "new.index.html";


#
# process arguments
#

$use_large = 1;
while ( $arg = shift @ARGV ) {
    if ( $arg eq "--large" ) {
	$use_large = 1;
    }
    if ( $arg eq "--outfile" ) {
        $outfile = shift @ARGV;
    }
}


#
# Generate all images
#

$src = "Source";
$ldir = "Large";
$sdir = "Small";
$mdir = "Movies";

$columns = 2;

$swidth = 320;
$sheight = 233;

$lwidth = 1024;
$lheight = 768;


#
# Make sure directories exist
#

if ( ! -e $ldir ) {
    mkdir $ldir, 0755;
}

if ( ! -e $sdir ) {
    mkdir $sdir, 0755;
}

# return 1 if file1 is newer than rile2
sub is_newer {
    my($file1, $file2) = @_;
    # print " - $file1 - $file2 - \n";

    ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime1, 
     $ctime, $blksize, $blocks) = stat($file1);
    ($dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime2, 
     $ctime, $blksize, $blocks) = stat($file2);

    if ( $mtime1 > $mtime2 ) {
	return 1;
    } else {
	return 0;
    }
}


#
# Make images (both large and small)
#

@FILES = `ls $src/*.jpg $src/*.JPG $src/*.png`;

foreach $file ( @FILES ) {
    chop $file;
    $file =~ s/$src\///;

    if ( is_newer( "$src/$file", "$ldir/$file" ) || ! -e "$ldir/$file" ) {
	print "Updating $ldir/$file\n";
	system("cp -f $src/$file $ldir");
	system("mogrify -geometry \'$lwidth" . "X" .
	       "$lheight>\' -interlace LINE -quality 80 $ldir/$file");
    }

    if ( is_newer( "$ldir/$file", "$sdir/$file" ) || ! -e "$sdir/$file" ) {
	print "Updating $sdir/$file\n";
	system("cp -f $ldir/$file $sdir");
	system("mogrify -geometry \'$swidth" . "X" . 
	       "$sheight>\' -interlace LINE -quality 80 $sdir/$file");
    }
}


#
# Check for large and small images to remove
#

@FILES = `ls $ldir`;
foreach $file ( @FILES ) {
    chop($file);
    # print "$file\n";
    if ( ! -f "$src/$file" ) {
	print "No matching src file - deleting large image $file ...\n";
	unlink( "$ldir/$file" );
    }
}

@FILES = `ls $sdir`;
foreach $file ( @FILES ) {
    chop($file);
    if ( ! -f "$src/$file" ) {
	print "No matching src file - deleting small image $file ...\n";
	unlink( "$sdir/$file" );
    }
}


#
# Build image list (for next/previous/first/last links)
#

open( MASTER, "<master.idx" );

@imagelist = ();

while ( <MASTER> ) {
    chop;
    if ( m/\.jpg$/ || m/\.JPG$/ || m/\.png$/ ) {
	push @imagelist, $_;
    } else {
	# just ignore everything else
    }
}

close( MASTER );


#
# Prepair $link subdirectory
#

$link = "Link";
system("rm -rf $link");
mkdir $link, 0755;


#
# Assemble index.html
#

$dir = `pwd`;
chop($dir);
$title = `basename $dir`;
chop($title);

open( MASTER, "<master.idx" );
open( OUT, ">$outfile" );

$j = 1;
$in_table = 0;

while ( <MASTER> ) {
    chop;
    if ( m/^#/ ) {
	# ignore comments
    } elsif ( m/\.txt$/ ) {
	# insert text

	if ( $in_table ) {
	    $in_table = 0;
	    print OUT "</TR>\n";
	    print OUT "</TABLE>\n";
	}

	$file = $_;
	open( IN, "<$src/$file" );
	while ( <IN> ) {
	    print OUT $_;
	}
	close( IN );
	print OUT "<P>\n";
	$j = 1;
	$in_table = 0;
    } elsif ( m/\.jpg$/ || m/\.JPG$/ || m/\.png$/ ) {
	# insert image in 3 wide tables

	$in_table = 1;
	$file = $_;

	$i = `basename $file`;
	chop($i);

	if ( $j == 1 ) {
	    print OUT "<!-- Begin Row -->\n";
	    print OUT "<TABLE ALIGN=CENTER>\n";
	    print OUT "<TR VALIGN=TOP>\n";
	}

        if ( $i =~ m/\.jpg$/ ) {
	    $linkname = `basename $i .jpg`;
        } elsif ( $i =~ m/\.JPG$/ ) {
	    $linkname = `basename $i .JPG`;
        } elsif ($i =~ m/\.png$/ ) {
	    $linkname = `basename $i .png`;
        }
	chop($linkname);

	$thumbinfo = `identify $sdir/$i`;
	($name, $type, $geom, $junk) = split(/\s+/, $thumbinfo, 4);
	($twidth, $theight) = split(/x/, $geom);
	$theight =~ s/\+.*$//;

	# print OUT "<TD WIDTH=220 HEIGHT=160>\n";
	print OUT "<TD WIDTH=$twidth HEIGHT=$sheight>\n";

	print OUT "<A HREF=\"$link/$linkname.html\">";
	print OUT "<IMG WIDTH=$twidth HEIGHT=$theight SRC=\"$sdir/$i\" ALT=\"$linkname\">";
	print OUT "</A><BR>\n";

	if ( -f "$src/$linkname.txt" ) {
	    print OUT "<FONT SIZE=-1 id=\"fgfs\">\n";
	    open( IN, "<$src/$linkname.txt" );
	    while ( <IN> ) {
		print OUT $_;
	    }
	    close( IN );
	    print OUT "</FONT>\n";
	} else {
	    print OUT "<FONT SIZE=-1 id=\"fgfs\">\n";
	    print OUT "$linkname\n";
	    print OUT "</FONT>\n";
	}

	print OUT "</TD>\n";

	if ( $j == $columns ) {
	    $in_table = 0;
	    print OUT "</TR>\n";
	    print OUT "</TABLE>\n";
	}

	if ( ++$j > $columns ) {
	    $j = 1;
	}
    } elsif ( m/\.AVI$/ || m/\.mpg$/ || m/\.mov$/ ) {
	# insert image in 3 wide tables

	$in_table = 1;
	$file = $_;

	$i = `basename $file`;
	chop($i);

	if ( $j == 1 ) {
	    print OUT "<!-- Begin Row -->\n";
	    print OUT "<TABLE ALIGN=CENTER>\n";
	    print OUT "<TR VALIGN=TOP>\n";
	}

        if ( $i =~ m/\.AVI$/ ) {
	    $linkname = `basename $i .AVI`;
        } elsif ( $i =~ m/\.mpg$/ ) {
	    $linkname = `basename $i .mpg`;
        } elsif ( $i =~ m/\.mov$/ ) {
	    $linkname = `basename $i .mov`;
        } else {
	    die "unknown movie type\n";
        }
	chop($linkname);
	# print OUT "<TD WIDTH=220 HEIGHT=160>\n";
	print OUT "<TD WIDTH=$swidth HEIGHT=$sheight>\n";

	$thumbinfo = `identify $mdir/$linkname.jpg`;
	($name, $type, $geom, $junk) = split(/\s+/, $thumbinfo, 4);
	$geom =~ s/\+.*//;
	($twidth, $theight) = split(/x/, $geom);
        print "movie thumb geom = $geom  $twidth  $theight\n";

	print OUT "<A HREF=\"$mdir/$i\">";
	if ( -f "$mdir/$linkname.jpg" ) {
	    print OUT "<IMG WIDTH=$twidth HEIGHT=$theight SRC=\"$mdir/$linkname.jpg\" ALT=\"$linkname\">";
	} else {
	    print OUT "$linkname";
	}
	print OUT "</A>\n";

	if ( -f "$mdir/$linkname.txt" ) {
            print OUT "<BR>\n";
	    print OUT "<FONT SIZE=-1 id=\"fgfs\">\n";
	    open( IN, "<$mdir/$linkname.txt" );
	    while ( <IN> ) {
		print OUT $_;
	    }
	    close( IN );
	    print OUT "</FONT>\n";
	} else {
            print OUT "<BR>\n";
	    print OUT "<FONT SIZE=-1 id=\"fgfs\">\n";
	    print OUT "$linkname\n";
	    print OUT "</FONT>\n";
        }

	print OUT "</TD>\n";

	if ( $j == $columns ) {
	    $in_table = 0;
	    print OUT "</TR>\n";
	    print OUT "</TABLE>\n";
	}

	if ( ++$j > $columns ) {
	    $j = 1;
	}
    } else {
	# just pass along the rest as is

	$j = 1;

	if ( $in_table ) {
	    $in_table = 0;
	    print OUT "</TR>\n";
	    print OUT "</TABLE>\n";
	}
	print OUT "$_\n";
    }
}


#
# Generate Links
#

# @FILES = `ls $src/*.jpg $src/*.JPG $src/*.png`;

$first = $imagelist[0];
if ( $first =~ m/\.jpg$/ ) {
    # print "  ext = jpg\n";
    $firstname = `basename $first .jpg`;
} elsif ( $first =~ m/\.JPG$/ ) {
    # print "  ext = JPG\n";
    $firstname = `basename $first .JPG`;
} else {
    # print "  ext = png\n";
    $firstname = `basename $first .png`;
}
chop($firstname);

$last = $imagelist[$#imagelist];
if ( $last =~ m/\.jpg$/ ) {
    # print "  ext = jpg\n";
    $lastname = `basename $last .jpg`;
} elsif ( $last =~ m/\.JPG$/ ) {
    # print "  ext = JPG\n";
    $lastname = `basename $last .JPG`;
} else {
    # print "  ext = png\n";
    $lastname = `basename $last .png`;
}
chop($lastname);

for ($i = 0; $i <= $#imagelist; $i++) {
    $file = $imagelist[$i];
    # print "'$file'\n";

    if ( $i > 0 ) {
	$prev = $imagelist[$i - 1];
    } else {
	$prev = "null";
    }

    if ( $i < $#imagelist ) {
	$next = $imagelist[$i + 1];
    } else {
	$next = "null";
    }

    if ( $file =~ m/\.jpg$/ ) {
        $linkname = `basename $file .jpg`;
        $ext = "jpg";
    } elsif ( $file =~ m/\.JPG$/ ) {
        $linkname = `basename $file .JPG`;
        $ext = "JPG";
    } else {
        $linkname = `basename $file .png`;
        $ext = "png";
    }
    chop($linkname);
    $nice_name = $linkname;
    $nice_name =~ s/\_/ /g;

    if ( $prev =~ m/\.jpg$/ ) {
	# print "  ext = jpg\n";
        $prevname = `basename $prev .jpg`;
    } elsif ( $prev =~ m/\.JPG$/ ) {
	# print "  ext = JPG\n";
        $prevname = `basename $prev .JPG`;
    } else {
	# print "  ext = png\n";
        $prevname = `basename $prev .png`;
    }
    chop($prevname);
    
    if ( $next =~ m/\.jpg$/ ) {
        $nextname = `basename $next .jpg`;
    } elsif ( $next =~ m/\.JPG$/ ) {
        $nextname = `basename $next .JPG`;
    } else {
        $nextname = `basename $next .png`;
    }
    chop($nextname);
    
    $outfile = "$link/$linkname.html";

    open( OUT, ">$outfile" );
    print OUT <<EOF;

<HTML>

<HEAD>
  <TITLE>$linkname.$ext</TITLE>
</HEAD>

<!-- <BODY BGCOLOR="#000000" ALINK="#000000" VLINK="#000000" LINK="#000000" TEXT="#FFFFFF"> -->
<BODY>

<A HREF="$firstname.html">[First]</A>
EOF

    if ( $prevname ne "null" ) {
	print OUT "<A HREF=\"$prevname.html\">[Previous]</A>\n";
    } else {
	print OUT "[Previous]\n";
    }

    if ( $nextname ne "null" ) {
	print OUT "<A HREF=\"$nextname.html\">[Next]</A>\n";
    } else {
	print OUT "[Next]\n";
    }

    print OUT <<EOF;
<A HREF="$lastname.html">[Last]</A>

<TABLE ALIGN=CENTER>
  <TR>
  <TD ALIGN=CENTER>
  <FONT SIZE=+1>
  $nice_name
  </FONT>
  </TD>
  </TR>

  <TR>
  <TD ALIGN=CENTER>
  <FONT SIZE=-1>
EOF

    if ( -f "$src/$linkname.txt" ) {
	# print OUT "<BR>\n";
	open( IN, "<$src/$linkname.txt" );
	while ( <IN> ) {
	    print OUT $_;
	}
	close( IN );
    }

    print OUT <<EOF;
  </FONT>
  </TD>
  </TR>
 
</TABLE>

Click on the image for the full size version.
 
<TABLE ALIGN=CENTER>
  <TR>
  <TD ALIGN=CENTER>
  <A HREF="../$src/$linkname.$ext">
EOF

    if ( $use_large ) {
	print OUT "  <IMG SRC=\"../$ldir/$linkname.$ext\">\n";
    } else {
  	print OUT "  <IMG SRC=\"../$src/$linkname.$ext\">\n";
    }

    print OUT <<EOF;
  </TD>
  </A>
  </TR>
</TABLE>

</BODY>
</HTML>
EOF

    close( OUT );
}

