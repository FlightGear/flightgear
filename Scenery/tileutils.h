/**************************************************************************
 * tileutils.h -- support routines to handle dynamic management of scenery tiles
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


struct bucket {
    int lon;  /* longitude (-180 to 179) */
    int lat;  /* latitude (-90 to 89) */
    int x;    /* x (0 to 7) */
    int y;    /* y (0 to 7) */
};


/* Generate the unique scenery tile index containing the specified
   lon/lat parameters.

   The index is constructed as follows:

   9 bits - to represent 360 degrees of longitude (-180 to 179)
   8 bits - to represent 180 degrees of latitude (-90 to 89)

   Each 1 degree by 1 degree tile is further broken down into an 8x8
   grid.  So we also need:

   3 bits - to represent x (0 to 7)
   3 bits - to represent y (0 to 7) */
/* static long gen_index(struct bucket *p); */


/* Parse a unique scenery tile index and find the lon, lat, x, and y */
/* static void parse_index(long int index, struct bucket *p); */


/* Build a path name from an tile index */
void gen_path(long int index, char *path);


/* offset an bucket struct by the specified amounts in the X & Y direction */
/* static void offset_bucket(struct bucket *in, struct bucket *out, int x, int y); */


/* Given a lat/lon, find the "bucket" or tile that it falls within */
void find_bucket(double lon, double lat, struct bucket *p);


/* Given a lat/lon, fill in the local tile index array */
void gen_idx_array(struct bucket *p1, long int *tiles, 
			  int width, int height);


/* $Log$
/* Revision 1.2  1998/01/08 02:22:28  curt
/* Continue working on basic features.
/*
 * Revision 1.1  1998/01/07 23:50:52  curt
 * "area" renamed to "tile"
 *
 * Revision 1.1  1998/01/07 23:23:40  curt
 * Initial revision.
 *
 * */


