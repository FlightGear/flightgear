/* rawdem.c -- library of routines for processing raw dem files (30 arcsec)
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


#include <math.h>      /* rint() */
#include <stdio.h>
#include <stdlib.h>    /* atoi() atof() */
#include <string.h>    /* swab() */

#include <sys/types.h> /* open() */
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>    /* close() */

#include "rawdem.h"


/* Read the DEM header to determine various key parameters for this
 * DEM file */
void rawReadDemHdr( fgRAWDEM *raw, char *hdr_file ) {
    FILE *hdr;
    char line[256], key[256], value[256];
    int i, len, offset;
    double tmp;

    if ( (hdr = fopen(hdr_file, "r")) == NULL ) {
	printf("Error opening DEM header file: %s\n", hdr_file);
	exit(-1);
    }

    /* process each line */
    while ( (fgets(line, 256, hdr) != NULL) ) {
	/* printf("%s", line); */
	len = strlen(line);

	/* extract key */
	i = 0;
	while ( (line[i] != ' ') && (i < len) ) {
	    key[i] = line[i];
	    i++;
	}
	key[i] = '\0';

	/* skip middle space */
	while ( (line[i] == ' ') && (i < len) ) {
	    i++;
	}
	offset = i;

	/* extract value */
	while ( (line[i] != '\n') && (i < len) ) {
	    value[i-offset] = line[i];
	    i++;
	}
	value[i-offset] = '\0';
	/* printf("key='%s'  value='%s'\n", key, value); */

	if ( strcmp(key, "NROWS") == 0 ) {
	    raw->nrows = atoi(value);
	} else if ( strcmp(key, "NCOLS") == 0 ) {
	    raw->ncols = atoi(value);
	} else if ( strcmp(key, "ULXMAP") == 0 ) {
	    tmp = atof(value);
	    raw->ulxmap = (int)rint(tmp * 3600.0); /* convert to arcsec */
	} else if ( strcmp(key, "ULYMAP") == 0 ) {
	    tmp = atof(value);
	    raw->ulymap = (int)rint(tmp * 3600.0); /* convert to arcsec */
	} else if ( strcmp(key, "XDIM") == 0 ) {
	    tmp = atof(value);
	    raw->xdim = (int)rint(tmp * 3600.0);   /* convert to arcsec */
	} else if ( strcmp(key, "YDIM") == 0 ) {
	    tmp = atof(value);
	    raw->ydim = (int)rint(tmp * 3600.0);   /* convert to arcsec */
	} else {
	    /* ignore for now */
	}
    }

    raw->rootx = raw->ulxmap - (raw->xdim / 2);
    raw->rooty = raw->ulymap + (raw->ydim / 2);

    printf("%d %d %d %d %d %d %d %d\n", raw->nrows, raw->ncols,
           raw->ulxmap, raw->ulymap, raw->rootx, raw->rooty, raw->xdim,
	   raw->ydim);
}


/* Open a raw DEM file. */
void rawOpenDemFile( fgRAWDEM *raw, char *raw_dem_file ) {
    printf("Opening Raw DEM file: %s\n", raw_dem_file);
    if ( (raw->fd = open(raw_dem_file ,O_RDONLY)) == -1 ) {
	printf("Error opening Raw DEM file: %s\n", raw_dem_file);
	exit(-1);
    }
}


/* Close a raw DEM file. */
void rawCloseDemFile( fgRAWDEM *raw ) {
    close(raw->fd);
}


/* Advance file pointer position to correct latitude (row) */
void rawAdvancePosition( fgRAWDEM *raw, int arcsec ) {
    long offset, result;

    offset = 2 * raw->ncols * ( arcsec  / raw->ydim );

    if ( (result = lseek(raw->fd, offset, SEEK_SET)) == -1 ) {
	printf("Error lseek filed trying to offset by %ld\n", offset);
	exit(-1);
    }

    printf("Successful seek ahead of %ld bytes\n", result);
}


/* Read the next row of data */
void rawReadNextRow( fgRAWDEM *raw, int index ) {
    char buf[MAX_COLS_X_2];
    int i, result;

    if ( raw->ncols > MAX_ROWS ) {
	printf("Error, buf needs to be bigger in rawReadNextRow()\n");
	exit(-1);
    }

    /* printf("Attempting to read %d bytes\n", 2 * raw->ncols); */
    result = read(raw->fd, buf, 2 * raw->ncols);
    /* printf("Read %d bytes\n", result); */

    /* reverse byte order */
    /* it would be nice to test in advance some how if we need to do
     * this */
    /* swab(frombuf, tobuf, 2 * raw->ncols); */

    for ( i = 0; i < raw->ncols; i++ ) {
	/* printf("hi = %d  lo = %d\n", buf[2*i], buf[2*i + 1]); */
	raw->strip[index][i] = ( (buf[2*i] + 1) << 8 ) + buf[2*i + 1];
    }
}


/* Convert from pixel centered values to pixel corner values.  This is
   accomplished by taking the average of the closes center nodes.  In
   the following diagram "x" marks the data point location:

   +-----+        x-----x
   |     |        |     |
   |  x  |  ===>  |     |
   |     |        |     |
   +-----+        x-----x
   
   */
void rawConvertCenter2Edge( fgRAWDEM *raw ) {
    int i, j;

    /* derive corner nodes */
    raw->edge[0][0]     = raw->center[0][0];
    raw->edge[120][0]   = raw->center[119][0];
    raw->edge[120][120] = raw->center[119][119];
    raw->edge[0][120]   = raw->center[0][119];

    /* derive edge nodes */
    for ( i = 1; i < 120; i++ ) {
	raw->edge[i][0] = (raw->center[i-1][0] + raw->center[i][0]) / 2.0;
	raw->edge[i][120] = (raw->center[i-1][119] + raw->center[i][119]) / 2.0;
	raw->edge[0][i] = (raw->center[0][i-1] + raw->center[0][i]) / 2.0;
	raw->edge[120][i] = (raw->center[119][i-1] + raw->center[119][i]) / 2.0;
    }

    /* derive internal nodes */
    for ( j = 1; j < 120; j++ ) {
	for ( i = 1; i < 120; i++ ) {
	    raw->edge[i][j] = ( raw->center[i-1][j-1] + 
				raw->center[i]  [j-1] +
				raw->center[i]  [j]   +
				raw->center[i-1][j] ) / 4;
	}
    }
}


/* Dump out the ascii format DEM file */
void rawDumpAsciiDEM( fgRAWDEM *raw ) {
}


/* Read a horizontal strip of (1 vertical degree) from the raw DEM
 * file specified by the upper latitude of the stripe specified in
 * degrees */
void rawReadStrip( fgRAWDEM *raw, int lat_degrees ) {
    int lat, yrange;
    int i, j, index, row, col;
    int min = 0, max = 0;
    int span, num_degrees, tile_width;
    int xstart, xend;

    /* convert to arcsec */
    lat = lat_degrees * 3600;

    printf("Max Latitude = %d arcsec\n", lat);

    /* validity check ... */
    if ( (lat > raw->rooty) || 
	 (lat < (raw->rooty - raw->nrows * raw->ydim + 1)) ) {
	printf("Latitude out of range for this DEM file\n");
	return;
    }

    printf ("Reading strip starting at %d (top and working down)\n", lat);

    /* advance to the correct latitude */
    rawAdvancePosition(raw, (raw->rooty - lat));

    /* printf("short = %d\n", sizeof(short)); */

    yrange = 3600 / raw->ydim;

    for ( i = 0; i < yrange; i++ ) {
	index = yrange - i - 1;
	/* printf("About to read into row %d\n", index); */
	rawReadNextRow(raw, index);

	for ( j = 0; j < raw->ncols; j++ ) {
	    if ( raw->strip[index][j] == -9999 ) {
		/* map ocean to 0 for now */
		raw->strip[index][j] = 0;
	    } 

	    if ( raw->strip[index][j] < min) {
		min = raw->strip[index][j];
	    }

	    if ( raw->strip[index][j] > max) {
		max = raw->strip[index][j];
	    }
	}
    }
    printf("min = %d  max = %d\n", min, max);

    /* extract individual tiles from the strip */
    span = raw->ncols * raw->xdim;
    num_degrees = span / 3600;
    tile_width = raw->ncols / num_degrees;
    printf("span = %d  num_degrees = %d  width = %d\n", 
	   span, num_degrees, tile_width);

    for ( i = 0; i < num_degrees; i++ ) {
	xstart = i * tile_width;
	xend = xstart + 120;

	for ( row = 0; row < yrange; row++ ) {
	    for ( col = xstart; col < xend; col++ ) {
		/* Copy from strip to pixel centered tile.  Yep,
                 * row/col are reversed here.  raw->strip is backwards
                 * for convenience.  I am converting to [x,y] now. */
		raw->center[col-xstart][row] = raw->strip[row][col];
	    }
	}

	/* Convert from pixel centered to pixel edge values */
	rawConvertCenter2Edge(raw);

	/* Dump out the ascii format DEM file */
	rawDumpAsciiDEM(raw);
    }
}


/* $Log$
/* Revision 1.1  1998/03/02 23:31:01  curt
/* Initial revision.
/*
 */
