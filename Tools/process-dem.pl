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
#---------------------------------------------------------------------------


# format version number
$scenery_format_version = "0.1";

$max_area = 10000;            # maximum triangle area
$remove_tmps = 1;

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
$system_name = `uname -s`; chop($system_name);

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


# replace all unix forward slashes with windoze backwards slashes if
# running on a cygwin32 system
sub fix_slashes { my($in) = @_;
    if ( $system_name =~ m/CYGWIN32/ ) { 
        $in =~ s/\/+/\//g;
        $in =~ s/\//\\/g;
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
    $command = fix_slashes($command);
    print "Running '$command'\n";

    open(OUT, "$command |");
    while ( <OUT> ) {
	print $_;
	if ( m/^Dir = / ) {
	    $subdir = $_;
	    $subdir =~ s/^Dir = //;
	    chop($subdir);
	}
	if ( m/Quad name field/ ) {
	    $quad_name = $_;
	    # strip header
	    $quad_name =~ s/.*Quad name field: //;
	    # crunch consequetive spaces
	    $quad_name =~ s/  +/ /g;
	    chop($quad_name);
	    print "QUAD NAME = $quad_name\n";
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
	    $command = fix_slashes($command);
	    if ( -r "$subdir/$fileroot.poly" ) {
		$command .= " -pc";
	    }
	    $command .= " -a$max_area -q10 $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    # remove input file.node
	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file";
		$file1 = fix_slashes($file1);
		unlink($file1);
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
    $command = "FixNode/fixnode";
    $command = fix_slashes($command);
    $command .= " $dem_file $subdir";
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
	
	    $command = "SplitTris/splittris";
	    $command = fix_slashes($command);
	    $command .= " $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file.node";
		$file1 = fix_slashes($file1);
		unlink($file1);
		$file1 = "$subdir/$file.node.orig";
		$file1 = fix_slashes($file1);
		unlink($file1);
		$file1 = "$subdir/$file.ele";
		$file1 = fix_slashes($file1);
		unlink($file1);
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
	
	    $command = "AssemTris/assemtris";
	    $command = fix_slashes($command);
	    $command .= " $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	}
	if ( $remove_tmps ) {
	    $file1 = "$subdir/$file.body";
	    $file1 = fix_slashes($file1);
	    unlink($file1);
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
	    $command = fix_slashes($command);

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

	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    # remove input file.node
	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file";
		$file1 = fix_slashes($file1);
		unlink($file1);
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
	    
	    $command = "Tri2obj/tri2obj";
	    $command = fix_slashes($command);
	    $command .= " $subdir/$file";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file.node";
		$file1 = fix_slashes($file1);
		unlink($file1);
		$file1 = "$subdir/$file.node.orig";
		$file1 = fix_slashes($file1);
		unlink($file1);
		$file1 = "$subdir/$file.ele";
		$file1 = fix_slashes($file1);
		unlink($file1);
	    }
	}
    }
}


# 6.  strip file.1.obj
# 
#     Strip the file.1.obj's.  Note, strips doesn't handle the minimal
#     case of striping a square correctly.
#
# 7.  cp stripe.objf file.2.obj
#
#     strips produces a file called "stripe.objf" ... copy this to file.2.obj

sub strips {
    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\.1\.obj$/ ) {
	    $newfile = $file;
	    $newfile =~ s/\.1\.obj$//;
	    $command = "Stripe_w/strips";
	    $command = fix_slashes($command);
	    $command .= " $subdir/$file $subdir/$newfile.2.obj";
	    print "Running '$command'\n";
    	    # $input = <STDIN>;
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	    
	    # copy to destination file
	    # $newfile = $file;
	    # $newfile =~ s/\.1\.obj$//;
	    # print "Copying to $subdir/$newfile.2.obj\n";
	    # open(IN, "<bands.d");
	    # open(IN, "<stripe.objf");
	    # open(OUT, ">$subdir/$newfile.2.obj");
	    # while ( <IN> ) {
		# print OUT $_;
	    # }
	    # close(IN);
	    # close(OUT);
	    
	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file";
		$file1 = fix_slashes($file1);
		unlink($file1);
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
	    
	    $command = "FixObj/fixobj";
	    $command = fix_slashes($command);
	    $command .= " $subdir/$file $subdir/$newfile";
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file";
		$file1 = fix_slashes($file1);
		unlink($file1);
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

    # try to get rid of double //
    $install_dir =~ s/\/+/\//g;
    print "Install dir = $install_dir\n";

    if ( $system_name !~ m/CYGWIN32/ ) {
	$command = "mkdir -p $install_dir";
    } else {
        $command = "Makedir/makedir $install_dir";
	$command = fix_slashes($command);
    }

    # print "Running '$command'\n";
    open(OUT, "$command |");
    while ( <OUT> ) {
	print $_;
    }
    close(OUT);

    # write out version and info record
    $version_file = "$install_dir/VERSION";
    open(VERSION, ">$version_file") || 
	die "Cannot open $version_file for writing\n";
    print VERSION "FGFS Scenery Version $scenery_format_version\n";
    if ( $system_name !~ m/CYGWIN32/ ) {
	$date = `date`; chop($date);
    } else {
	# ???
	$date = "not available";
    }
    $hostname = `hostname`; chop($hostname);
    print VERSION "Creator = $ENV{LOGNAME}\n";
    print VERSION "Date = $date\n";
    print VERSION "Machine = $hostname\n";
    print VERSION "\n";
    print VERSION "DEM File Name = $dem_file\n";
    print VERSION "DEM Label = $quad_name\n";
    print VERSION "Error Tolerance = $error (this value is squared)\n";
    close(VERSION);

    @FILES = `ls $subdir`;
    foreach $file ( @FILES ) {
	chop($file);
	if ( $file =~ m/\d\d.obj$/ ) {
	    $new_file = file_root($file);
	    
	    $command = 
		"gzip -c --best -v < $subdir/$file > $install_dir/$new_file.gz";
	    $command = fix_slashes($command);

	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);

	    if ( $remove_tmps ) {
		$file1 = "$subdir/$file";
		$file1 = fix_slashes($file1);
		unlink($file1);
	    }
	} elsif ( $file =~ m/\d\d.apt$/ ) {
	    $command = "cp $subdir/$file $install_dir/$file";
	    $command = fix_slashes($command);
	    print "Running '$command'\n";
	    open(OUT, "$command |");
	    while ( <OUT> ) {
		print $_;
	    }
	    close(OUT);
	}
    }
}


#---------------------------------------------------------------------------
