/**************************************************************************
 * tileutils.c -- support routines to handle dynamic management of scenery tiles
 *
 * Written by Curtis Olson, started January 1998.
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


#include <math.h>
#include <stdio.h>

#include "tileutils.h"
#include "../Include/constants.h"


/* Generate the unique scenery tile index containing the specified
   lon/lat parameters.

   The index is constructed as follows:

   9 bits - to represent 360 degrees of longitude (-180 to 179)
   8 bits - to represent 180 degrees of latitude (-90 to 89)

   Each 1 degree by 1 degree tile is further broken down into an 8x8
   grid.  So we also need:

   3 bits - to represent x (0 to 7)
   3 bits - to represent y (0 to 7) */
static long gen_index(struct bucket *p) {
    long index = 0;

    index = ((p->lon + 180) << 14) + ((p->lat + 90) << 6) + (p->y << 3) + p->x;
    /* printf("  generated index = %ld\n", index); */

    return(index);
}


/* Parse a unique scenery tile index and find the lon, lat, x, and y */
static void parse_index(long int index, struct bucket *p) {
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


/* Build a path name from an tile index */
void gen_path(long int index, char *path) {
    struct bucket p;
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


/* offset an bucket struct by the specified amounts in the X & Y direction */
static void offset_bucket(struct bucket *in, struct bucket *out, int x, int y) {
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


/* Given a lat/lon, find the "bucket" or tile that it falls within */
void find_bucket(double lon, double lat, struct bucket *p) {
    double diff;

    diff = lon - (double)(int)lon;
    /* printf("diff = %.2f\n", diff); */
    if ( (lon >= 0) || (fabs(diff) < FG_EPSILON) ) {
	p->lon = (int)lon;
    } else {
	p->lon = (int)lon - 1;
    }
    /* printf("  p->lon = %d\n", p->lon); */

    diff = lat - (double)(int)lat;
    /* printf("diff = %.2f\n", diff); */
    if ( (lat >= 0) || (fabs(diff) < FG_EPSILON) ) {
	p->lat = (int)lat;
    } else {
	p->lat = (int)lat - 1;
    }
    /* printf("  p->lat = %d\n", p->lat); */

    p->x = (lon - p->lon) * 8;
    p->y = (lat - p->lat) * 8;
    printf("Bucket = lon,lat = %d,%d  x,y index = %d,%d\n", 
	   p->lon, p->lat, p->x, p->y);
}


/* Given a lat/lon, fill in the local tile index array */
void gen_idx_array(struct bucket *p1, long int *tile, 
			  int width, int height) {
    struct bucket p2;
    int dw, dh, i, j;

    dh = height / 2;
    dw = width / 2;
    for ( j = 0; j < height; j++ ) {
	for ( i = 0; i < width; i++ ) {
	    offset_bucket(p1, &p2, i - dw, j - dh);
	    tile[(j*width)+i] = gen_index(&p2);
	    printf("  bucket = %d %d %d %d  index = %ld\n", 
		   p2.lon, p2.lat, p2.x, p2.y, tile[(j*width)+i]);
	}
    }
}


/* sample main for testing
int main() {
    struct bucket p1;
    long int tile[49];
    char path[256];
    double lon, lat;
    int i, j;

    p1.lon = 180;
    p1.lat = 90;
    p1.x = 7;
    p1.y = 7;

    printf("Max index = %ld\n", gen_index(&p1));

    lon = -50.0;
    lat = -50.0;
    find_bucket(lon, lat, &p1);
    gen_idx_array(&p1, tile, 7, 7);
    for ( j = 0; j < 7; j++ ) {
	for ( i = 0; i < 7; i++ ) {
	    gen_path(tile[(j*7)+i], path);
	    printf("  path = %s\n", path);
	}
    }

    lon = 50.0;
    lat = 50.0;
    find_bucket(lon, lat, &p1);
    gen_idx_array(&p1, tile, 7, 7);
    for ( j = 0; j < 7; j++ ) {
	for ( i = 0; i < 7; i++ ) {
	    gen_path(tile[(j*7)+i], path);
	    printf("  path = %s\n", path);
	}
    }

    return(1);
} */


/* $Log$
/* Revision 1.1  1998/01/07 23:50:52  curt
/* "area" renamed to "tile"
/*
 * Revision 1.1  1998/01/07 23:23:40  curt
 * Initial revision.
 *
 * */


