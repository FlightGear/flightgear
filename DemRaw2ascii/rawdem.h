/* rawdem.h -- library of routines for processing raw dem files (30 arcsec)
 *
 * Written by Curtis Olson, started February 1998.
 *
 * Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 */


#ifndef _RAWDEM_H
#define _RAWDEM_H


#define MAX_ROWS 6000
#define MAX_COLS 7200
#define MAX_COLS_X_2 14400

typedef struct {
    /* header info */
    int nrows;       /* number of rows */
    int ncols;       /* number of cols */
    int ulxmap;      /* X coord of center of upper left pixel in arcsec */
    int ulymap;      /* Y coord of center of upper left pixel in arcsec */
    int rootx;       /* X coord of upper left *edge* of DEM region in degrees */
    int rooty;       /* Y coord of upper left *edge* of DEM region in degrees */
    int xdim;        /* X dimension of a pixel */
    int ydim;        /* Y dimension of a pixel */
    int tmp_min;     /* current 1x1 degree tile minimum */
    int tmp_max;     /* current 1x1 degree tile maximum */

    /* file ptr */
    int fd;          /* Raw DEM file descriptor */

    /* storage area for a 1 degree high strip of data.  Note, for
     * convenience this is in y,x order */
    short strip[120][MAX_ROWS];

    short center[120][120];  /* tile with data taken at center of pixel */
    float edge[121][121];    /* tile with data converted to corners */
} fgRAWDEM;


/* Read the DEM header to determine various key parameters for this
 * DEM file */
void rawReadDemHdr( fgRAWDEM *raw, char *hdr_file );

/* Open a raw DEM file. */
void rawOpenDemFile( fgRAWDEM *raw, char *raw_dem_file );

/* Close a raw DEM file. */
void rawCloseDemFile( fgRAWDEM *raw );

/* Read a horizontal strip of (1 vertical degree) from the raw DEM
 * file specified by the upper latitude of the stripe specified in
 * degrees.  The output the individual ASCII format DEM tiles.  */
void rawProcessStrip( fgRAWDEM *raw, int lat_degrees, char *path );


#endif /* _RAWDEM_H */


/* $Log$
/* Revision 1.2  1998/03/03 13:10:30  curt
/* Close to a working version.
/*
 * Revision 1.1  1998/03/02 23:31:02  curt
 * Initial revision.
 *
 */
