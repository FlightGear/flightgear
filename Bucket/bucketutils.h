/**************************************************************************
 * bucketutils.h -- support routines to handle fgBUCKET operations
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


#ifndef _BUCKETUTILS_H
#define _BUCKETUTILS_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


struct fgBUCKET {
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
long int fgBucketGenIndex(struct fgBUCKET *p);


/* Parse a unique scenery tile index and find the lon, lat, x, and y */
void fgBucketParseIndex(long int index, struct fgBUCKET *p);


/* Build a path name from an tile index */
void fgBucketGenBasePath(struct fgBUCKET *p, char *path);


/* offset an bucket struct by the specified amounts in the X & Y direction */
void fgBucketOffset(struct fgBUCKET *in, struct fgBUCKET *out, int x, int y);


/* Given a lat/lon, find the "bucket" or tile that it falls within */
void fgBucketFind(double lon, double lat, struct fgBUCKET *p);


/* Given a lat/lon, fill in the local tile index array */
void fgBucketGenIdxArray(struct fgBUCKET *p1, struct fgBUCKET *tiles,
			  int width, int height);


#ifdef __cplusplus
}
#endif


#endif /* _BUCKETUTILS_H */


/* $Log$
/* Revision 1.1  1998/04/08 23:28:59  curt
/* Adopted Gnu automake/autoconf system.
/*
 * Revision 1.2  1998/01/24 00:03:28  curt
 * Initial revision.
 *
 * Revision 1.1  1998/01/23 20:06:52  curt
 * tileutils.* renamed to bucketutils.*
 *
 * Revision 1.6  1998/01/22 02:59:42  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.5  1998/01/14 02:19:05  curt
 * Makde offset_bucket visible to outside.
 *
 * Revision 1.4  1998/01/13 00:23:12  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.3  1998/01/10 00:01:48  curt
 * Misc api changes and tweaks.
 *
 * Revision 1.2  1998/01/08 02:22:28  curt
 * Continue working on basic features.
 *
 * Revision 1.1  1998/01/07 23:50:52  curt
 * "area" renamed to "tile"
 *
 * Revision 1.1  1998/01/07 23:23:40  curt
 * Initial revision.
 *
 * */


