#!/usr/bin/perl

#---------------------------------------------------------------------------
# Toplevel script to automate DEM file processing and conversion
#
# Written by Curtis Olson, started January 1998.
#
# Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


$| = 1;                         # flush buffers after every write

$do_demfit = 0;
$do_triangle_1 = 0;
$do_fixnode = 0;
$do_splittris = 1;

$do_tri2obj = 0;
$do_strips = 0;
$do_fixobj = 0;


# return the file name root (ending at last ".")
sub file_root {
    my($file) = @_;
    my($pos);

    $pos = rindex($file, ".");
    return substr($file, 0, $pos);
}


# set the FG_ROOT environment variable if it hasn't already been set.
if ( $ENV{FG_ROOT} eq "" ) {
    # look for a file called fgtop as a place marker
    if ( -e "fgtop" ) {
        $ENV{FG_ROOT} = ".";
    } elsif ( -e "../fgtop" ) {
        $ENV{FG_ROOT} = "..";
    }
}


# 1.  Start with file.dem

$dem_file = shift(@ARGV);
$error = shift(@ARGV);
$error += 0.0;

print "Source file = $dem_file  Error tolerance = $error\n";

if ( $error < 0.5 ) {
    die "I doubt you'll be happy with an error tolerance as low as $error.\n";
}

# 2.  dem2node $FG_ROOT dem_file tolerance^2 (meters)
# 
#     - dem2node .. dem_file 160000
#
#     splits dem file into 64 file.node's which contain the
#     irregularly fitted vertices

if ( $do_demfit ) {
    $command = "./Dem2node/demfit $ENV{FG_ROOT} $dem_file $error";

    print "Running '$command'\n";

    open(OUT, "$command |");
    while ( <OUT> ) {
	print $_;
	if ( m/Scenery/ ) {
	    $subdir = $_;
	    $subdir =~ s/Dir = //;
	}
    }
    close(OUT);
}

# 3.  triangle -q file (Takes file.node and produces file.1.node and
#                      file.1.ele)

$subdir = "../Scenery/w120n030/w111n033";
print "Subdirectory for this dem file is $subdir\n";

if ( $do_triangle_1 ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	print $file;
	chop($file);
	if ( ($file =~ m/\.node$/) && ($file !~ m/\.\d\.node$/) ) {
	    $command = "./Triangle/triangle -q $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    # remove input file.node
	    unlink("$subdir/$file");
	}
    }
}

# 4.  fixnode file.dem subdir
#
#     Take the original .dem file (for interpolating Z values) and the
#     subdirecotry containing all the file.1.node's and replace with
#     fixed file.1.node

if ( $do_fixnode ) {
    $command = "./FixNode/fixnode $dem_file $subdir";
    print "Running '$command'\n";
    open(OUT, "$command |");
    while ( <OUT> ) {
	print $_;
    }
    close(OUT);
}


# 4.1 splittris file (.1.node) (.1.ele)

#     Extract the corner, edge, and body vertices (in original
#     geodetic coordinates) and normals (in cartesian coordinates) and
#     save them in something very close to the .obj format as file.se,
#     file.sw, file.nw, file.ne, file.north, file.south, file.east,
#     file.west, and file.body.  This way we can reconstruct the
#     region using consistant edges and corners.  

#     Arbitration rules: If an opposite edge file already exists,
#     don't create our matching edge.  If a corner already exists,
#     don't create ours.  Basically, the early bird gets the worm and
#     gets to define the edge verticies and normals.  All the other
#     adjacent tiles must use these.

if ( $do_splittris ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.node$/ ) {
	    $file =~ s/\.node$//;  # strip off the ".node"
	
	    $command = "./SplitTris/splittris $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	}
    }
}


# 4.2 read in tile sections/ele) skipping edges, read edges out of
#     edge files, save including proper shared edges (as node/ele)
#     files.  If my edge and adjacent edge both exist, use other,
#     delete mine.  If only mine exists, use it.


# 4.3 Retriangulate fixed up files (without -q option)


# 5.  tri2obj file (.1.node) (.1.ele)
#
#     Take the file.1.node and file.1.ele and produce file.1.obj

if ( $do_tri2obj ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.node$/ ) {
	    $file =~ s/\.node$//;  # strip off the ".node"
	    
	    $command = "./Tri2obj/tri2obj $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    unlink("$subdir/$file.node");
	    unlink("$subdir/$file.node.orig");
	    unlink("$subdir/$file.ele");
	}
    }
}


# 6.  strip file.1.obj
# 
#     Strip the file.1.obj's
#
# 7.  cp bands.d file.2.obj
#
#     strips produces a file called "bands.d" ... copy this to file.2.obj

if ( $do_strips ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.obj$/ ) {
	    $command = "./Stripe_u/strips $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    # copy to destination file
	    $newfile = $file;
	    $newfile =~ s/\.1\.obj$//;
	    print "Copying to $subdir/$newfile.2.obj\n";
	    open(IN, "<bands.d");
	    open(OUT, ">$subdir/$newfile.2.obj");
	    while ( <IN> ) {
		print OUT $_;
	    }
	    close(IN);
	    close(OUT);
	    
	    unlink("$subdir/$file");
	}
    }
}


# 8.  fixobj file-new
#
#     Sort file.2.obj by strip winding

if ( $do_fixobj ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.2\.obj$/ ) {
	    $newfile = $file;
	    $newfile =~ s/\.2\.obj$/.obj/;
	    
	    $command = "./FixObj/fixobj $subdir/$file $subdir/$newfile";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    unlink("$subdir/$file");
	}
    }
}


#---------------------------------------------------------------------------
# $Log$
# Revision 1.3  1998/01/14 02:15:52  curt
# Updated front end script to keep plugging away on tile fitting.
#
# Revision 1.2  1998/01/12 20:42:08  curt
# Working on fitting tiles together in a seamless manner.
#
# Revision 1.1  1998/01/09 23:06:46  curt
# Initial revision.
#
