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
void rawDumpAsciiDEM( fgRAWDEM *raw, char *path, int ilon, int ilat ) {
    char outfile[256];
    char tmp[256];
    int lon, lat;
    char lon_sign, lat_sign;
    int i, j;
    FILE *fd;

    /* Generate output file name */

    if ( ilon >= 0 ) {
	lon = ilon;
	lon_sign = 'e';
    } else {
	lon = -ilon;
	lon_sign = 'w';
    }

    if ( ilat >= 0 ) {
	lat = ilat;
	lat_sign = 'n';
    } else {
	lat = -ilat;
	lat_sign = 's';
    }

    sprintf(outfile, "%s/%c%03d%c%03d.dem", path, lon_sign, lon, lat_sign, lat);

    printf("outfile = %s\n", outfile);

    if ( (fd = fopen(outfile, "w")) == NULL ) {
	printf("Error opening output file = %s\n", outfile);
	exit(-1);
    }

    /* Dump the "A" record */

    /* print descriptive header (144 characters) */
    sprintf(tmp, "%s - Generated from a 30 arcsec binary DEM", outfile);
    fprintf(fd, "%-144s", tmp);

    /* DEM level code, 3 reflects processing by DMA */
    fprintf(fd, "%6d", 1);

    /* Pattern code, 1 indicates a regular elevation pattern */
    fprintf(fd, "%6d", 1);

    /* Planimetric reference system code, 0 indicates geographic 
     * coordinate system. */
    fprintf(fd, "%6d", 0);

    /* Zone code */
    fprintf(fd, "%6d", 0);

    /* Map projection parameters (ignored) */
    for ( i = 0; i < 15; i++ ) {
        fprintf(fd, "%6.1f%18s", 0.0, "");
    }

   /* Units code, 3 represents arc-seconds as the unit of measure for
     * ground planimetric coordinates throughout the file. */
    fprintf(fd, "%6d", 3);

    /* Units code; 2 represents meters as the unit of measure for
     * elevation coordinates throughout the file. */
    fprintf(fd, "%6d", 2);

    /* Number (n) of sides in the polygon which defines the coverage of
     * the DEM file (usually equal to 4). */
    fprintf(fd, "%6d", 4);

    /* Ground coordinates of bounding box in arc-seconds */
    fprintf(fd, "%20.15fD+06", ilon * 3600.0 / 1000000.0);
    fprintf(fd, "%20.15fD+06", ilat * 3600.0 / 1000000.0);

    fprintf(fd, "%20.15fD+06", ilon * 3600.0 / 1000000.0);
    fprintf(fd, "%20.15fD+06", (ilat+1) * 3600.0 / 1000000.0);

    fprintf(fd, "%20.15fD+06", (ilon+1) * 3600.0 / 1000000.0);
    fprintf(fd, "%20.15fD+06", (ilat+1) * 3600.0 / 1000000.0);

    fprintf(fd, "%20.15fD+06", (ilon+1) * 3600.0 / 1000000.0);
    fprintf(fd, "%20.15fD+06", (ilat) * 3600.0 / 1000000.0);

    /* Minimum/maximum elevations in meters */
    fprintf(fd, "   %20.15E", (double)raw->tmp_min);
    fprintf(fd, "   %20.15E", (double)raw->tmp_max);

    /* Counterclockwise angle from the primary axis of ground
     * planimetric referenced to the primary axis of the DEM local
     * reference system. */
    fprintf(fd, "%6.1f", 0.0);

    /* Accuracy code; 0 indicates that a record of accuracy does not
     * exist and that no record type C will follow. */
    fprintf(fd, "%24d", 0);

    /* DEM spacial resolution.  Usually (3,3) (3,6) or (3,9)
     * depending on latitude */
    fprintf(fd, "%12.6E", 30.0);
    fprintf(fd, "%12.6E", 30.0);

    /* accuracy code */
    fprintf(fd, "%12.6E", 1.0);
    
    /* dimension of arrays to follow (1)*/
    fprintf(fd, "%6d", 1);

    /* number of profiles */
    fprintf(fd, "%6d", 3600 / raw->ydim + 1);

    /* pad the end */
    fprintf(fd, "%160s", "");


    /* Dump "B" records */

    for ( j = 0; j <= 120; j++ ) {
	/* row / column id of this profile */
	fprintf(fd, "%6d%6d", 1, j + 1);

	/* Number of columns and rows (elevations) in this profile */
	fprintf(fd, "%6d%6d", 3600 / raw->xdim + 1, 1);

	/* Ground planimetric coordinates (arc-seconds) of the first
	 * elevation in the profile */
	fprintf(fd, "%20.15fD+06", ilon * 3600.0 / 1000000.0);
	fprintf(fd, "%20.15fD+06", (ilat * 3600.0 + j * raw->ydim) / 1000000.0);

	/* Elevation of local datum for the profile.  Always zero for
	 * 1-degree DEM, the reference is mean sea level. */
	fprintf(fd, "%6.1f", 0.0);
	fprintf(fd, "%18s", "");

	/* Minimum and maximum elevations for the profile. */
	fprintf(fd, "   %20.15E", 0.0);
	fprintf(fd, "   %20.15E", 0.0);

	/* One (usually) dimensional array (prof_num_cols,1) of elevations */
	for ( i = 0; i <= 120; i++ ) {
	    fprintf(fd, "%6.0f", raw->edge[j][i]);
	}
    }

    fprintf(fd, "\n");

    fclose(fd);
}


/* Read a horizontal strip of (1 vertical degree) from the raw DEM
 * file specified by the upper latitude of the stripe specified in
 * degrees.  The output the individual ASCII format DEM tiles.  */
void rawProcessStrip( fgRAWDEM *raw, int lat_degrees, char *path ) {
    int lat, yrange;
    int i, j, index, row, col;
    int min, max;
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
	}
    }

    /* extract individual tiles from the strip */
    span = raw->ncols * raw->xdim;
    num_degrees = span / 3600;
    tile_width = raw->ncols / num_degrees;
    printf("span = %d  num_degrees = %d  width = %d\n", 
	   span, num_degrees, tile_width);

    for ( i = 0; i < num_degrees; i++ ) {
	xstart = i * tile_width;
	xend = xstart + 120;

	min = 10000; max = -10000;
	for ( row = 0; row < yrange; row++ ) {
	    for ( col = xstart; col < xend; col++ ) {
		/* Copy from strip to pixel centered tile.  Yep,
                 * row/col are reversed here.  raw->strip is backwards
                 * for convenience.  I am converting to [x,y] now. */
		raw->center[col-xstart][row] = raw->strip[row][col];
	
		if ( raw->strip[row][col] < min) {
		    min = raw->strip[row][col];
		}
		
		if ( raw->strip[row][col] > max) {
		    max = raw->strip[row][col];
		}
	    }
	}

	raw->tmp_min = min;
	raw->tmp_max = max;

	/* Convert from pixel centered to pixel edge values */
	rawConvertCenter2Edge(raw);

	/* Dump out the ascii format DEM file */
	rawDumpAsciiDEM(raw, path, (raw->rootx / 3600) + i, lat_degrees - 1);
    }
}


/* $Log$
/* Revision 1.3  1998/03/03 13:10:29  curt
/* Close to a working version.
/*
 * Revision 1.2  1998/03/03 02:04:01  curt
 * Starting DEM Ascii format output routine.
 *
 * Revision 1.1  1998/03/02 23:31:01  curt
 * Initial revision.
 *
 */
