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


$max_area = 10000;            # maximum triangle area
$remove_tmps = 0;

$| = 1;                         # flush buffers after every write

$do_dem2node =   1;
$do_triangle_1 = 1;
$do_fixnode =    1;
$do_splittris =  1;
$do_assemtris =  1;
$do_triangle_2 = 1;

$do_tri2obj =    1;
$do_strips =     1;
$do_fixobj =     1;
 
$do_install =    1;


if ( $#ARGV < 3 ) {
    die "Usage: $0 <fg-root-dir> <work-dir> <error^2> dem-file(s)\n";
}

# Start with file.dem

$fg_root = shift(@ARGV);
$work_dir = shift(@ARGV);
$error = shift(@ARGV);
$error += 0.0;

while ( $dem_file = shift(@ARGV) ) {
    print "Source file = $dem_file  Error tolerance = $error\n";

    if ( $error < 0.5 ) {
	die "I doubt you'll be happy with an error tolerance as " . 
	    "low as $error.\n";
    }


    if ( $do_dem2node ) {
	dem2node() ;
    } else {
	$subdir = "./work/Scenery/w120n030/w111n033";
	print "WARNING:  Hardcoding subdir = $subdir\n";
    }

    triangle_1() if ( $do_triangle_1 );
    fixnode() if ( $do_fixnode );
    splittris() if ( $do_splittris );
    assemtris() if ( $do_assemtris );
    triangle_2() if ( $do_triangle_2);
    tri2obj() if ( $do_tri2obj );
    strips() if ( $do_strips );
    fixobj() if ( $do_fixobj );
    install() if ( $do_install );
}


# exit normally
exit(0);


# fix command to work with windoze, replaces first "/" with "\\"
sub fix_command {
    my($in) = @_;

    $system = `uname -s`;
    chop($system);

    if ( $system =~ m/CYGWIN32/ ) { 
	$in =~ s/\//\\\\/;
    }

    return($in);
}


# return the file name root (ending at last ".")
sub file_root {
    my($file) = @_;
    my($pos);

    $pos = rindex($file, ".");
    return substr($file, 0, $pos);
}


# 1.  dem2node work_dir dem_file tolerance^2 (meters)
# 
#     - dem2node .. dem_file 160000
#
#     splits dem file into 64 file.node's which contain the
#     irregularly fitted vertices

sub dem2node {
    $command = "Dem2node/dem2node $work_dir $dem_file $error";
    $command = fix_command($command);
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
} 


# 2.  triangle -q file (Takes file.node and produces file.1.node and
#                      file.1.ele)

print "Subdirectory for this dem file is $subdir\n";

sub triangle_1 {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	# print $file;
	chop($file);
	if ( ($file =~ m/\.node$/) && ($file !~ m/\.\d\.node$/) ) {
	    # special handling is needed if .poly file exists
	    $fileroot = $file;
	    $fileroot =~ s/\.node$//;
	    print "$subdir/$fileroot\n";
	    $command = "Triangle/triangle";
	    if ( -r "$subdir/$fileroot.poly" ) {
		$command = " -pc";
	    }
	    $command .= " -a$max_area -q10 $subdir/$file";
	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    # remove input file.node
	    if ( $remove_tmps ) {
		unlink("$subdir/$file");
	    }
	}
    }
}


# 3.  fixnode file.dem subdir
#
#     Take the original .dem file (for interpolating Z values) and the
#     subdirecotry containing all the file.1.node's and replace with
#     fixed file.1.node

sub fixnode {
    $command = "FixNode/fixnode $dem_file $subdir";
    $command = fix_command($command);
    print "Running '$command'\n";
    open(OUT, "$command |") || die "cannot run command\n";
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

sub splittris {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.node$/ ) {
	    $file =~ s/\.node$//;  # strip off the ".node"
	
	    $command = "SplitTris/splittris $subdir/$file";
	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		unlink("$subdir/$file.node");
		unlink("$subdir/$file.node.orig");
		unlink("$subdir/$file.ele");
	    }
	}
    }
}


# 4.2 read in the split of version of the tiles, reconstruct the tile
#     using the proper shared corners and edges.  Save as a node file
#     so we can retriangulate.

sub assemtris {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.body$/ ) {
	    $file =~ s/\.1\.body$//;  # strip off the ".body"
	
	    $command = "AssemTris/assemtris $subdir/$file";
	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	}
	if ( $remove_tmps ) {
	    unlink("$subdir/$file.body");
	}
    }
}


# 4.3 Retriangulate reassembled files (without -q option) so no new
#     nodes are generated.

sub triangle_2 {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	# print $file;
	chop($file);
	if ( ($file =~ m/\.node$/) && ($file !~ m/\.\d\.node$/) ) {
	    $base = $file;
	    $base =~ s/\.node$//;
	    print("Test for $subdir/$base.q\n");

	    $command = "Triangle/triangle";

	    if ( -r "$subdir/$base.q" ) {
		# if triangle hangs, we can create a filebase.q for
		# the file it hung on.  Then, we test for that file
		# here which causes the incremental algorithm to run
		# (which shouldn't ever hang.)
		$command .= " -i";
	    }

	    if ( -r "$subdir/$base.poly" ) {
		$command .= " -pc $subdir/$base";
	    } else {
		$command .= " $subdir/$file";
	    }

	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    # remove input file.node
	    if ( $remove_tmps ) {
		unlink("$subdir/$file");
	    }
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

sub tri2obj {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.node$/ ) {
	    $file =~ s/\.node$//;  # strip off the ".node"
	    
	    $command = "Tri2obj/tri2obj $subdir/$file";
	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    if ( $remove_tmps ) {
		unlink("$subdir/$file.node");
		unlink("$subdir/$file.node.orig");
		unlink("$subdir/$file.ele");
	    }
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

sub strips {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.obj$/ ) {
	    $command = "Stripe_w/strips $subdir/$file";
	    $command = fix_command($command);
	    print "Running '$command'\n";
    	    # $input = <STDIN>;
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    # copy to destination file
	    $newfile = $file;
	    $newfile =~ s/\.1\.obj$//;
	    print "Copying to $subdir/$newfile.2.obj\n";
	    # open(IN, "<bands.d");
	    open(IN, "<stripe.objf");
	    open(OUT, ">$subdir/$newfile.2.obj");
	    while ( <IN> ) {
		print OUT $_;
	    }
	    close(IN);
	    close(OUT);
	    
	    if ( $remove_tmps ) {
		unlink("$subdir/$file");
	    }
	}
    }
}


# 8.  fixobj file-new
#
#     Sort file.2.obj by strip winding

sub fixobj {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.2\.obj$/ ) {
	    $newfile = $file;
	    $newfile =~ s/\.2\.obj$/.obj/;
	    
	    $command = "FixObj/fixobj $subdir/$file $subdir/$newfile";
	    $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		unlink("$subdir/$file");
	    }
	}
    }
}


# 9.  install
#
#     rename, compress, and install scenery files

sub install {
    $tmp = $subdir;
    $tmp =~ s/$work_dir//;
    # print "Temp dir = $tmp\n";
    $install_dir = "$fg_root/$tmp";
    print "Install dir = $install_dir\n";
    system("mkdir -p $install_dir");

    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\d\d.obj$/ ) {
	    $new_file = file_root($file);
	    
	    $command = "gzip -v --best < $subdir/$file > $install_dir/$new_file.gz";
	    # $command = fix_command($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		unlink("$subdir/$file");
	    }
	}
    }
}


#---------------------------------------------------------------------------
# $Log$
# Revision 1.25  1998/07/22 21:46:09  curt
# minor tweaks.
#
# Revision 1.24  1998/07/21 04:33:47  curt
# More tweaks for sub-area cutouts.
#
# Revision 1.23  1998/07/20 12:55:35  curt
# Several tweaks to start incorporating area cutouts into the pipeline.
#
# Revision 1.22  1998/07/08 14:49:13  curt
# tweaks.
#
# Revision 1.21  1998/06/08 17:18:37  curt
# Mods to test new Stripe fixes from Wilbur Streett.
#
# Revision 1.20  1998/06/05 18:20:24  curt
# Added DemInfo to dump out "A" record DEM info.
# Modified process-dem.pl to work in a temp directory and compress/copy the
# result to the final destination.
#
# Revision 1.19  1998/05/27 02:25:26  curt
# Added a flag to the first run of "triangle" to impose a maximum triangle
# size.  This forces really flat areas to be subdivided a certain amount
# anyways.  This makes for slightly more interesting scenery.
#
# Revision 1.18  1998/05/20 20:55:40  curt
# Makefile tweaks
#
# Revision 1.17  1998/04/28 01:23:25  curt
# Added a work around so that we can get past the "triangle" program
# hanging, by aborting and rerunning with that tile marked to use the "-i"
# option.
#
# Revision 1.16  1998/04/18 03:57:53  curt
# Added zlib library support.
#
# Revision 1.15  1998/04/08 23:24:07  curt
# Adopted Gnu automake/autoconf system.
#
# Revision 1.14  1998/04/06 21:09:38  curt
# Additional win32 support.
# Fixed a bad bug in dem file parsing that was causing the output to be
# flipped about x = y.
#
# Revision 1.13  1998/03/19 02:52:52  curt
# Updated to reflect some minor tool reorganization and the creation of class
# to handle DEM processing needs.
#
# Revision 1.12  1998/03/19 01:48:35  curt
# Added gpc-2.01 (generic polygon clipping library)
#
# Revision 1.11  1998/03/03 03:36:57  curt
# Cumulative tweaks.
#
# Revision 1.10  1998/02/01 03:42:26  curt
# Modifications to handle compressed dem files.
#
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
