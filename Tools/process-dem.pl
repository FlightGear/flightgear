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

$do_demfit =     1;
$do_triangle_1 = 1;
$do_fixnode =    1;
$do_splittris =  1;
$do_assemtris =  1;
$do_triangle_2 = 1;

$do_tri2obj =    1;
$do_strips =     1;
$do_fixobj =     1;


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
	if ( m/^Dir = / ) {
	    $subdir = $_;
	    $subdir =~ s/^Dir = //;
	    chop($subdir);
	}
    }
    close(OUT);
} else {
    $subdir = "../Scenery/w100n040/w093n045";
    print "WARNING:  Hardcoding subdir = $subdir\n";
}

# 3.  triangle -q file (Takes file.node and produces file.1.node and
#                      file.1.ele)

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

	    unlink("$subdir/$file.node");
	    unlink("$subdir/$file.node.orig");
	    unlink("$subdir/$file.ele");
	}
    }
}


# 4.2 read in the split of version of the tiles, reconstruct the tile
#     using the proper shared corners and edges.  Save as a node file
#     so we can retriangulate.

if ( $do_assemtris ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.body$/ ) {
	    $file =~ s/\.body$//;  # strip off the ".body"
	
	    $command = "./AssemTris/assemtris $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	}
	unlink("$subdir/$file.body");
    }
}


# 4.3 Retriangulate reassembled files (without -q option) so no new
#     nodes are generated.

if ( $do_triangle_2 ) {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	print $file;
	chop($file);
	if ( ($file =~ m/\.node$/) && ($file !~ m/\.\d\.node$/) ) {
	    $command = "./Triangle/triangle $subdir/$file";
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


# 5.  tri2obj file (.1.node) (.1.ele)
#
#     Take the file.1.node and file.1.ele and produce file.1.obj
#
#     Extracts normals out of the shared edge/vertex files, and uses
#     the precalcuated normals for these nodes instead of calculating
#     new ones.  By sharing normals as well as vertices, not only are
#     the gaps between tiles eliminated, but the colors and lighting
#     transition smoothly across tile boundaries.

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
#     Strip the file.1.obj's.  Note, strips doesn't handle the minimal
#     case of striping a square correctly.
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
# Revision 1.9  1998/01/27 18:36:54  curt
# Lots of updates to get back in sync with changes made over in .../Src/
#
# Revision 1.8  1998/01/21 17:59:05  curt
# Uncomment lines to remove several intermediate files.
#
# Revision 1.7  1998/01/19 19:51:06  curt
# A couple final pre-release tweaks.
#
# Revision 1.6  1998/01/15 21:33:33  curt
# Assembling triangles and building a new .node file with the proper shared
# vertices now works.  Now we just have to use the shared normals and we'll
# be all set.
#
# Revision 1.5  1998/01/15 02:50:08  curt
# Tweaked to add next stage.
#
# Revision 1.4  1998/01/14 15:55:34  curt
# Finished splittris, started assemtris.
#
# Revision 1.3  1998/01/14 02:15:52  curt
# Updated front end script to keep plugging away on tile fitting.
#
# Revision 1.2  1998/01/12 20:42:08  curt
# Working on fitting tiles together in a seamless manner.
#
# Revision 1.1  1998/01/09 23:06:46  curt
# Initial revision.
#
