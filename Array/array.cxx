// array.cxx -- Array management class
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998 - 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

// #include <ctype.h>    // isspace()
// #include <stdlib.h>   // atoi()
// #include <math.h>     // rint()
// #include <stdio.h>
// #include <string.h>
// #ifdef HAVE_SYS_STAT_H
// #  include <sys/stat.h> // stat()
// #endif
// #ifdef FG_HAVE_STD_INCLUDES
// #  include <cerrno>
// #else
// #  include <errno.h>
// #endif
// #ifdef HAVE_UNISTD_H
// # include <unistd.h>   // stat()
// #endif

#include STL_STRING

#include <Include/fg_constants.h>
#include <Misc/fgstream.hxx>
#include <Misc/strutils.hxx>
#include <Math/leastsqs.hxx>

#include "array.hxx"

FG_USING_STD(string);


FGArray::FGArray( void ) {
    // cout << "class FGArray CONstructor called." << endl;
    in_data = new float[ARRAY_SIZE_1][ARRAY_SIZE_1];
    out_data = new float[ARRAY_SIZE_1][ARRAY_SIZE_1];
}


FGArray::FGArray( const string &file ) {
    // cout << "class FGArray CONstructor called." << endl;
    in_data = new float[ARRAY_SIZE_1][ARRAY_SIZE_1];
    out_data = new float[ARRAY_SIZE_1][ARRAY_SIZE_1];

    FGArray::open(file);
}


// open an Array file
int
FGArray::open( const string& file ) {
    // open input file (or read from stdin)
    if ( file ==  "-" ) {
	cout << "  Opening array data pipe from stdin" << endl;
	// fd = stdin;
	// fd = gzdopen(STDIN_FILENO, "r");
	cout << "  Not yet ported ..." << endl;
	return 0;
    } else {
	in = new fg_gzifstream( file );
	if ( !(*in) ) {
	    cout << "  Cannot open " << file << endl;
	    return 0;
	}
	cout << "  Opening array data file: " << file << endl;
    }

    return 1;
}


// close an Array file
int
FGArray::close() {
    // the fg_gzifstream doesn't seem to have a close()

    delete in;

    return 1;
}


// parse Array file
int
FGArray::parse() {
    int i;

    // cur_col = 0;

    *in >> originx >> originy;
    *in >> cols >> col_step;
    *in >> rows >> row_step;

    cout << "    origin  = " << originx << "  " << originy << endl;
    cout << "    cols = " << cols << "  rows = " << rows << endl;
    cout << "    col_step = " << col_step << "  row_step = " << row_step <<endl;

    for ( int i = 0; i < cols; i++ ) {
	for ( int j = 0; j < rows; j++ ) {
	    *in >> in_data[i][j];
	}
    }

    cout << "    Done parsing\n";

    return 1;
}


// Initialize output mesh structure
void FGArray::outputmesh_init( void ) {
    int i, j;
    
    for ( j = 0; j < ARRAY_SIZE_1; j++ ) {
	for ( i = 0; i < ARRAY_SIZE_1; i++ ) {
	    out_data[i][j] = -9999.0;
	}
    }
}


// Get the value of a mesh node
double FGArray::outputmesh_get_pt( int i, int j ) {
    return ( out_data[i][j] );
}


// Set the value of a mesh node
void FGArray::outputmesh_set_pt( int i, int j, double value ) {
    // cout << "Setting data[" << i << "][" << j << "] = " << value << endl;
   out_data[i][j] = value;
}


// Use least squares to fit a simpler data set to dem data
void FGArray::fit( double error ) {
    double x[ARRAY_SIZE_1], y[ARRAY_SIZE_1];
    double m, b, max_error, error_sq;
    double x1, y1;
    // double ave_error;
    double cury, lasty;
    int n, row, start, end;
    int colmin, colmax, rowmin, rowmax;
    bool good_fit;
    // FILE *dem, *fit, *fit1;

    error_sq = error * error;

    cout << "  Initializing output mesh structure" << endl;
    outputmesh_init();

    // determine dimensions
    colmin = 0;
    colmax = cols;
    rowmin = 0;
    rowmax = rows;
    cout << "  Fitting region = " << colmin << "," << rowmin << " to " 
	 << colmax << "," << rowmax << endl;;
    
    // include the corners explicitly
    outputmesh_set_pt(colmin, rowmin, in_data[colmin][rowmin]);
    outputmesh_set_pt(colmin, rowmax, in_data[colmin][rowmax]);
    outputmesh_set_pt(colmax, rowmax, in_data[colmax][rowmax]);
    outputmesh_set_pt(colmax, rowmin, in_data[colmax][rowmin]);

    cout << "  Beginning best fit procedure" << endl;

    for ( row = rowmin; row < rowmax; row++ ) {
	// fit  = fopen("fit.dat",  "w");
	// fit1 = fopen("fit1.dat", "w");

	start = colmin;

	// cout << "    fitting row = " << row << endl;

	while ( start < colmax - 1 ) {
	    end = start + 1;
	    good_fit = true;

	    x[0] = start * col_step;
	    y[0] = in_data[start][row];

	    x[1] = end * col_step;
	    y[1] = in_data[end][row];

	    n = 2;

	    // cout << "Least square of first 2 points" << endl;
	    least_squares(x, y, n, &m, &b);

	    end++;

	    while ( (end < colmax) && good_fit ) {
		++n;
		// cout << "Least square of first " << n << " points" << endl;
		x[n-1] = x1 = end * col_step;
		y[n-1] = y1 = in_data[end][row];
		least_squares_update(x1, y1, &m, &b);
		// ave_error = least_squares_error(x, y, n, m, b);
		max_error = least_squares_max_error(x, y, n, m, b);

		/*
		printf("%d - %d  ave error = %.2f  max error = %.2f  y = %.2f*x + %.2f\n", 
		start, end, ave_error, max_error, m, b);
		
		f = fopen("gnuplot.dat", "w");
		for ( j = 0; j <= end; j++) {
		    fprintf(f, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
			    in_data[row][j]);
		}
		for ( j = start; j <= end; j++) {
		    fprintf(f, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
			    in_data[row][j]);
		}
		fclose(f);

		printf("Please hit return: "); gets(junk);
		*/

		if ( max_error > error_sq ) {
		    good_fit = false;
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
	    // ave_error = least_squares_error(x, y, n, m, b);
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
	for ( j = 0; j < ARRAY_SIZE_1; j++) {
	    fprintf(dem, "%.2f %.2f\n", 0.0 + ( j * col_step ), 
		    in_data[j][row]);
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


// return the current altitude based on grid data.  We should rewrite
// this to interpolate exact values, but for now this is good enough
double FGArray::interpolate_altitude( double lon, double lat ) {
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
	cout << "WARNING: Attempt to interpolate value outside of array!!!" 
	     << endl;
	return(-9999);
    }

    dx = xlocal - xindex;
    dy = ylocal - yindex;

    if ( dx > dy ) {
	// lower triangle
	// printf("  Lower triangle\n");

	x1 = xindex; 
	y1 = yindex; 
	z1 = in_data[x1][y1];

	x2 = xindex + 1; 
	y2 = yindex; 
	z2 = in_data[x2][y2];
				  
	x3 = xindex + 1; 
	y3 = yindex + 1; 
	z3 = in_data[x3][y3];

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
	z1 = in_data[x1][y1];

	x2 = xindex; 
	y2 = yindex + 1; 
	z2 = in_data[x2][y2];
				  
	x3 = xindex + 1; 
	y3 = yindex + 1; 
	z3 = in_data[x3][y3];

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


#if 0
// Write out a node file that can be used by the "triangle" program.
// Check for an optional "index.node.ex" file in case there is a .poly
// file to go along with this node file.  Include these nodes first
// since they are referenced by position from the .poly file.
void FGArray::outputmesh_output_nodes( const string& fg_root, FGBucket& p )
{
    double exnodes[MAX_EX_NODES][3];
    struct stat stat_buf;
    string dir;
    char file[256], exfile[256];
#ifdef WIN32
    char tmp_path[256];
#endif
    string command;
    FILE *fd;
    long int index;
    int colmin, colmax, rowmin, rowmax;
    int i, j, count, excount, result;

    // determine dimensions
    colmin = p.get_x() * ( (cols - 1) / 8);
    colmax = colmin + ( (cols - 1) / 8);
    rowmin = p.get_y() * ( (rows - 1) / 8);
    rowmax = rowmin + ( (rows - 1) / 8);
    cout << "  dumping region = " << colmin << "," << rowmin << " to " <<
	colmax << "," << rowmax << "\n";

    // generate the base directory
    string base_path = p.gen_base_path();
    cout << "  fg_root = " << fg_root << "  Base Path = " << base_path << endl;
    dir = fg_root + "/Scenery/" + base_path;
    cout << "  Dir = " << dir << endl;
    
    // stat() directory and create if needed
    errno = 0;
    result = stat(dir.c_str(), &stat_buf);
    if ( result != 0 && errno == ENOENT ) {
	cout << "  Creating directory\n";

	command = "mkdir -p " + dir + "\n";
	system( command.c_str() );
    } else {
	// assume directory exists
    }

    // get index and generate output file name
    index = p.gen_index();
    sprintf(file, "%s/%ld.node", dir.c_str(), index);

    // get (optional) extra node file name (in case there is matching
    // .poly file.
    strcpy(exfile, file);
    strcat(exfile, ".ex");

    // load extra nodes if they exist
    excount = 0;
    if ( (fd = fopen(exfile, "r")) != NULL ) {
	int junki;
	fscanf(fd, "%d %d %d %d", &excount, &junki, &junki, &junki);

	if ( excount > MAX_EX_NODES - 1 ) {
	    printf("Error, too many 'extra' nodes, increase array size\n");
	    exit(-1);
	} else {
	    printf("    Expecting %d 'extra' nodes\n", excount);
	}

	for ( i = 1; i <= excount; i++ ) {
	    fscanf(fd, "%d %lf %lf %lf\n", &junki, 
		   &exnodes[i][0], &exnodes[i][1], &exnodes[i][2]);
	    printf("(extra) %d %.2f %.2f %.2f\n", 
		    i, exnodes[i][0], exnodes[i][1], exnodes[i][2]);
	}
	fclose(fd);
    }

    printf("Creating node file:  %s\n", file);
    fd = fopen(file, "w");

    // first count regular nodes to generate header
    count = 0;
    for ( j = rowmin; j <= rowmax; j++ ) {
	for ( i = colmin; i <= colmax; i++ ) {
	    if ( out_data[i][j] > -9000.0 ) {
		count++;
	    }
	}
	// printf("    count = %d\n", count);
    }
    fprintf(fd, "%d 2 1 0\n", count + excount);

    // now write out extra node data
    for ( i = 1; i <= excount; i++ ) {
	fprintf(fd, "%d %.2f %.2f %.2f\n", 
		i, exnodes[i][0], exnodes[i][1], exnodes[i][2]);
    }

    // write out actual node data
    count = excount + 1;
    for ( j = rowmin; j <= rowmax; j++ ) {
	for ( i = colmin; i <= colmax; i++ ) {
	    if ( out_data[i][j] > -9000.0 ) {
		fprintf(fd, "%d %.2f %.2f %.2f\n", 
			count++, 
			originx + (double)i * col_step, 
			originy + (double)j * row_step,
			out_data[i][j]);
	    }
	}
	// printf("    count = %d\n", count);
    }

    fclose(fd);
}
#endif


FGArray::~FGArray( void ) {
    // printf("class FGArray DEstructor called.\n");
    delete [] in_data;
    delete [] out_data;
}


// $Log$
// Revision 1.2  1999/03/13 23:50:26  curt
// Tweaked output formatting a bit.
//
// Revision 1.1  1999/03/13 18:45:02  curt
// Initial revision. (derived from libDEM.a code.)
//
