#!/usr/local/bin/perl
#
# dem2scene.pl -- Read in a dem data file, and output a more usable format.
#
# Written by Curtis Olson, started May 1997.
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


use strict;


# declare variables
my($token);
my($i, $j);
my($arg);
my($res) = 1;

# "A" Record Information
my($dem_description, $dem_quadrangle);
my($dem_x1, $dem_y1, $dem_x2, $dem_y2, $dem_x3, $dem_y3, $dem_x4, $dem_y4);
my($dem_z1, $dem_z2); 
my($dem_resolution, $dem_num_profiles);

# "B" Record Information
my($prof_row, $prof_col);
my($prof_num_rows, $prof_num_cols);
my($prof_x1, $prof_y1);
my($prof_data);

# set input record separator to be a space
$/ = " ";

# parse command line arguments
while ( $arg = shift(@ARGV) ) {
    if ( $arg eq "-r" ) {
	$res = shift(@ARGV);
	if ( $res < 1 ) {
	    &usage();
	}
    } else {
	&usage();
    }
}

# print usage and die
sub usage {
    die "Usage: $0 [ -r resval ]\n";
}


&read_a_record();
&output_scene_hdr();

$i = 0;
while ( $i < $dem_num_profiles ) {
    &read_b_record();
    &output_row();

    $i++;

    if ( $i < $dem_num_profiles ) {
	# not on last record
	for ( $j = 1; $j < $res; $j++ ) {
	    # print "skipping row\n";
	    &read_b_record();
	}
    }
}

&output_scene_close();

# read and parse DEM "A" record
sub read_a_record {
    my($i);

    # read initial descriptive header
    while ( ($token = &next_token()) ne "_END_OF_FILE_" ) {
	if ( $token !~ m/^NJ/ && $token !~ m/^NI/ ) {
	    $dem_description .= "$token ";
	} else {
	    chop($dem_description);
	    $dem_quadrangle = $token;
	    last;
	}
    }
    # print "'$dem_description' '$dem_quadrangle'\n";

    # DEM level code, 3 reflects processing by DMA
    &next_token();

    # Pattern code, 1 indicates a regular elevation pattern
    &next_token();

    # Planimetric reference system code, 0 indicates geographic
    # coordinate system.
    &next_token();

    # Zone code
    &next_token();

    # Map projection parameters (ignored)
    for ($i = 0; $i < 15; $i++) {
	&next_token();
    }

    # Units code, 3 represents arc-seconds as the unit of measure for
    # ground planimetric coordinates throughout the file.
    die "Unknown units code!\n" if ( &next_token() ne "3" );

    # Units code; 2 represents meters as the unit of measure for
    # elevation coordinates throughout the file.  
    die "Unknown units code!\n" if ( &next_token() ne "2" );

    # Number (n) of sides in the polygon which defines the coverage of
    # the DEM file (usually equal to 4).
    die "Unknown polygon dimension!\n" if ( &next_token() ne "4" );

    # Ground coordinates of bounding box in arc-seconds
    $dem_x1 = &next_token();
    $dem_y1 = &next_token();

    $dem_x2 = &next_token();
    $dem_y2 = &next_token();

    $dem_x3 = &next_token();
    $dem_y3 = &next_token();

    $dem_x4 = &next_token();
    $dem_y4 = &next_token();

    # Minimum/maximum elevations in meters
    $dem_z1 = &next_token();
    $dem_z2 = &next_token();

    # Counterclockwise angle from the primary axis of ground
    # planimetric referenced to the primary axis of the DEM local
    # reference system.
    &next_token();

    # Accuracy code; 0 indicates that a record of accuracy does not
    # exist and that no record type C will follow.
    # &next_token();

    # DEM spacial resolution.  Usually (3,3,1) (3,6,1) or (3,9,1)
    # depending on latitude
    $dem_resolution = &next_token();

    # one dimensional arrays
    &next_token();

    # number of profiles
    $dem_num_profiles = &next_token();
    $dem_num_profiles = (($dem_num_profiles - 1) / $res) + 1;
}


# output the scene headers
sub output_scene_hdr {
    my($dx, $dy, $dz);

    printf("mesh %s_terrain {\n", $dem_quadrangle);

    $dem_x1 =~ s/D/E/; $dem_x1 += 0.0;
    $dem_y1 =~ s/D/E/; $dem_y1 += 0.0;
    print "    // This mesh is rooted at the following coordinates (in arc seconds)\n";
    print "    origin_lon = $dem_x1\n";
    print "    origin_lat = $dem_y1\n";
    print "\n";

    print "    // Number of rows and columns (needed by the parser so it can create\n";
    print "    //the proper size structure\n";
    print "    rows = $dem_num_profiles\n";
    print "    cols = $dem_num_profiles\n";  # This isn't necessarily guaranteed
    print "\n";

    ($dx, $dy, $dz) = $dem_resolution =~ 
	m/(.............)(............)(............)/;
    $dx *= $res;
    $dy *= $res;
    print "    // Distance between x and y data points (in arc seconds)\n";
    print "    row_step = $dx\n";
    print "    col_step = $dy\n";
    print "\n";
}


# output the scene close
sub output_scene_close {
    print "\n";
    print "}\n";
}


# read and parse DEM "B" record
sub read_b_record {
    my($i, $j);

    # row / column id of this profile
    $prof_row = &next_token();
    $prof_col = &next_token();

    # Number of rows (elevations) and columns in this profile;
    $prof_num_rows = &next_token();
    $prof_num_cols = &next_token();

    $prof_num_rows = (($prof_num_rows - 1) / $res) + 1;
    # print "profile num rows = $prof_num_rows\n";

    # Ground planimetric coordinates (arc-seconds) of the first
    # elevation in the profile
    $prof_x1 = &next_token();
    $prof_y1 = &next_token();

    # Elevation of local datum for the profile.  Always zero for
    # 1-degree DEM, the reference is mean sea level.
    &next_token();

    # Minimum and maximum elevations for the profile.
    &next_token();
    &next_token();

    # One (usually) dimensional array ($prof_num_rows,1) of elevations
    $prof_data = "";
    $i = 0;
    while ( $i < $prof_num_rows ) {
	$prof_data .= &next_token();
	$prof_data .= " ";

	$i++;

	if ( $i < $prof_num_rows ) {
	    # not on last data point
	    # skip the tokens to get requested resolution
	    for ($j = 1; $j < $res; $j++) {
		# print "skipping ...\n";
		&next_token();
	    }
	}
    }
    chop($prof_data);

    # print "$prof_data\n\n";
}


# output a row of data
sub output_row {
    print "    row = ($prof_data)\n";
}


# return next token from input stream
sub next_token {
    my($token);

    # print "in next token\n";

    do {
	$token = <>; chop($token);
	if ( eof() ) {
	    $token = "_END_OF_FILE_";
	}
    } while ( $token eq "" );

    # print "returning $token\n";

    return $token;
}

while ( ($token = &next_token()) ne "_END_OF_FILE_" ) {
    # print "'$token'\n";
}


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1997/05/30 19:30:16  curt
# The LaRCsim flight model is starting to look like it is working.
#
# Revision 1.1  1997/05/27 21:56:02  curt
# Initial revision (with data skipping support)
#
