// main.c -- main loop
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
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>

#include "area.hxx"

#include <Bucket/bucketutils.h>
#include <Include/fg_constants.h>


int main( int argc, char **argv ) {
    fgBUCKET b;
    point2d nodes[4];
    FILE *fd;
    char base[256], path[256], command[256], file[256], exfile[256];
    double lon, lat, elevation, heading;
    double length, width;
    long int index;
    int i, count;

    if ( argc != 2 ) {
	printf("Usage %s <work dir>\n", argv[0]);
	exit(0);
    }

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
}


