/**************************************************************************
 * areamgr.c -- routines to handle dynamic management of scenery areas
 *
 * Written by Curtis Olson, started October 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef WIN32
#  include <windows.h>
#endif

#include <GL/glut.h>
#include "../XGL/xgl.h"

#include <math.h>

#include "../Include/constants.h"
/* temporary */
#include "../Math/fg_random.h"


struct ai {
    int lon;  /* longitude (-180 to 179) */
    int lat;  /* latitude (-90 to 89) */
    int x;    /* x (0 to 7) */
    int y;    /* y (0 to 7) */
};


/* we're entering into hacking territory here ... what fun. ;-) */
long int area_array[7][7];


/* Generate the unique scenery area index containing the specified
   lon/lat parameters.

   The index is constructed as follows:

   9 bits - to represent 360 degrees of longitude (-180 to 179)
   8 bits - to represent 180 degrees of latitude (-90 to 89)

   Each 1 degree by 1 degree area is further broken down into an 8x8
   grid.  So we also need:

   3 bits - to represent x (0 to 7)
   3 bits - to represent y (0 to 7) */
static long gen_index(struct ai *p) {
    long index = 0;

    index = ((p->lon + 180) << 14) + ((p->lat + 90) << 6) + (p->y << 3) + p->x;
    /* printf("  generated index = %ld\n", index); */

    return(index);
}


/* Parse a unique scenery area index and find the lon, lat, x, and y */
static void parse_index(long int index, struct ai *p) {
    p->lon = index >> 14;
    index -= p->lon << 14;
    p->lon -= 180;

    p->lat = index >> 6;
    index -= p->lat << 6;
    p->lat -= 90;

    p->y = index >> 3;
    index -= p->y << 3;

    p->x = index;
}


/* Build a path name from an area index */
static void gen_path(long int index, char *path) {
    struct ai p;
    int top_lon, top_lat, main_lon, main_lat;
    char hem, pole;

    parse_index(index, &p);

    path[0] = '\0';

    top_lon = p.lon / 10;
    main_lon = p.lon;
    if ( (p.lon < 0) && (top_lon * 10 != p.lon) ) {
	top_lon -= 1;
	main_lon -= 1;
    }
    top_lon *= 10;
    if ( top_lon >= 0 ) {
	hem = 'e';
    } else {
	hem = 'w';
	top_lon *= -1;
    }
    if ( main_lon < 0 ) {
	main_lon *= -1;
    }

    top_lat = p.lat / 10;
    main_lat = p.lat;
    if ( (p.lat < 0) && (top_lat * 10 != p.lat) ) {
	top_lat -= 1;
	main_lat -= 1;
    }
    top_lat *= 10;
    if ( top_lat >= 0 ) {
	pole = 'n';
    } else {
	pole = 's';
	top_lat *= -1;
    }
    if ( main_lat < 0 ) {
	main_lat *= -1;
    }

    sprintf(path, "%c%03d%c%03d/%c%03d%c%03d/%ld.ter", 
	    hem, top_lon, pole, top_lat,
	    hem, main_lon, pole, main_lat,
	    index);
}


/* offset an ai struct by the specified amounts in the X & Y direction */
static void offset_ai(struct ai *in, struct ai *out, int x, int y) {
    int diff, temp;
    int dist_lat;

    out->lon = in->lon;
    out->lat = in->lat;
    out->x = in->x;
    out->y = in->y;

    /* do X direction */
    diff = out->x + x;
    /* printf("      reducing x (%d)\n", diff); */
    if ( diff >= 0 ) {
	temp = diff / 8;
    } else if ( diff < -7 ) {
	temp = (diff + 1) / 8 - 1;
    } else {
	temp = diff / 8 - 1;
    }
    out->x = ((diff % 8) + 8) % 8;
    out->lon = ( (out->lon + 180 + 360 + temp) % 360 ) - 180;

    /* do Y direction */
    diff = out->y + y;
    /* printf("      reducing x (%d)\n", diff); */
    if ( diff >= 0 ) {
	temp = diff / 8;
    } else if ( diff < -7 ) {
	temp = (diff + 1) / 8 - 1;
    } else {
	temp = diff / 8 - 1;
    }
    out->y = ((diff % 8) + 8) % 8;
    out->lat = out->lat + temp;

    if ( out->lat >= 90 ) {
	dist_lat = out->lat - 90;
	/* printf("      +lat = %d  +y = %d\n", dist_lat, out->y); */
	
	out->lat = 90 - (dist_lat + 1);
	out->lon = ( (out->lon + 180 + 180) % 360 ) - 180;
	out->y = 7 - out->y;
    }

    if ( out->lat < -90 ) {
	dist_lat = -90 - out->lat;
	/* printf("      +lat = %d  +y = %d\n", dist_lat, out->y); */
	
	out->lat = -90 + (dist_lat - 1);
	out->lon = ( (out->lon + 180 + 180) % 360 ) - 180;
	out->y = 7 - out->y;
    }
}


/* Given a lat/lon, fill in the local area index array */
static void gen_idx_array(double lon, double lat) {
    struct ai p1, p2, p3;
    double diff;
    char path[256];
    long int index = 0;
    int i, j;

    printf("lon = %.2f  lat = %.2f\n", lon, lat);

    diff = lon - (double)(int)lon;
    printf("diff = %.2f\n", diff);
    if ( (lon >= 0) || (fabs(diff) < FG_EPSILON) ) {
	p1.lon = (int)lon;
    } else {
	p1.lon = (int)lon - 1;
    }
    printf("  p1.lon = %d\n", p1.lon);

    diff = lat - (double)(int)lat;
    printf("diff = %.2f\n", diff);
    if ( (lat >= 0) || (fabs(diff) < FG_EPSILON) ) {
	p1.lat = (int)lat;
    } else {
	p1.lat = (int)lat - 1;
    }
    printf("  p1.lat = %d\n", p1.lat);

    p1.x = (lon - p1.lon) * 8;
    p1.y = (lat - p1.lat) * 8;
    printf("  lon,lat = %d,%d  x,y index = %d,%d\n", 
	   p1.lon, p1.lat, p1.x, p1.y);

    for ( i = -3; i <= 3; i++ ) {
	for ( j = -3; j <= 3; j++ ) {
	    offset_ai(&p1, &p2, i, j);
	    printf("  %d %d %d %d\n", p2.lon, p2.lat, p2.x, p2.y);
	    area_array[i][j] = gen_index(&p2);
	    printf("  generated index = %ld\n", area_array[i][j]);

	    /* parse_index(area_array[i][j], &p3); */
	    /* printf("  %d %d %d %d\n", p3.lon, p3.lat, p3.x, p3.y); */
	    gen_path(area_array[i][j], path);
	    printf("  path = %s\n\n", path);
	}
    }
}


main() {
    struct ai p1;
    int i, j, index;
    double lon, lat;

    fg_srandom();

    p1.lon = 180;
    p1.lat = 90;
    p1.x = 7;
    p1.y = 7;

    /* printf("Max index = %ld\n", gen_index(&p1)); */

    lon = -50.0;
    lat = -50.0;
    gen_idx_array(lon, lat);

    lon = 50.0;
    lat = 50.0;
    gen_idx_array(lon, lat);

    for ( i = 0; i < 100; i++ ) {
	lon = ( 360.0 * fg_random() ) - 180.0;
	lat = ( 180.0 * fg_random() ) - 90.0;

	gen_idx_array(lon, lat);
    }
}


/* $Log$
/* Revision 1.2  1998/01/07 03:29:29  curt
/* Given an arbitrary lat/lon, we can now:
/*   generate a unique index for the chunk containing the lat/lon
/*   generate a path name to the chunk file
/*   build a list of the indexes of all the nearby areas.
/*
 * Revision 1.1  1998/01/07 02:05:48  curt
 * Initial revision.
 * */


