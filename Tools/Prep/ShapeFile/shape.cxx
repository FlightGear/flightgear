// shape.cxx -- shape/gpc utils
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <Include/compiler.h>

#include STL_STRING

#include <Bucket/newbucket.hxx>
#include <Debug/logstream.hxx>

#include <Polygon/index.hxx>
#include <Polygon/names.hxx>
#include "shape.hxx"


#define FG_MAX_VERTICES 100000
static gpc_vertex_list v_list;


class point2d {
public:
    double x, y;
};


static void clip_and_write_poly( string root, long int p_index, AreaType area, 
				 FGBucket b, gpc_polygon *shape ) {
    point2d c, min, max;
    c.x = b.get_center_lon();
    c.y = b.get_center_lat();
    double span = bucket_span(c.y);
    gpc_polygon base, result;
    char tile_name[256], poly_index[256];

    // calculate bucket dimensions
    if ( (c.y >= -89.0) && (c.y < 89.0) ) {
	min.x = c.x - span / 2.0;
	max.x = c.x + span / 2.0;
	min.y = c.y - FG_HALF_BUCKET_SPAN;
	max.y = c.y + FG_HALF_BUCKET_SPAN;
    } else if ( c.y < -89.0) {
	min.x = -90.0;
	max.x = -89.0;
	min.y = -180.0;
	max.y = 180.0;
    } else if ( c.y >= 89.0) {
	min.x = 89.0;
	max.x = 90.0;
	min.y = -180.0;
	max.y = 180.0;
    } else {
	FG_LOG ( FG_GENERAL, FG_ALERT, 
		 "Out of range latitude in clip_and_write_poly() = " << c.y );
    }

    FG_LOG( FG_GENERAL, FG_INFO, "  (" << min.x << "," << min.y << ") ("
	    << max.x << "," << max.y << ")" );

    // set up clipping tile
    v_list.vertex[0].x = min.x;
    v_list.vertex[0].y = min.y;

    v_list.vertex[1].x = max.x;
    v_list.vertex[1].y = min.y;

    v_list.vertex[2].x = max.x;
    v_list.vertex[2].y = max.y;

    v_list.vertex[3].x = min.x;
    v_list.vertex[3].y = max.y;

    v_list.num_vertices = 4;

    base.num_contours = 0;
    base.contour = NULL;
    gpc_add_contour( &base, &v_list, 0 );

    // FG_LOG( FG_GENERAL, FG_DEBUG, "base = 4 vertices" );

    /*
    FILE *bfp= fopen("base", "w");
    gpc_write_polygon(bfp, &base);
    fclose(bfp);
    */

    gpc_polygon_clip(GPC_INT, &base, shape, &result);

    if ( result.num_contours > 0 ) {
	long int t_index = b.gen_index();
	string path = root + "/Scenery/" + b.gen_base_path();
	string command = "mkdir -p " + path;
	system( command.c_str() );

	sprintf( tile_name, "%ld", t_index );
	string polyfile = path + "/" + tile_name;

	sprintf( poly_index, "%ld", p_index );
	polyfile += ".";
	polyfile += poly_index;

	string poly_type = get_area_name( area );
	if ( poly_type == "Unknown" ) {
	    cout << "unknown area type in clip_and_write_poly()!" << endl;
	    exit(-1);
	}
	
	FILE *rfp= fopen( polyfile.c_str(), "w" );
	fprintf( rfp, "%s\n", poly_type.c_str() );
	gpc_write_polygon( rfp, 1, &result );
	fclose( rfp );

	// only free result if it is not empty
	gpc_free_polygon(&result);
    }

    gpc_free_polygon(&base);
}


// Initialize structure we use to create polygons for the gpc library
bool shape_utils_init() {
    v_list.num_vertices = 0;
    v_list.vertex = new gpc_vertex[FG_MAX_VERTICES];;

    return true;
}


// initialize a gpc_polygon
void init_shape(gpc_polygon *shape) {
    shape->num_contours = 0;
    shape->contour = NULL;
}


// make a gpc_polygon, first contour is outline, remaining contours
// are holes
void add_to_shape(int count, double *coords, gpc_polygon *shape) {
    
    for ( int i = 0; i < count; i++ ) {
	v_list.vertex[i].x = coords[i*2+0];
	v_list.vertex[i].y = coords[i*2+1];
    }

    v_list.num_vertices = count;

    if ( shape->num_contours == 0 ) {
	// outline
	gpc_add_contour( shape, &v_list, 0 );
    } else {
	// hole
	gpc_add_contour( shape, &v_list, 1 );
    }
}


// process shape (write polygon to all intersecting tiles)
void process_shape(string path, AreaType area, gpc_polygon *master_gpc_shape) {
    point2d min, max;
    long int index;
    int i, j;

    min.x = min.y = 200.0;
    max.x = max.y = -200.0;

    // find min/max of polygon
    for ( i = 0; i < master_gpc_shape->num_contours; i++ ) {
	for ( j = 0; j < master_gpc_shape->contour[i].num_vertices; j++ ) {
	    double x = master_gpc_shape->contour[i].vertex[j].x;
	    double y = master_gpc_shape->contour[i].vertex[j].y;

	    if ( x < min.x ) { min.x = x; }
	    if ( y < min.y ) { min.y = y; }
	    if ( x > max.x ) { max.x = x; }
	    if ( y > max.y ) { max.y = y; }
	}
    }

    /*
    FILE *sfp= fopen("shape", "w");
    gpc_write_polygon(sfp, master_gpc_shape);
    fclose(sfp);
    exit(-1);
    */
	
    // get next polygon index
    index = poly_index_next();

    FG_LOG( FG_GENERAL, FG_INFO, "  min = " << min.x << "," << min.y
	    << " max = " << max.x << "," << max.y );

    // find buckets for min, and max points of convex hull.
    // note to self: self, you should think about checking for
    // polygons that span the date line
    FGBucket b_min(min.x, min.y);
    FGBucket b_max(max.x, max.y);
    FG_LOG( FG_GENERAL, FG_INFO, "  Bucket min = " << b_min );
    FG_LOG( FG_GENERAL, FG_INFO, "  Bucket max = " << b_max );
	    
    if ( b_min == b_max ) {
	clip_and_write_poly( path, index, area, b_min, master_gpc_shape );
    } else {
	FGBucket b_cur;
	int dx, dy, i, j;
	    
	fgBucketDiff(b_min, b_max, &dx, &dy);
	FG_LOG( FG_GENERAL, FG_INFO, 
		"  polygon spans tile boundaries" );
	FG_LOG( FG_GENERAL, FG_INFO, "  dx = " << dx 
		<< "  dy = " << dy );

	if ( (dx > 200) || (dy > 200) ) {
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "somethings really wrong!!!!" );
	    exit(-1);
	}

	for ( j = 0; j <= dy; j++ ) {
	    // for performance reasons, we'll clip out just this
	    // horizontal row, and clip all the tiles in this row
	    // against the smaller shape

	    FG_LOG ( FG_GENERAL, FG_INFO, 
		     "Generating clip row " << j << " of " << dy );
		
	    FGBucket b_clip = fgBucketOffset(min.x, min.y, 0, j);
	    gpc_polygon row, clip_row;
	    point2d c, clip_max, clip_min;
	    c.x = b_clip.get_center_lon();
	    c.y = b_clip.get_center_lat();

	    // calculate bucket clip_min.y and clip_max.y
	    if ( (c.y >= -89.0) && (c.y < 89.0) ) {
		clip_min.y = c.y - FG_HALF_BUCKET_SPAN;
		clip_max.y = c.y + FG_HALF_BUCKET_SPAN;
	    } else if ( c.y < -89.0) {
		clip_min.y = -90.0;
		clip_max.y = -89.0;
	    } else if ( c.y >= 89.0) {
		clip_min.y = 89.0;
		clip_max.y = 90.0;
	    } else {
		FG_LOG ( FG_GENERAL, FG_ALERT, 
			 "Out of range latitude in clip_and_write_poly() = " 
			 << c.y );
	    }
	    clip_min.x = -180.0;
	    clip_max.x = 180.0;
    
	    // set up clipping tile
	    v_list.vertex[0].x = clip_min.x;
	    v_list.vertex[0].y = clip_min.y;

	    v_list.vertex[1].x = clip_max.x;
	    v_list.vertex[1].y = clip_min.y;
	    
	    v_list.vertex[2].x = clip_max.x;
	    v_list.vertex[2].y = clip_max.y;

	    v_list.vertex[3].x = clip_min.x;
	    v_list.vertex[3].y = clip_max.y;

	    v_list.num_vertices = 4;

	    row.num_contours = 0;
	    row.contour = NULL;
	    gpc_add_contour( &row, &v_list, 0 );

	    clip_row.num_contours = 0;
	    clip_row.contour = NULL;
	    
	    gpc_polygon_clip(GPC_INT, &row, master_gpc_shape, &clip_row);

	    /* FILE *sfp = fopen("master_shape", "w");
	    gpc_write_polygon(sfp, 0, master_gpc_shape);
	    fclose(sfp);
	    sfp = fopen("clip_row", "w");
	    gpc_write_polygon(sfp, 0, &clip_row);
	    fclose(sfp); */

	    for ( i = 0; i <= dx; i++ ) {
		b_cur = fgBucketOffset(min.x, min.y, i, j);
		clip_and_write_poly( path, index, area, b_cur, &clip_row );
	    }
	    gpc_free_polygon(&row);
	    gpc_free_polygon(&clip_row);
	}
	// string answer; cin >> answer;
    }
}


// free a gpc_polygon
void free_shape(gpc_polygon *shape) {
    gpc_free_polygon(shape);
}


