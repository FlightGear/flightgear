// dem.cxx -- DEM management class
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#include <ctype.h>    // isspace()
#include <stdlib.h>   // atoi()
#include <math.h>     // rint()
#include <stdio.h>
#include <string.h>

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h> // stat()
#endif

#ifdef FG_HAVE_STD_INCLUDES
#  include <cerrno>
#else
#  include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>   // stat()
#endif

#include <Misc/fgstream.hxx>
#include <Misc/strutils.hxx>
#include <Include/fg_constants.h>

#include "dem.hxx"


#define MAX_EX_NODES 10000

#if 0
#ifdef WIN32
# ifdef __BORLANDC__
#  include <dir.h>
#  define MKDIR(a) mkdir(a)
# else
#  define MKDIR(a) mkdir(a,S_IRWXU)  // I am just guessing at this flag (NHV)
# endif // __BORLANDC__
#endif // WIN32
#endif //0


FGDem::FGDem( void ) {
    // cout << "class FGDem CONstructor called." << endl;
    dem_data = new float[DEM_SIZE_1][DEM_SIZE_1];
    output_data = new float[DEM_SIZE_1][DEM_SIZE_1];
}


FGDem::FGDem( const string &file ) {
    // cout << "class FGDem CONstructor called." << endl;
    dem_data = new float[DEM_SIZE_1][DEM_SIZE_1];
    output_data = new float[DEM_SIZE_1][DEM_SIZE_1];

    FGDem::open(file);
}


// open a DEM file
int
FGDem::open ( const string& file ) {
    // open input file (or read from stdin)
    if ( file ==  "-" ) {
	printf("Loading DEM data file: stdin\n");
	// fd = stdin;
	// fd = gzdopen(STDIN_FILENO, "r");
	printf("Not yet ported ...\n");
	return 0;
    } else {
	in = new fg_gzifstream( file );
	if ( !(*in) ) {
	    cout << "Cannot open " << file << endl;
	    return 0;
	}
	cout << "Loading DEM data file: " << file << endl;
    }

    return 1;
}


// close a DEM file
int
FGDem::close () {
    // the fg_gzifstream doesn't seem to have a close()

    delete in;

    return 1;
}


// return next token from input stream
string
FGDem::next_token() {
    string token;

    *in >> token;

    // cout << "    returning " + token + "\n";

    return token;
}


// return next integer from input stream
int
FGDem::next_int() {
    int result;
    
    *in >> result;

    return result;
}


// return next double from input stream
double
FGDem::next_double() {
    double result;

    *in >> result;

    return result;
}


// return next exponential num from input stream
double
FGDem::next_exp() {
    string token;

    token = next_token();

    const char* p = token.c_str();
    char buf[64];
    char* bp = buf;
    
    for ( ; *p != 0; ++p )
    {
	if ( *p == 'D' )
	    *bp++ = 'E';
	else
	    *bp++ = *p;
    }
    *bp = 0;
    return ::atof( buf );
}


// read and parse DEM "A" record
int
FGDem::read_a_record() {
    int i, inum;
    double dnum;
    string name, token;
    char c;

    // get the name field (144 characters)
    for ( i = 0; i < 144; i++ ) {
	in->get(c);
	name += c;
    }
  
    // clean off the trailing whitespace
    name = trim(name);
    cout << "    Quad name field: " << name << endl;

    // DEM level code, 3 reflects processing by DMA
    inum = next_int();
    cout << "    DEM level code = " << inum << "\n";

    if ( inum > 3 ) {
	return 0;
    }

    // Pattern code, 1 indicates a regular elevation pattern
    inum = next_int();
    cout << "    Pattern code = " << inum << "\n";

    // Planimetric reference system code, 0 indicates geographic
    // coordinate system.
    inum = next_int();
    cout << "    Planimetric reference code = " << inum << "\n";

    // Zone code
    inum = next_int();
    cout << "    Zone code = " << inum << "\n";

    // Map projection parameters (ignored)
    for ( i = 0; i < 15; i++ ) {
	dnum = next_exp();
	// printf("%d: %f\n",i,dnum);
    }

    // Units code, 3 represents arc-seconds as the unit of measure for
    // ground planimetric coordinates throughout the file.
    inum = next_int();
    if ( inum != 3 ) {
	cout << "    Unknown (X,Y) units code = " << inum << "!\n";
	exit(-1);
    }

    // Units code; 2 represents meters as the unit of measure for
    // elevation coordinates throughout the file.
    inum = next_int();
    if ( inum != 2 ) {
	cout << "    Unknown (Z) units code = " << inum << "!\n";
	exit(-1);
    }

    // Number (n) of sides in the polygon which defines the coverage of
    // the DEM file (usually equal to 4).
    inum = next_int();
    if ( inum != 4 ) {
	cout << "    Unknown polygon dimension = " << inum << "!\n";
	exit(-1);
    }

    // Ground coordinates of bounding box in arc-seconds
    dem_x1 = originx = next_exp();
    dem_y1 = originy = next_exp();
    cout << "    Origin = (" << originx << "," << originy << ")\n";

    dem_x2 = next_exp();
    dem_y2 = next_exp();

    dem_x3 = next_exp();
    dem_y3 = next_exp();

    dem_x4 = next_exp();
    dem_y4 = next_exp();

    // Minimum/maximum elevations in meters
    dem_z1 = next_exp();
    dem_z2 = next_exp();
    cout << "    Elevation range " << dem_z1 << " to " << dem_z2 << "\n";

    // Counterclockwise angle from the primary axis of ground
    // planimetric referenced to the primary axis of the DEM local
    // reference system.
    token = next_token();

    // Accuracy code; 0 indicates that a record of accuracy does not
    // exist and that no record type C will follow.

    // DEM spacial resolution.  Usually (3,3,1) (3,6,1) or (3,9,1)
    // depending on latitude

    // I will eventually have to do something with this for data at
    // higher latitudes */
    token = next_token();
    cout << "    accuracy & spacial resolution string = " << token << endl;
    i = token.length();
    cout << "    length = " << i << "\n";

    inum = atoi( token.substr( 0, i - 36 ) );
    row_step = atof( token.substr( i - 24, 12 ) );
    col_step = atof( token.substr( i - 36, 12 ) );
    cout << "    Accuracy code = " << inum << "\n";
    cout << "    column step = " << col_step << 
	"  row step = " << row_step << "\n";

    // dimension of arrays to follow (1)
    token = next_token();

    // number of profiles
    dem_num_profiles = cols = next_int();
    cout << "    Expecting " << dem_num_profiles << " profiles\n";

    return 1;
}


// read and parse DEM "B" record
void
FGDem::read_b_record( ) {
    string token;
    int i;

    // row / column id of this profile
    prof_row = next_int();
    prof_col = next_int();
    // printf("col id = %d  row id = %d\n", prof_col, prof_row);

    // Number of columns and rows (elevations) in this profile
    prof_num_rows = rows = next_int();
    prof_num_cols = next_int();
    // printf("    profile num rows = %d\n", prof_num_rows);

    // Ground planimetric coordinates (arc-seconds) of the first
    // elevation in the profile
    prof_x1 = next_exp();
    prof_y1 = next_exp();
    // printf("    Starting at %.2f %.2f\n", prof_x1, prof_y1);

    // Elevation of local datum for the profile.  Always zero for
    // 1-degree DEM, the reference is mean sea level.
    token = next_token();

    // Minimum and maximum elevations for the profile.
    token = next_token();
    token = next_token();

    // One (usually) dimensional array (prof_num_cols,1) of elevations
    for ( i = 0; i < prof_num_rows; i++ ) {
	prof_data = next_int();
	dem_data[cur_col][i] = (float)prof_data;
    }
}


// parse dem file
int
FGDem::parse( ) {
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
	    cout << "    loaded " << cur_col << " profiles of data\n";
	}
    }

    cout << "    Done parsing\n";

    return 1;
}


// write out the area of data covered by the specified bucket.  Data
// is written out column by column starting at the lower left hand
// corner.
int
FGDem::write_area( const string& root, FGBucket& b, bool compress ) {
    // calculate some boundaries
    double min_x = ( b.get_center_lon() - 0.5 * b.get_width() ) * 3600.0;
    double max_x = ( b.get_center_lon() + 0.5 * b.get_width() ) * 3600.0;

    double min_y = ( b.get_center_lat() - 0.5 * b.get_height() ) * 3600.0;
    double max_y = ( b.get_center_lat() + 0.5 * b.get_height() ) * 3600.0;

    cout << b << endl;
    cout << "width = " << b.get_width() << " height = " << b.get_height() 
	 << endl;

    int start_x = (int)((min_x - originx) / col_step);
    int span_x = (int)(b.get_width() * 3600.0 / col_step);

    int start_y = (int)((min_y - originy) / row_step);
    int span_y = (int)(b.get_height() * 3600.0 / row_step);

    cout << "start_x = " << start_x << "  span_x = " << span_x << endl;
    cout << "start_y = " << start_y << "  span_y = " << span_y << endl;

    // Do a simple sanity checking.  But, please, please be nice to
    // this write_area() routine and feed it buckets that coincide
    // well with the underlying grid structure and spacing.

    if ( ( min_x < originx )
	 || ( max_x > originx + cols * col_step )
	 || ( min_y < originy )
	 || ( max_y > originy + rows * row_step ) ) {
	cout << "  ERROR: bucket at least partially outside DEM data range!" <<
	    endl;
	return 0;
    }

    // generate output file name
    string base = b.gen_base_path();
    string path = root + "/Scenery/" + base;
    string command = "mkdir -p " + path;
    system( command.c_str() );

    string demfile = path + "/" + b.gen_index_str() + ".dem";
    cout << "demfile = " << demfile << endl;

    // write the file
    FILE *fp;
    if ( (fp = fopen(demfile.c_str(), "w")) == NULL ) {
	cout << "cannot open " << demfile << " for writing!" << endl;
	exit(-1);
    }

    fprintf( fp, "%d %d\n", (int)min_x, (int)min_y );
    fprintf( fp, "%d %d %d %d\n", span_x + 1, (int)col_step, 
	     span_y + 1, (int)row_step );
    for ( int i = start_x; i <= start_x + span_x; ++i ) {
	for ( int j = start_y; j <= start_y + span_y; ++j ) {
	    fprintf( fp, "%d ", (int)dem_data[i][j] );
	}
	fprintf( fp, "\n" );
    }
    fclose(fp);

    if ( compress ) {
	string command = "gzip --best -f " + demfile;
	system( command.c_str() );
    }

    return 1;
}


#if 0

// return the current altitude based on grid data.  We should rewrite
// this to interpolate exact values, but for now this is good enough
double FGDem::interpolate_altitude( double lon, double lat ) {
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
void FGDem::fit( double error, FGBucket& p ) {
    double x[DEM_SIZE_1], y[DEM_SIZE_1];
    double m, b, ave_error, max_error;
    double cury, lasty;
    int n, row, start, end;
    int colmin, colmax, rowmin, rowmax;
    bool good_fit;
    // FILE *dem, *fit, *fit1;

    printf("Initializing output mesh structure\n");
    outputmesh_init();

    // determine dimensions
    colmin = p.get_x() * ( (cols - 1) / 8);
    colmax = colmin + ( (cols - 1) / 8);
    rowmin = p.get_y() * ( (rows - 1) / 8);
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
	    good_fit = true;

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
void FGDem::outputmesh_init( void ) {
    int i, j;
    
    for ( j = 0; j < DEM_SIZE_1; j++ ) {
	for ( i = 0; i < DEM_SIZE_1; i++ ) {
	    output_data[i][j] = -9999.0;
	}
    }
}


// Get the value of a mesh node
double FGDem::outputmesh_get_pt( int i, int j ) {
    return ( output_data[i][j] );
}


// Set the value of a mesh node
void FGDem::outputmesh_set_pt( int i, int j, double value ) {
    // printf("Setting data[%d][%d] = %.2f\n", i, j, value);
   output_data[i][j] = value;
}


// Write out a node file that can be used by the "triangle" program.
// Check for an optional "index.node.ex" file in case there is a .poly
// file to go along with this node file.  Include these nodes first
// since they are referenced by position from the .poly file.
void FGDem::outputmesh_output_nodes( const string& fg_root, FGBucket& p )
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
    cout << "fg_root = " << fg_root << "  Base Path = " << base_path << endl;
    dir = fg_root + "/Scenery/" + base_path;
    cout << "Dir = " << dir << endl;
    
    // stat() directory and create if needed
    errno = 0;
    result = stat(dir.c_str(), &stat_buf);
    if ( result != 0 && errno == ENOENT ) {
	cout << "Creating directory\n";

// #ifndef WIN32

	command = "mkdir -p " + dir + "\n";
	system( command.c_str() );

#if 0
// #else // WIN32

	// Cygwin crashes when trying to output to node file
	// explicitly making directory structure seems OK on Win95

	extract_path (base_path, tmp_path);

	dir = fg_root + "/Scenery";
	if (my_mkdir ( dir.c_str() )) { exit (-1); }

	dir = fg_root + "/Scenery/" + tmp_path;
	if (my_mkdir ( dir.c_str() )) { exit (-1); }

	dir = fg_root + "/Scenery/" + base_path;
	if (my_mkdir ( dir.c_str() )) { exit (-1); }

// #endif // WIN32
#endif //0

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
	    if ( output_data[i][j] > -9000.0 ) {
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
#endif


FGDem::~FGDem( void ) {
    // printf("class FGDem DEstructor called.\n");
    delete [] dem_data;
    delete [] output_data;
}


