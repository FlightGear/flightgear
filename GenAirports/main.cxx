// main.cxx -- main loop
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Include/compiler.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <list>
#include <stdio.h>
#include <string.h>
#include STL_STRING

#include <Bucket/newbucket.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Misc/fgstream.hxx>
#include <Polygon/index.hxx>

#include "area.hxx"
#include "convex_hull.hxx"


// write out airport data
void write_airport( int p_index, list_container hull_list, FGBucket b, 
		    const string& root, const bool cut_and_keep ) {
    char tile_name[256], poly_index[256];

    long int b_index = b.gen_index();
    string base = b.gen_base_path();
    string path = root + "/Scenery/" + base;
    string command = "mkdir -p " + path;
    system( command.c_str() );

    sprintf(tile_name, "%ld", b_index);
    string aptfile = path + "/" + tile_name;

    sprintf( poly_index, "%d", p_index );
    aptfile += ".";
    aptfile += poly_index;
    cout << "apt file = " << aptfile << endl;

    FILE *fd;
    if ( (fd = fopen(aptfile.c_str(), "a")) == NULL ) {
        cout << "Cannot open file: " << aptfile << endl;
        exit(-1);
    }

    if ( cut_and_keep ) {
	fprintf( fd, "AirportKeep\n" );
    } else {
	fprintf( fd, "AirportIgnore\n" );
    }
    fprintf( fd, "%d\n", hull_list.size() );
    // write perimeter polygon
    list_iterator current = hull_list.begin();
    list_iterator last = hull_list.end();
    for ( ; current != last ; ++current ) {
	fprintf( fd, "%.7f %.7f\n", (*current).lon, (*current).lat );
    }

    fclose(fd);
}


// process and airport + runway list
void process_airport( string airport, list < string > & runway_list,
		      const string& root ) {
    list_container rwy_list, apt_list, hull_list;
    list_iterator current, last;

    // parse main airport information
    int elev;

    cout << airport << endl;
    string apt_type = airport.substr(0, 1);
    string apt_code = airport.substr(2, 4);
    string apt_lat = airport.substr(7, 10);
    string apt_lon = airport.substr(18, 11);
    string apt_elev = airport.substr(30, 5);
    sscanf( apt_elev.c_str(), "%d", &elev );
    string apt_use = airport.substr(36, 1);
    string apt_twr = airport.substr(37, 1);
    string apt_bldg = airport.substr(38, 1);
    string apt_name = airport.substr(40);

    /*
    cout << "  type = " << apt_type << endl;
    cout << "  code = " << apt_code << endl;
    cout << "  lat  = " << apt_lat << endl;
    cout << "  lon  = " << apt_lon << endl;
    cout << "  elev = " << apt_elev << " " << elev << endl;
    cout << "  use  = " << apt_use << endl;
    cout << "  twr  = " << apt_twr << endl;
    cout << "  bldg = " << apt_bldg << endl;
    cout << "  name = " << apt_name << endl;
    */

    // parse runways and generate the vertex list
    string rwy_str;
    double lon, lat, hdg;
    int len, width;

    list < string >::iterator last_runway = runway_list.end();
    for ( list < string >::iterator current_runway = runway_list.begin();
	  current_runway != last_runway ; ++current_runway ) {
	rwy_str = (*current_runway);

	cout << rwy_str << endl;
	string rwy_no = rwy_str.substr(2, 4);
	string rwy_lat = rwy_str.substr(6, 10);
	sscanf( rwy_lat.c_str(), "%lf", &lat);
	string rwy_lon = rwy_str.substr(17, 11);
	sscanf( rwy_lon.c_str(), "%lf", &lon);
	string rwy_hdg = rwy_str.substr(29, 7);
	sscanf( rwy_hdg.c_str(), "%lf", &hdg);
	string rwy_len = rwy_str.substr(36, 7);
	sscanf( rwy_len.c_str(), "%d", &len);
	string rwy_width = rwy_str.substr(43, 4);
	sscanf( rwy_width.c_str(), "%d", &width);
	string rwy_sfc = rwy_str.substr(47, 4);
	string rwy_end1 = rwy_str.substr(52, 6);
	string rwy_end2 = rwy_str.substr(59, 6);

	/*
	cout << "  no    = " << rwy_no << endl;
	cout << "  lat   = " << rwy_lat << " " << lat << endl;
	cout << "  lon   = " << rwy_lon << " " << lon << endl;
	cout << "  hdg   = " << rwy_hdg << " " << hdg << endl;
	cout << "  len   = " << rwy_len << " " << len << endl;
	cout << "  width = " << rwy_width << " " << width << endl;
	cout << "  sfc   = " << rwy_sfc << endl;
	cout << "  end1  = " << rwy_end1 << endl;
	cout << "  end2  = " << rwy_end2 << endl;
	*/
	
	rwy_list = gen_runway_area( lon, lat, hdg * DEG_TO_RAD, 
				    (double)len * FEET_TO_METER,
				    (double)width * FEET_TO_METER );

	// add rwy_list to apt_list
	current = rwy_list.begin();
	last = rwy_list.end();
	for ( ; current != last ; ++current ) {
	    apt_list.push_back(*current);
	}
    }

    if ( apt_list.size() == 0 ) {
	cout << "no runway points generated" << endl;
	return;
    }

    // printf("Runway points in degrees\n");
    // current = apt_list.begin();
    // last = apt_list.end();
    // for ( ; current != last; ++current ) {
    //   printf( "%.5f %.5f\n", current->lon, current->lat );
    // }
    // printf("\n");

    // generate convex hull
    hull_list = convex_hull(apt_list);

    // get next polygon index
    int index = poly_index_next();

    // find average center, min, and max point of convex hull
    point2d average, min, max;
    double sum_x, sum_y;
    int count = hull_list.size();
    current = hull_list.begin();
    last = hull_list.end();
    sum_x = sum_y = 0.0;
    min.x = min.y = 200.0;
    max.x = max.y = -200.0;
    for ( ; current != last; ++current ) {
	// printf("return = %.6f %.6f\n", (*current).x, (*current).y);
	sum_x += (*current).x;
	sum_y += (*current).y;

	if ( (*current).x < min.x ) { min.x = (*current).x; }
	if ( (*current).y < min.y ) { min.y = (*current).y; }
	if ( (*current).x > max.x ) { max.x = (*current).x; }
	if ( (*current).y > max.y ) { max.y = (*current).y; }
    }
    average.x = sum_x / count;
    average.y = sum_y / count;

    // find buckets for center, min, and max points of convex hull.
    // note to self: self, you should think about checking for runways
    // that span the data line
    FGBucket b(average.lon, average.lat);
    FGBucket b_min(min.x, min.y);
    FGBucket b_max(max.x, max.y);
    cout << "Bucket center = " << b << endl;
    cout << "Bucket min = " << b_min << endl;
    cout << "Bucket max = " << b_max << endl;
    
    if ( b_min == b_max ) {
	write_airport( index, hull_list, b, root, true );
    } else {
	FGBucket b_cur;
	int dx, dy, i, j;

	fgBucketDiff(b_min, b_max, &dx, &dy);
	cout << "airport spans tile boundaries" << endl;
	cout << "  dx = " << dx << "  dy = " << dy << endl;

	if ( (dx > 1) || (dy > 1) ) {
	    cout << "somethings really wrong!!!!" << endl;
	    exit(-1);
	}

	for ( j = 0; j <= dy; j++ ) {
	    for ( i = 0; i <= dx; i++ ) {
		b_cur = fgBucketOffset(min.x, min.y, i, j);
		if ( b_cur == b ) {
		    write_airport( index, hull_list, b_cur, root, true );
		} else {
		    write_airport( index, hull_list, b_cur, root, false );
		}
	    }
	}
	// string answer; cin >> answer;
    }
}


// reads the apt_full file and extracts and processes the individual
// airport records
int main( int argc, char **argv ) {
    list < string > runway_list;
    string airport, last_airport;
    string line;
    char tmp[256];

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc != 3 ) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Usage " << argv[0] << " <apt_file> <work_dir>" );
	exit(-1);
    }

    // make work directory
    string work_dir = argv[2];
    string command = "mkdir -p " + work_dir;
    system( command.c_str() );

    // initialize persistant polygon counter
    string counter_file = work_dir + "/polygon.counter";
    poly_index_init( counter_file );

    fg_gzifstream in( argv[1] );
    if ( !in ) {
        FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << argv[1] );
        exit(-1);
    }

    // throw away the first 3 lines
    in.getline(tmp, 256);
    in.getline(tmp, 256);
    in.getline(tmp, 256);

    last_airport = "";

    while ( ! in.eof() ) {
	in.getline(tmp, 256);
	line = tmp;
	// cout << line << endl;

	if ( line.length() == 0 ) {
	    // empty, skip
	} else if ( line[0] == '#' ) {
	    // comment, skip
	} else if ( (line[0] == 'A') || (line[0] == 'S') ) {
	    // start of airport record
	    airport = line;

	    if ( last_airport.length() ) {
		// process previous record
		process_airport(last_airport, runway_list, argv[2]);
	    }

	    // clear runway list for start of next airport
	    runway_list.erase(runway_list.begin(), runway_list.end());

	    last_airport = airport;
	} else if ( line[0] == 'R' ) {
	    // runway entry
	    runway_list.push_back(line);
	} else if ( line == "99" ) {
	    // end of file
	    break;
	} else {
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "Unknown line in file" << endl << line );
	    exit(-1);
	}
    }

    if ( last_airport.length() ) {
	// process previous record
	process_airport(last_airport, runway_list, argv[2]);
    }

    return 0;
}


// $Log$
// Revision 1.7  1999/02/25 21:32:49  curt
// Modified to adhere to new polygon naming convention, and also to read the
// new Robin Peel aiport format.
//
// Revision 1.6  1999/02/11 01:10:51  curt
// Start of scenery revamp project.
//
// Revision 1.5  1998/09/17 18:40:43  curt
// Debug message tweaks.
//
// Revision 1.4  1998/09/09 20:59:56  curt
// Loop construct tweaks for STL usage.
// Output airport file to be used to generate airport scenery on the fly
//   by the run time sim.
//
//
