// -*- Mode: C++ -*-
//
// dem.c -- DEM management class
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#include <ctype.h>    // isspace()
#include <math.h>     // rint()
#include <stdio.h>
#include <stdlib.h>   // atoi()
#include <string.h>
#include <sys/stat.h> // stat()
#include <unistd.h>   // stat()

#include <zlib/zlib.h>

#include "dem.hxx"
#include "leastsqs.hxx"

#include <Include/fg_constants.h>


#ifdef WIN32
#  define MKDIR(a) mkdir(a,S_IRWXU)     // I am just guessing at this flag (NHV)
#endif // WIN32


fgDEM::fgDEM( void ) {
    // printf("class fgDEM CONstructor called.\n");
    dem_data = new float[DEM_SIZE_1][DEM_SIZE_1];
    output_data = new float[DEM_SIZE_1][DEM_SIZE_1];
}


#ifdef WIN32

// return the file path name ( foo/bar/file.ext = foo/bar )
void extract_path (char *in, char *base) {
    int len, i;
    
    len = strlen (in);
    strcpy (base, in);

    i = len - 1;
    while ( (i >= 0) && (in[i] != '/') ) {
	i--;
    }

    base[i] = '\0';
}


// Make a subdirectory
int my_mkdir (char *dir) {
    struct stat stat_buf;
    int result;

    printf ("mk_dir() ");

    result = stat (dir, &stat_buf);

    if (result != 0) {
	MKDIR (dir);
	result = stat (dir, &stat_buf);
	if (result != 0) {
	    printf ("problem creating %s\n", dir);
	} else {
	    printf ("%s created\n", dir);
	}
    } else {
	printf ("%s already exists\n", dir);
    }

    return (result);
}

#endif // WIN32


// open a DEM file
int fgDEM::open ( char *file ) {
    // open input file (or read from stdin)
    if ( strcmp(file, "-") == 0 ) {
	printf("Loading DEM data file: stdin\n");
	// fd = stdin;
	fd = gzdopen(STDIN_FILENO, "r");
    } else {
	if ( (fd = gzopen(file, "rb")) == NULL ) {
	    printf("Cannot gzopen %s\n", file);
	    return(0);
	}
	printf("Loading DEM data file: %s\n", file);
    }

    return(1);
}


// close a DEM file
int fgDEM::close ( void ) {
    gzclose(fd);

    return(1);
}


// return next token from input stream
static void next_token(gzFile fd, char *token) {
    int i, result;
    char c;

    i = 0;
    c = gzgetc(fd);
    // skip past spaces
    while ( (c != -1) && (c == ' ') ) {
	c = gzgetc(fd);
    }
    while ( (c != -1) && (c != ' ') && (c != '\n') ){
	token[i] = c;
	i++;
	c = gzgetc(fd);
    }
    token[i] = '\0';

    if ( c == -1 ) {
	strcpy(token, "__END_OF_FILE__");
	printf("    Warning:  Reached end of file!\n");
    }

    // printf("    returning %s\n", token);
}


// return next integer from input stream
static int next_int(gzFile fd) {
    char token[80];

    next_token(fd, token);
    return ( atoi(token) );
}


// return next double from input stream
static double next_double(gzFile fd) {
    char token[80];

    next_token(fd, token);
    return ( atof(token) );
}


// return next exponential num from input stream
static int next_exp(gzFile fd) {
    char token[80];
    double mantissa;
    int exp, acc;
    int i;

    next_token(fd, token);

    sscanf(token, "%lfD%d", &mantissa, &exp);

    // printf("    Mantissa = %.4f  Exp = %d\n", mantissa, exp);

    acc = 1;
    if ( exp > 0 ) {
	for ( i = 1; i <= exp; i++ ) {
	    acc *= 10;
	}
    } else if ( exp < 0 ) {
	for ( i = -1; i >= exp; i-- ) {
	    acc /= 10;
	}
    }

    return( (int)rint(mantissa * (double)acc) );
}


// read and parse DEM "A" record
int fgDEM::read_a_record( void ) {
    int i, inum;
    double dnum;
    char name[144];
    char token[80];
    char *ptr;

    // get the name field (144 characters)
    for ( i = 0; i < 144; i++ ) {
	name[i] = gzgetc(fd);
    }
    name[i+1] = '\0';

    // clean off the whitespace at the end
    for ( i = strlen(name)-2; i > 0; i-- ) {
	if ( !isspace(name[i]) ) {
	    i=0;
	} else {
	    name[i] = '\0'; 
	}
    }
    printf("    Quad name field: %s\n", name);

    // DEM level code, 3 reflects processing by DMA
    inum = next_int(fd);
    printf("    DEM level code = %d\n", inum);

    if ( inum > 3 ) {
	return(0);
    }

    // Pattern code, 1 indicates a regular elevation pattern
    inum = next_int(fd);
    printf("    Pattern code = %d\n", inum);

    // Planimetric reference system code, 0 indicates geographic
    // coordinate system.
    inum = next_int(fd);
    printf("    Planimetric reference code = %d\n", inum);

    // Zone code
    inum = next_int(fd);
    printf("    Zone code = %d\n", inum);

    // Map projection parameters (ignored)
    for ( i = 0; i < 15; i++ ) {
	dnum = next_double(fd);
	// printf("%d: %f\n",i,dnum);
    }

    // Units code, 3 represents arc-seconds as the unit of measure for
    // ground planimetric coordinates throughout the file.
    inum = next_int(fd);
    if ( inum != 3 ) {
	printf("    Unknown (X,Y) units code = %d!\n", inum);
	exit(-1);
    }

    // Units code; 2 represents meters as the unit of measure for
    // elevation coordinates throughout the file.
    inum = next_int(fd);
    if ( inum != 2 ) {
	printf("    Unknown (Z) units code = %d!\n", inum);
	exit(-1);
    }

    // Number (n) of sides in the polygon which defines the coverage of
    // the DEM file (usually equal to 4).
    inum = next_int(fd);
    if ( inum != 4 ) {
	printf("    Unknown polygon dimension = %d!\n", inum);
	exit(-1);
    }

    // Ground coordinates of bounding box in arc-seconds
    dem_x1 = originx = next_exp(fd);
    dem_y1 = originy = next_exp(fd);
    printf("    Origin = (%.2f,%.2f)\n", originx, originy);

    dem_x2 = next_exp(fd);
    dem_y2 = next_exp(fd);

    dem_x3 = next_exp(fd);
    dem_y3 = next_exp(fd);

    dem_x4 = next_exp(fd);
    dem_y4 = next_exp(fd);

    // Minimum/maximum elevations in meters
    dem_z1 = next_exp(fd);
    dem_z2 = next_exp(fd);
    printf("    Elevation range %.4f %.4f\n", dem_z1, dem_z2);

    // Counterclockwise angle from the primary axis of ground
    // planimetric referenced to the primary axis of the DEM local
    // reference system.
    next_token(fd, token);

    // Accuracy code; 0 indicates that a record of accuracy does not
    // exist and that no record type C will follow.

    // DEM spacial resolution.  Usually (3,3,1) (3,6,1) or (3,9,1)
    // depending on latitude

    // I will eventually have to do something with this for data at
    // higher latitudes */
    next_token(fd, token);
    printf("    accuracy & spacial resolution string = %s\n", token);
    i = strlen(token);
    printf("    length = %d\n", i);

    ptr = token + i - 12;
    printf("    last field = %s = %.2f\n", ptr, atof(ptr));
    ptr[0] = '\0';

    ptr = ptr - 12;
    col_step = atof(ptr);
    printf("    last field = %s = %.2f\n", ptr, col_step);
    ptr[0] = '\0';

    ptr = ptr - 12;
    row_step = atof(ptr);
    printf("    last field = %s = %.2f\n", ptr, row_step);
    ptr[0] = '\0';

    // accuracy code = atod(token)
    inum = atoi(token);
    printf("    Accuracy code = %d\n", inum);

    printf("    column step = %.2f  row step = %.2f\n", 
	   col_step, row_step);
    // dimension of arrays to follow (1)
    next_token(fd, token);

    // number of profiles
    dem_num_profiles = cols = next_int(fd);
    printf("    Expecting %d profiles\n", dem_num_profiles);

    return(1);
}


// read and parse DEM "B" record
void fgDEM::read_b_record( void ) {
    char token[80];
    int i;

    // row / column id of this profile
    prof_row = next_int(fd);
    prof_col = next_int(fd);
    // printf("col id = %d  row id = %d\n", prof_col, prof_row);

    // Number of columns and rows (elevations) in this profile
    prof_num_rows = rows = next_int(fd);
    prof_num_cols = next_int(fd);
    // printf("    profile num rows = %d\n", prof_num_rows);

    // Ground planimetric coordinates (arc-seconds) of the first
    // elevation in the profile
    prof_x1 = next_exp(fd);
    prof_y1 = next_exp(fd);
    // printf("    Starting at %.2f %.2f\n", prof_x1, prof_y1);

    // Elevation of local datum for the profile.  Always zero for
    // 1-degree DEM, the reference is mean sea level.
    next_token(fd, token);

    // Minimum and maximum elevations for the profile.
    next_token(fd, token);
    next_token(fd, token);

    // One (usually) dimensional array (prof_num_cols,1) of elevations
    for ( i = 0; i < prof_num_rows; i++ ) {
	prof_data = next_int(fd);
	dem_data[cur_col][i] = (float)prof_data;
    }
}


// parse dem file
int fgDEM::parse( void ) {
    int i;

    cur_col = 0;

    if ( !read_a_record() ) {
	return(0);
    }

    for ( i = 0; i < dem_num_profiles; i++ ) {
	// printf("Ready to read next b record\n");
	read_b_record();
	cur_col++;

	if ( cur_col % 100 == 0 ) {
	    printf("    loaded %d profiles of data\n", cur_col);
	}
    }

    printf("    Done parsing\n");

    return(1);
}


// return the current altitude based on mesh data.  We should rewrite
// this to interpolate exact values, but for now this is good enough
double fgDEM::interpolate_altitude( double lon, double lat ) {
    // we expect incoming (lon,lat) to be in arcsec for now

    double xlocal, ylocal, dx, dy, zA, zB, elev;
    int x1, x2, x3, y1, y2, y3;
    float z1, z2, z3;
    int xindex, yindex;

    /* determine if we are in the lower triangle or the upper triangle 
       ______
       |   /|
       |  / |
       | /  |
       |/   |
       ------

       then calculate our end points
     */

    xlocal = (lon - originx) / col_step;
    ylocal = (lat - originy) / row_step;

    xindex = (int)(xlocal);
    yindex = (int)(ylocal);

    // printf("xindex = %d  yindex = %d\n", xindex, yindex);

    if ( xindex + 1 == cols ) {
	xindex--;
    }

    if ( yindex + 1 == rows ) {
	yindex--;
    }

    if ( (xindex < 0) || (xindex + 1 >= cols) ||
	 (yindex < 0) || (yindex + 1 >= rows) ) {
	return(-9999);
    }

    dx = xlocal - xindex;
    dy = ylocal - yindex;

    if ( dx > dy ) {
	// lower triangle
	// printf("  Lower triangle\n");

	x1 = xindex; 
	y1 = yindex; 
	z1 = dem_data[x1][y1];

	x2 = xindex + 1; 
	y2 = yindex; 
	z2 = dem_data[x2][y2];
				  
	x3 = xindex + 1; 
	y3 = yindex + 1; 
	z3 = dem_data[x3][y3];

	// printf("  dx = %.2f  dy = %.2f\n", dx, dy);
	// printf("  (x1,y1,z1) = (%d,%d,%d)\n", x1, y1, z1);
	// printf("  (x2,y2,z2) = (%d,%d,%d)\n", x2, y2, z2);
	// printf("  (x3,y3,z3) = (%d,%d,%d)\n", x3, y3, z3);

	zA = dx * (z2 - z1) + z1;
	zB = dx * (z3 - z1) + z1;
	
	// printf("  zA = %.2f  zB = %.2f\n", zA, zB);

	if ( dx > FG_EPSILON ) {
	    elev = dy * (zB - zA) / dx + zA;
	} else {
	    elev = zA;
	}
    } else {
	// upper triangle
	// printf("  Upper triangle\n");

	x1 = xindex; 
	y1 = yindex; 
	z1 = dem_data[x1][y1];

	x2 = xindex; 
	y2 = yindex + 1; 
	z2 = dem_data[x2][y2];
				  
	x3 = xindex + 1; 
	y3 = yindex + 1; 
	z3 = dem_data[x3][y3];

	// printf("  dx = %.2f  dy = %.2f\n", dx, dy);
	// printf("  (x1,y1,z1) = (%d,%d,%d)\n", x1, y1, z1);
	// printf("  (x2,y2,z2) = (%d,%d,%d)\n", x2, y2, z2);
	// printf("  (x3,y3,z3) = (%d,%d,%d)\n", x3, y3, z3);
 
	zA = dy * (z2 - z1) + z1;
	zB = dy * (z3 - z1) + z1;
	
	// printf("  zA = %.2f  zB = %.2f\n", zA, zB );
	// printf("  xB - xA = %.2f\n", col_step * dy / row_step);

	if ( dy > FG_EPSILON ) {
	    elev = dx * (zB - zA) / dy    + zA;
	} else {
	    elev = zA;
	}
    }

    return(elev);
}


// Use least squares to fit a simpler data set to dem data
void fgDEM::fit( double error, fgBUCKET *p ) {
    double x[DEM_SIZE_1], y[DEM_SIZE_1];
    double m, b, ave_error, max_error;
    double cury, lasty;
    int n, row, start, end, good_fit;
    int colmin, colmax, rowmin, rowmax;
    // FILE *dem, *fit, *fit1;

    printf("Initializing output mesh structure\n");
    outputmesh_init();

    // determine dimensions
    colmin = p->x * ( (cols - 1) / 8);
    colmax = colmin + ( (cols - 1) / 8);
    rowmin = p->y * ( (rows - 1) / 8);
    rowmax = rowmin + ( (rows - 1) / 8);
    printf("Fitting region = %d,%d to %d,%d\n", colmin, rowmin, colmax, rowmax);
    
    // include the corners explicitly
    outputmesh_set_pt(colmin, rowmin, dem_data[colmin][rowmin]);
    outputmesh_set_pt(colmin, rowmax, dem_data[colmin][rowmax]);
    outputmesh_set_pt(colmax, rowmax, dem_data[colmax][rowmax]);
    outputmesh_set_pt(colmax, rowmin, dem_data[colmax][rowmin]);

    printf("Beginning best fit procedure\n");

    for ( row = rowmin; row <= rowmax; row++ ) {
	// fit  = fopen("fit.dat",  "w");
	// fit1 = fopen("fit1.dat", "w");

	start = colmin;

	// printf("    fitting row = %d\n", row);

	while ( start < colmax ) {
	    end = start + 1;
	    good_fit = 1;

	    x[(end - start) - 1] = 0.0 + ( start * col_step );
	    y[(end - start) - 1] = dem_data[start][row];

	    while ( (end <= colmax) && good_fit ) {
		n = (end - start) + 1;
		// printf("Least square of first %d points\n", n);
		x[end - start] = 0.0 + ( end * col_step );
		y[end - start] = dem_data[end][row];
		least_squares(x, y, n, &m, &b);
		ave_error = least_squares_error(x, y, n, m, b);
		max_error = least_squares_max_error(x, y, n, m, b);

		/*
		printf("%d - %d  ave error = %.2f  max error = %.2f  y = %.2f*x + %.2f\n", 
		start, end, ave_error, max_error, m, b);
		
		f = fopen("gnuplot.dat", "w");
		for ( j = 0; j <= end; j++) {
		    fprintf(f, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
			    dem_data[row][j]);
		}
		for ( j = start; j <= end; j++) {
		    fprintf(f, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
			    dem_data[row][j]);
		}
		fclose(f);

		printf("Please hit return: "); gets(junk);
		*/

		if ( max_error > error ) {
		    good_fit = 0;
		}
		
		end++;
	    }

	    if ( !good_fit ) {
		// error exceeded the threshold, back up
		end -= 2;  // back "end" up to the last good enough fit
		n--;       // back "n" up appropriately too
	    } else {
		// we popped out of the above loop while still within
		// the error threshold, so we must be at the end of
		// the data set
		end--;
	    }
	    
	    least_squares(x, y, n, &m, &b);
	    ave_error = least_squares_error(x, y, n, m, b);
	    max_error = least_squares_max_error(x, y, n, m, b);

	    /*
	    printf("\n");
	    printf("%d - %d  ave error = %.2f  max error = %.2f  y = %.2f*x + %.2f\n", 
		   start, end, ave_error, max_error, m, b);
	    printf("\n");

	    fprintf(fit1, "%.2f %.2f\n", x[0], m * x[0] + b);
	    fprintf(fit1, "%.2f %.2f\n", x[end-start], m * x[end-start] + b);
	    */

	    if ( start > colmin ) {
		// skip this for the first line segment
		cury = m * x[0] + b;
		outputmesh_set_pt(start, row, (lasty + cury) / 2);
		// fprintf(fit, "%.2f %.2f\n", x[0], (lasty + cury) / 2);
	    }

	    lasty = m * x[end-start] + b;
	    start = end;
	}

	/*
	fclose(fit);
	fclose(fit1);

	dem = fopen("gnuplot.dat", "w");
	for ( j = 0; j < DEM_SIZE_1; j++) {
	    fprintf(dem, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
		    dem_data[j][row]);
	} 
	fclose(dem);
	*/

	// NOTICE, this is for testing only.  This instance of
        // output_nodes should be removed.  It should be called only
        // once at the end once all the nodes have been generated.
	// newmesh_output_nodes(&nm, "mesh.node");
	// printf("Please hit return: "); gets(junk);
    }

    // outputmesh_output_nodes(fg_root, p);
}


// Initialize output mesh structure
void fgDEM::outputmesh_init( void ) {
    int i, j;
    
    for ( j = 0; j < DEM_SIZE_1; j++ ) {
	for ( i = 0; i < DEM_SIZE_1; i++ ) {
	    output_data[i][j] = -9999.0;
	}
    }
}


// Get the value of a mesh node
double fgDEM::outputmesh_get_pt( int i, int j ) {
    return ( output_data[i][j] );
}


// Set the value of a mesh node
void fgDEM::outputmesh_set_pt( int i, int j, double value ) {
    // printf("Setting data[%d][%d] = %.2f\n", i, j, value);
   output_data[i][j] = value;
}


// Write out a node file that can be used by the "triangle" program
void fgDEM::outputmesh_output_nodes( char *fg_root, fgBUCKET *p ) {
    struct stat stat_buf;
    char base_path[256], dir[256], file[256];
#ifdef WIN32
    char tmp_path[256];
#endif
    char command[256];
    FILE *fd;
    long int index;
    int colmin, colmax, rowmin, rowmax;
    int i, j, count, result;

    // determine dimensions
    colmin = p->x * ( (cols - 1) / 8);
    colmax = colmin + ( (cols - 1) / 8);
    rowmin = p->y * ( (rows - 1) / 8);
    rowmax = rowmin + ( (rows - 1) / 8);
    printf("  dumping region = %d,%d to %d,%d\n", 
	   colmin, rowmin, colmax, rowmax);

    // generate the base directory
    fgBucketGenBasePath(p, base_path);
    printf("fg_root = %s  Base Path = %s\n", fg_root, base_path);
    sprintf(dir, "%s/Scenery/%s", fg_root, base_path);
    printf("Dir = %s\n", dir);
    
    // stat() directory and create if needed
    result = stat(dir, &stat_buf);
    if ( result != 0 ) {
	printf("Stat error need to create directory\n");

#ifndef WIN32

	sprintf(command, "mkdir -p %s\n", dir);
	system(command);

#else // WIN32

	// Cygwin crashes when trying to output to node file
	// explicitly making directory structure seems OK on Win95

	extract_path (base_path, tmp_path);

	sprintf (dir, "%s/Scenery", fg_root);
	if (my_mkdir (dir)) { exit (-1); }

	sprintf (dir, "%s/Scenery/%s", fg_root, tmp_path);
	if (my_mkdir (dir)) { exit (-1); }

	sprintf (dir, "%s/Scenery/%s", fg_root, base_path);
	if (my_mkdir (dir)) { exit (-1); }

#endif // WIN32

    } else {
	// assume directory exists
    }

    // get index and generate output file name
    index = fgBucketGenIndex(p);
    sprintf(file, "%s/%ld.node", dir, index);

    printf("Creating node file:  %s\n", file);
    fd = fopen(file, "w");

    // first count nodes to generate header
    count = 0;
    for ( j = rowmin; j <= rowmax; j++ ) {
	for ( i = colmin; i <= colmax; i++ ) {
	    if ( output_data[i][j] > -9000.0 ) {
		count++;
	    }
	}
	// printf("    count = %d\n", count);
    }
    fprintf(fd, "%d 2 1 0\n", count);

    // now write out actual node data
    count = 1;
    for ( j = rowmin; j <= rowmax; j++ ) {
	for ( i = colmin; i <= colmax; i++ ) {
	    if ( output_data[i][j] > -9000.0 ) {
		fprintf(fd, "%d %.2f %.2f %.2f\n", 
			count++, 
			originx + (double)i * col_step, 
			originy + (double)j * row_step,
			output_data[i][j]);
	    }
	}
	// printf("    count = %d\n", count);
    }

    fclose(fd);
}


fgDEM::~fgDEM( void ) {
    // printf("class fgDEM DEstructor called.\n");
    free(dem_data);
    free(output_data);
}


// $Log$
// Revision 1.8  1998/07/04 00:47:18  curt
// typedef'd struct fgBUCKET.
//
// Revision 1.7  1998/06/05 18:14:39  curt
// Abort out early when reading the "A" record if it doesn't look like
// a proper DEM file.
//
// Revision 1.6  1998/05/02 01:49:21  curt
// Fixed a bug where the wrong variable was being initialized.
//
// Revision 1.5  1998/04/25 15:00:32  curt
// Changed "r" to "rb" in gzopen() options.  This fixes bad behavior in win32.
//
// Revision 1.4  1998/04/22 13:14:46  curt
// Fixed a bug in zlib usage.
//
// Revision 1.3  1998/04/18 03:53:05  curt
// Added zlib support.
//
// Revision 1.2  1998/04/14 02:43:27  curt
// Used "new" to auto-allocate large DEM parsing arrays in class constructor.
//
// Revision 1.1  1998/04/08 22:57:22  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.3  1998/04/06 21:09:41  curt
// Additional win32 support.
// Fixed a bad bug in dem file parsing that was causing the output to be
// flipped about x = y.
//
// Revision 1.2  1998/03/23 20:35:41  curt
// Updated to use FG_EPSILON
//
// Revision 1.1  1998/03/19 02:54:47  curt
// Reorganized into a class lib called fgDEM.
//
// Revision 1.1  1998/03/19 01:46:28  curt
// Initial revision.
//
