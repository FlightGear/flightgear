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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <list>
#include <stdio.h>
#include <string.h>
#include <string>

#include <Bucket/bucketutils.h>
#include <Include/fg_constants.h>
#include <Include/fg_zlib.h>

#include "area.hxx"
#include "convex_hull.hxx"


// process and airport + runway list
void process_airport( string last_airport, list < string > & runway_list,
		      const string& root ) {
    list_container rwy_list, apt_list, hull_list;
    list_iterator current, last;

    string line_str;
    double lon, lat;
    int len, width, hdg, label_hdg, elev;
    char codes[10];
    char side;
    point2d average;
    double sum_x, sum_y;

    FILE *fd;
    fgBUCKET b;
    long int index;
    char base[256], tmp[256];
    string path, command, exfile, file, aptfile;
    int i, count;

    printf( "(apt) %s", last_airport.c_str() );

    list < string >::iterator last_runway = runway_list.end();
    for ( list < string >::iterator current_runway = runway_list.begin();
	  current_runway != last_runway ; ++current_runway ) {
	line_str = (*current_runway);
	printf( "%s", line_str.c_str() );

	sscanf( line_str.c_str(), "%lf %lf %d %d %d %s %d %c %d\n",
		&lon, &lat, &len, &width, &hdg, codes, &label_hdg, 
		&side, &elev );

	rwy_list = gen_runway_area( lon, lat, (double)hdg * DEG_TO_RAD, 
				    (double)len * FEET_TO_METER,
				    (double)width * FEET_TO_METER );

	// add rwy_list to apt_list
	current = rwy_list.begin();
	last = rwy_list.end();
	for ( ; current != last ; ++current ) {
	    apt_list.push_back(*current);
	}
    }

    // printf("Runway points in degrees\n");
    current = apt_list.begin();
    last = apt_list.end();
    for ( ; current != last; ++current ) {
	// printf( "%.5f %.5f\n", current->lon, current->lat );
    }
    // printf("\n");

    // generate convex hull
    hull_list = convex_hull(apt_list);

    // find average center point of convex hull
    count = hull_list.size();

    current = hull_list.begin();
    last = hull_list.end();
    sum_x = sum_y = 0.0;
    for ( ; current != last; ++current ) {
	// printf("return = %.6f %.6f\n", (*current).x, (*current).y);
	sum_x += (*current).x;
	sum_y += (*current).y;
    }

    average.x = sum_x / count;
    average.y = sum_y / count;

    // find bucket based on average center point of hull list.
    // Eventually we'll need to handle cases where the area crosses
    // bucket boundaries.
    fgBucketFind( average.lon, average.lat, &b);
    printf( "Bucket = lon,lat = %d,%d  x,y index = %d,%d\n", 
	    b.lon, b.lat, b.x, b.y);

    index = fgBucketGenIndex(&b);
    fgBucketGenBasePath(&b, base);
    path = root + "/Scenery/" + base;
    command = "mkdir -p " + path;
    system( command.c_str() );

    sprintf(tmp, "%ld", index);
    exfile =  path + "/" + tmp + ".node.ex";
    file =    path + "/" + tmp + ".poly";
    aptfile = path + "/" + tmp + ".apt";
    printf( "extra node file = %s\n", exfile.c_str() );
    printf( "poly file = %s\n", file.c_str() );
    printf( "apt file = %s\n", aptfile.c_str() );

    // output exclude nodes
    printf("Output exclude nodes\n");
    if ( (fd = fopen(exfile.c_str(), "w")) == NULL ) {
        printf("Cannot open file: %s\n", exfile.c_str());
        exit(-1);
    }

    fprintf( fd, "%d 2 0 0\n", count );

    current = hull_list.begin();
    last = hull_list.end();
    i = 1;
    for ( ; current != last ; ++current ) {
	// printf( "(%.4f, %.4f)\n", 
	fprintf( fd, "%d %.2f %.2f %.2f\n", i, 
		 (*current).lon * 3600.0, (*current).lat * 3600.0, 
		 (double)elev * FEET_TO_METER );
	++i;
    }
    fclose(fd);

    // output poly
    printf("Output poly\n");
    if ( (fd = fopen(file.c_str(), "w")) == NULL ) {
        printf("Cannot open file: %s\n", file.c_str());
        exit(-1);
    }

    // output empty node list
    fprintf(fd, "0 2 0 0\n");

    // output segments
    fprintf( fd, "%d 0\n", count );
    for ( i = 1; i < count; i++ ) {
	fprintf( fd, "%d %d %d\n", i, i, i + 1 );
    }
    fprintf( fd, "%d %d %d\n", count, count, 1 );

    // output hole center
    fprintf( fd, "1\n");
    fprintf( fd, "1 %.2f %.2f\n", average.x * 3600.0, average.y * 3600);

    fclose(fd);

    // output "apt" file
    printf("Output airport\n");
    if ( (fd = fopen(aptfile.c_str(), "w")) == NULL ) {
        printf("Cannot open file: %s\n", aptfile.c_str());
        exit(-1);
    }

    // write main airport identifier
    fprintf(fd, "a %s", last_airport.c_str() );

    // write perimeter polygon
    current = hull_list.begin();
    last = hull_list.end();
    for ( ; current != last ; ++current ) {
	fprintf( fd, "p %.7f %.7f %.2f\n", (*current).lon, (*current).lat, 
		 (double)elev * FEET_TO_METER );
    }

    // write runway info
    for ( list < string >::iterator current_runway = runway_list.begin();
	  current_runway != last_runway ; ++current_runway ) {
	line_str = (*current_runway);
	line_str = line_str.substr(1, line_str.size());
	fprintf(fd, "r %s", line_str.c_str() );
    }

    fclose(fd);
}


// reads the apt_full file and extracts and processes the individual
// airport records
int main( int argc, char **argv ) {
    list < string > runway_list;
    string apt_path, gz_apt_path;
    string airport, last_airport;
    fgFile f;
    char line[256];
    /*
    fgBUCKET b;
    point2d nodes[4];
    char base[256], path[256], command[256], file[256], exfile[256];
    double lon, lat, elevation, heading;
    double length, width;
    long int index;
    */

    if ( argc != 3 ) {
	printf("Usage %s <apt_file> <work dir>\n", argv[0]);
	exit(0);
    }

    apt_path = argv[1];
    gz_apt_path = apt_path + ".gz";

    // first try "path.gz"
    if ( (f = fgopen(gz_apt_path.c_str(), "rb")) == NULL ) {
	// next try "path"
        if ( (f = fgopen(apt_path.c_str(), "rb")) == NULL ) {
	    printf( "Cannot open file: %s\n", apt_path.c_str());
	}
    }

    while ( fggets(f, line, 250) != NULL ) {
	// printf("%s", line);
	if ( strlen(line) == 0 ) {
	    // empty, skip
	} else if ( line[0] == '#' ) {
	    // comment, skip
	} else if ( line[0] == '\t' ) {
	    // runway entry
	    runway_list.push_back(line);
	} else {
	    // start of airport record
	    airport = line;

	    if ( last_airport.length() ) {
		// process previous record
		process_airport(last_airport, runway_list, argv[2]);
	    }

	    // clear runway list for start of next airport
	    runway_list.erase(runway_list.begin(), runway_list.end());

	    last_airport = airport;
	}
    }

    if ( last_airport.length() ) {
	// process previous record
	process_airport(last_airport, runway_list, argv[2]);
    }

    fgclose(f);

    return(1);
}


// $Log$
// Revision 1.5  1998/09/17 18:40:43  curt
// Debug message tweaks.
//
// Revision 1.4  1998/09/09 20:59:56  curt
// Loop construct tweaks for STL usage.
// Output airport file to be used to generate airport scenery on the fly
//   by the run time sim.
//
//
