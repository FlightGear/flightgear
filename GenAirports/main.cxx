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


// process and airport + runway list
void process_airport( string last_airport, list < string > & runway_list ) {
    list < point2d > rwy_list, apt_list;
    list < point2d > :: iterator current;                          
    list < point2d > :: iterator last;                    

    string line_str;
    double lon, lat;
    int len, width, hdg, label_hdg, elev;
    char codes[10];
    char side;

    printf( "(apt) %s", last_airport.c_str() );

    while ( runway_list.size() ) {
	line_str = runway_list.front();
	runway_list.pop_front();
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
	while ( current != last ) {
	    apt_list.push_back(*current);
	    current++;
	}
    }

    printf("Final results in degrees\n");
    current = apt_list.begin();
    last = apt_list.end();
    while ( current != last ) {
	// printf( "(%.4f, %.4f)\n", 
	printf( "%.5f %.5f\n", 
		current->lon * RAD_TO_DEG,
		current->lat * RAD_TO_DEG );
	current++;
    }
    printf("\n");
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
		process_airport(last_airport, runway_list);
	    }

	    last_airport = airport;
	}
    }

    if ( last_airport.length() ) {
	// process previous record
	process_airport(last_airport, runway_list);
    }

    fgclose(f);

    return(1);
}


#if 0
    // P13 (Globe, AZ)
    // lon = -110.6642442;
    // lat = 33.3528903;
    // heading = 102.0 * DEG_TO_RAD;
    // length = 1769;
    // width = 23;

    // KANE
    lon = -93.2113889;
    lat = 45.145;
    elevation = 912 * FEET_TO_METER;
    heading = 270.0 * DEG_TO_RAD;
    length = 1220;
    width = 23;

    gen_runway_area( lon * DEG_TO_RAD, lat * DEG_TO_RAD, 
		     heading, length, width, nodes, &count );

    fgBucketFind(lon, lat, &b);
    printf( "Bucket = lon,lat = %d,%d  x,y index = %d,%d\n", 
	    b.lon, b.lat, b.x, b.y);

    index = fgBucketGenIndex(&b);
    fgBucketGenBasePath(&b, base);
    sprintf(path, "%s/Scenery/%s", argv[1], base);
    sprintf(command, "mkdir -p %s\n", path);
    system(command);
    
    sprintf(exfile, "%s/%ld.node.ex", path, index);
    sprintf(file, "%s/%ld.poly", path, index);
    printf( "extra node file = %s\n", exfile);
    printf( "poly file = %s\n", file);

    // output extra nodes
    if ( (fd = fopen(exfile, "w")) == NULL ) {
        printf("Cannot open file: %s\n", exfile);
        exit(-1);
    }

    fprintf(fd, "%d 2 0 0\n", count);
    for ( i = 0; i < count; i++ ) {
	fprintf( fd, "%d %.2f %.2f %.2f\n", i + 1, 
		 nodes[i].lon * RAD_TO_ARCSEC, nodes[i].lat * RAD_TO_ARCSEC, 
		 elevation);
    }
    fclose(fd);

    // output poly
    if ( (fd = fopen(file, "w")) == NULL ) {
        printf("Cannot open file: %s\n", file);
        exit(-1);
    }

    // output empty node list
    fprintf(fd, "0 2 0 0\n");

    // output segments
    fprintf(fd, "%d 0\n", count);
    for ( i = 0; i < count - 1; i++ ) {
	fprintf( fd, "%d %d %d\n", i + 1, i + 1, i + 2 );
    }
    fprintf( fd, "%d %d %d\n", count, count, 1 );

    // output hole center
    fprintf( fd, "1\n");
    fprintf( fd, "1 %.2f %.2f\n", lon * 3600.0, lat * 3600);

    fclose(fd);

#endif


// $Log: main.c,v
//
