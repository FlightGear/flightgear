// tilemgr.cxx -- routines to handle dynamic management of scenery tiles
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.h>

#include <Bucket/bucketutils.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/fg_types.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_geodesy.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Math/vector.hxx>
#include <Objects/material.hxx>
#include <Objects/obj.hxx>
#include <Weather/weather.h>

#include "scenery.hxx"
#include "tilecache.hxx"


#define FG_LOCAL_X_Y         81  // max(o->tile_diameter) ** 2

#define FG_SQUARE( X ) ( (X) * (X) )


// closest (potentially viewable) tiles, centered on current tile.
// This is an array of pointers to cache indexes.
int tiles[FG_LOCAL_X_Y];


// Initialize the Tile Manager subsystem
int fgTileMgrInit( void ) {
    fgPrintf( FG_TERRAIN, FG_INFO, "Initializing Tile Manager subsystem.\n");

    // load default material library
    material_mgr.load_lib();

    return 1;
}


// load a tile
void fgTileMgrLoadTile( fgBUCKET *p, int *index) {
    fgTILECACHE *c;

    c = &global_tile_cache;

    fgPrintf( FG_TERRAIN, FG_DEBUG, "Updating for bucket %d %d %d %d\n", 
	   p->lon, p->lat, p->x, p->y);
    
    // if not in cache, load tile into the next available slot
    if ( (*index = c->Exists(p)) < 0 ) {
	*index = c->NextAvail();
	c->EntryFillIn(*index, p);
    }

    fgPrintf( FG_TERRAIN, FG_DEBUG, "Selected cache index of %d\n", *index);
}


// given the current lon/lat, fill in the array of local chunks.  If
// the chunk isn't already in the cache, then read it from disk.
int fgTileMgrUpdate( void ) {
    fgTILECACHE *c;
    fgFLIGHT *f;
    fgBUCKET p1, p2;
    static fgBUCKET p_last = {-1000, 0, 0, 0};
    int tile_diameter;
    int i, j, dw, dh;

    c = &global_tile_cache;
    f = current_aircraft.flight;

    tile_diameter = current_options.get_tile_diameter();

    fgBucketFind(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p1);
    dw = tile_diameter / 2;
    dh = tile_diameter / 2;

    if ( (p1.lon == p_last.lon) && (p1.lat == p_last.lat) &&
	 (p1.x == p_last.x) && (p1.y == p_last.y) ) {
	// same bucket as last time
	fgPrintf( FG_TERRAIN, FG_DEBUG, "Same bucket as last time\n");
    } else if ( p_last.lon == -1000 ) {
	// First time through, initialize the system and load all
	// relavant tiles

	fgPrintf( FG_TERRAIN, FG_INFO, "  First time through ... ");
	fgPrintf( FG_TERRAIN, FG_INFO, "  Updating Tile list for %d,%d %d,%d\n",
		  p1.lon, p1.lat, p1.x, p1.y);
	fgPrintf( FG_TERRAIN, FG_INFO, "  Loading %d tiles\n", 
		  tile_diameter * tile_diameter);

	// wipe/initialize tile cache
	c->Init();

	// build the local area list and update cache
	for ( j = 0; j < tile_diameter; j++ ) {
	    for ( i = 0; i < tile_diameter; i++ ) {
		fgBucketOffset(&p1, &p2, i - dw, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*tile_diameter) + i]);
	    }
	}
    } else {
	// We've moved to a new bucket, we need to scroll our
        // structures, and load in the new tiles

	// CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
	// AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
	// THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION.

	fgPrintf( FG_TERRAIN, FG_INFO, "Updating Tile list for %d,%d %d,%d\n",
		  p1.lon, p1.lat, p1.x, p1.y);

	if ( (p1.lon > p_last.lon) ||
	     ( (p1.lon == p_last.lon) && (p1.x > p_last.x) ) ) {
	    fgPrintf( FG_TERRAIN, FG_INFO, "  Loading %d tiles\n", 
		      tile_diameter);
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling East
		for ( i = 0; i < tile_diameter - 1; i++ ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i + 1];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, dw + 1, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*tile_diameter) + 
					     tile_diameter - 1]);
	    }
	} else if ( (p1.lon < p_last.lon) ||
		    ( (p1.lon == p_last.lon) && (p1.x < p_last.x) ) ) {
	    fgPrintf( FG_TERRAIN, FG_INFO, "  Loading %d tiles\n", 
		      tile_diameter);
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling West
		for ( i = tile_diameter - 1; i > 0; i-- ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i - 1];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, -dw - 1, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*tile_diameter) + 0]);
	    }
	}

	if ( (p1.lat > p_last.lat) ||
	     ( (p1.lat == p_last.lat) && (p1.y > p_last.y) ) ) {
	    fgPrintf( FG_TERRAIN, FG_INFO, "  Loading %d tiles\n", 
		      tile_diameter);
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling North
		for ( j = 0; j < tile_diameter - 1; j++ ) {
		    tiles[(j * tile_diameter) + i] =
			tiles[((j+1) * tile_diameter) + i];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, i - dw, dh + 1);
		fgTileMgrLoadTile(&p2, &tiles[((tile_diameter-1) * 
					       tile_diameter) + i]);
	    }
	} else if ( (p1.lat < p_last.lat) ||
		    ( (p1.lat == p_last.lat) && (p1.y < p_last.y) ) ) {
	    fgPrintf( FG_TERRAIN, FG_INFO, "  Loading %d tiles\n", 
		      tile_diameter);
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling South
		for ( j = tile_diameter - 1; j > 0; j-- ) {
		    tiles[(j * tile_diameter) + i] = 
			tiles[((j-1) * tile_diameter) + i];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, i - dw, -dh - 1);
		fgTileMgrLoadTile(&p2, &tiles[0 + i]);
	    }
	}
    }
    p_last.lon = p1.lon;
    p_last.lat = p1.lat;
    p_last.x = p1.x;
    p_last.y = p1.y;
    return 1;
}


// Calculate shortest distance from point to line
static double point_line_dist_squared( fgPoint3d *tc, fgPoint3d *vp, 
				       MAT3vec d )
{
    MAT3vec p, p0;
    double dist;

    p[0] = tc->x; p[1] = tc->y; p[2] = tc->z;
    p0[0] = vp->x; p0[1] = vp->y; p0[2] = vp->z;

    dist = fgPointLineSquared(p, p0, d);

    // printf("dist = %.2f\n", dist);

    return(dist);
}


// Calculate if point/radius is inside view frustum
static int viewable( fgPoint3d *cp, double radius ) {
    fgVIEW *v;
    MAT3hvec world, eye;
    int viewable = 1; // start by assuming it's viewable
    double x0, x1, y1, slope;

    v = &current_view;

    MAT3_SET_HVEC(world, cp->x, cp->y, cp->z, 1.0);
    MAT3mult_vec(eye, world, v->WORLD_TO_EYE);
    // printf( "\nworld -> eye = %.2f %.2f %.2f  radius = %.2f\n", 
    //         eye[0], eye[1], eye[2], radius);

    // Check near clip plane
    if ( eye[2] - radius > 0.0 ) {
	return(0);
    }

    // Check far clip plane
    if ( eye[2] + radius < -current_weather.visibility ) {
	return(0);
    }

    // check right clip plane (from eye perspective)
    // y = m * (x - x0) = equation of a line intercepting X axis at x0
    x1 = v->cos_fov_x * radius;
    y1 = v->sin_fov_x * radius;
    slope = v->slope_x;
    x0 = x1 - y1 / slope;

    // printf("(r) x1 = %.2f  y1 = %.2f\n", x1, y1);
    // printf("eye[0] = %.2f  eye[2] = %.2f\n", eye[0], eye[2]);
    // printf("(r) x0 = %.2f  slope_x = %.2f  radius = %.2f\n", 
    //        x0, slope, radius);

    if ( eye[2] > slope * (eye[0] - x0) ) {
	return(0);
    }

    // check left clip plane (from eye perspective)
    x1 = -x1;
    slope = -slope;
    x0 = x1 - y1 / slope;

    // printf("(r) x1 = %.2f  y1 = %.2f\n", x1, y1);
    // printf("eye[0] = %.2f  eye[2] = %.2f\n", eye[0], eye[2]);
    // printf("(r) x0 = %.2f  slope_x = %.2f  radius = %.2f\n", 
    //        x0, slope, radius);

    if ( eye[2] > slope * (eye[0] - x0) ) {
	return(0);
    }

    // check bottom clip plane (from eye perspective)
    x1 = -(v->cos_fov_y) * radius;
    y1 = v->sin_fov_y * radius;
    slope = v->slope_y;
    x0 = x1 - y1 / slope;

    // printf("(r) x1 = %.2f  y1 = %.2f\n", x1, y1);
    // printf("eye[1] = %.2f  eye[2] = %.2f\n", eye[1], eye[2]);
    // printf("(r) x0 = %.2f  slope_y = %.2f  radius = %.2f\n", 
    //       x0, slope, radius);

    if ( eye[2] > slope * (eye[1] - x0) ) {
	return(0);
    }

    // check top clip plane (from eye perspective)
    x1 = -x1;
    slope = -slope;
    x0 = x1 - y1 / slope;

    // printf("(r) x1 = %.2f  y1 = %.2f\n", x1, y1);
    // printf("eye[1] = %.2f  eye[2] = %.2f\n", eye[1], eye[2]);
    // printf("(r) x0 = %.2f  slope_y = %.2f  radius = %.2f\n", 
    //        x0, slope, radius);

    if ( eye[2] > slope * (eye[1] - x0) ) {
	return(0);
    }

    return(viewable);
}


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double fgTileMgrCurElev( double lon, double lat, fgPoint3d *abs_view_pos ) {
    fgTILECACHE *c;
    fgTILE *t;
    // fgVIEW *v;
    fgFRAGMENT *frag_ptr;
    fgBUCKET p;
    fgPoint3d earth_center, result;
    fgPoint3d pp;
    MAT3vec local_up;
    list < fgFRAGMENT > :: iterator current;
    list < fgFRAGMENT > :: iterator last;
    double dist, min_dist, lat_geod, alt, sea_level_r;
    double x, y, z;
    int index, tile_diameter, i;

    c = &global_tile_cache;
    // v = &current_view;

    local_up[0] = abs_view_pos->x;
    local_up[1] = abs_view_pos->y;
    local_up[2] = abs_view_pos->z;

    tile_diameter = current_options.get_tile_diameter();

    // Find current translation offset
    fgBucketFind(lon * RAD_TO_DEG, lat * RAD_TO_DEG, &p);
    index = c->Exists(&p);
    t = c->GetTile(index);

    scenery.next_center.x = t->center.x;
    scenery.next_center.y = t->center.y;
    scenery.next_center.z = t->center.z;

    earth_center.x = 0.0;
    earth_center.y = 0.0;
    earth_center.z = 0.0;

    fgPrintf( FG_TERRAIN, FG_DEBUG, 
	      "Pos = (%.2f, %.2f) Current bucket = %d %d %d %d  Index = %ld\n", 
	      lon * RAD_TO_DEG, lat * RAD_TO_DEG,
	      p.lon, p.lat, p.x, p.y, fgBucketGenIndex(&p) );

    // traverse the potentially viewable tile list
    for ( i = 0; i < (tile_diameter * tile_diameter); i++ ) {
	index = tiles[i];
	// fgPrintf( FG_TERRAIN, FG_DEBUG, "Index = %d\n", index);
	t = c->GetTile(index);

	// calculate tile offset
	x = (t->offset.x = t->center.x - scenery.center.x);
	y = (t->offset.y = t->center.y - scenery.center.y);
	z = (t->offset.z = t->center.z - scenery.center.z);

	// calc current terrain elevation calculate distance from
	// vertical tangent line at current position to center of
	// tile.
	
	/* printf("distance squared = %.2f, bounding radius = %.2f\n", 
	       point_line_dist_squared(&(t->offset), &(v->view_pos), 
	       v->local_up), t->bounding_radius); */

	dist = point_line_dist_squared( &(t->center), abs_view_pos, 
				        local_up );
	if ( dist < FG_SQUARE(t->bounding_radius) ) {

	    // traverse fragment list for tile
	    current = t->fragment_list.begin();
	    last = t->fragment_list.end();

	    while ( current != last ) {
		frag_ptr = &(*current);
		current++;
		/* printf("distance squared = %.2f, bounding radius = %.2f\n", 
		       point_line_dist_squared( &(frag_ptr->center), 
					&abs_view_pos), local_up),
		       frag_ptr->bounding_radius); */

		dist = point_line_dist_squared( &(frag_ptr->center), 
					abs_view_pos, local_up);
		if ( dist <= FG_SQUARE(frag_ptr->bounding_radius) ) {
		    if ( frag_ptr->intersect( abs_view_pos, 
					      &earth_center, 0, &result ) ) {
			// compute geocentric coordinates of tile center
			pp = fgCartToPolar3d(result);
			// convert to geodetic coordinates
			fgGeocToGeod(pp.lat, pp.radius, &lat_geod, 
				     &alt, &sea_level_r);
			// printf("alt = %.2f\n", alt);
			// exit since we found an intersection
			return(alt);
		    }
		}
	    }
	}
    }

    printf("no terrain intersection found\n");
    return(0);
}


// Render the local tiles
void fgTileMgrRender( void ) {
    fgTILECACHE *c;
    fgFLIGHT *f;
    fgTILE *t, *last_tile_ptr;
    fgVIEW *v;
    fgBUCKET p;
    fgPoint3d frag_offset, pp;
    fgPoint3d earth_center, result;
    fgFRAGMENT *frag_ptr;
    fgMATERIAL *mtl_ptr;
    GLdouble *m;
    double dist, min_dist, lat_geod, alt, sea_level_r;
    double x, y, z;
    list < fgFRAGMENT > :: iterator current;
    list < fgFRAGMENT > :: iterator last;
    int i, j, size;
    int tile_diameter, textures;
    int index;
    int culled = 0;
    int drawn = 0;
    int total_faces = 0;

    c = &global_tile_cache;
    f = current_aircraft.flight;
    v = &current_view;

    tile_diameter = current_options.get_tile_diameter();
    textures = current_options.get_textures();

    // Find current translation offset
    fgBucketFind(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p);
    index = c->Exists(&p);
    t = c->GetTile(index);

    scenery.next_center.x = t->center.x;
    scenery.next_center.y = t->center.y;
    scenery.next_center.z = t->center.z;

    earth_center.x = 0.0;
    earth_center.y = 0.0;
    earth_center.z = 0.0;

    fgPrintf( FG_TERRAIN, FG_DEBUG, 
	      "Pos = (%.2f, %.2f) Current bucket = %d %d %d %d  Index = %ld\n", 
	      FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG,
	      p.lon, p.lat, p.x, p.y, fgBucketGenIndex(&p) );

    // initialize the transient per-material fragment lists
    material_mgr.init_transient_material_lists();
    min_dist = 100000.0;

    // Pass 1
    // traverse the potentially viewable tile list
    for ( i = 0; i < (tile_diameter * tile_diameter); i++ ) {
	index = tiles[i];
	// fgPrintf( FG_TERRAIN, FG_DEBUG, "Index = %d\n", index);
	t = c->GetTile(index);

	// calculate tile offset
	x = (t->offset.x = t->center.x - scenery.center.x);
	y = (t->offset.y = t->center.y - scenery.center.y);
	z = (t->offset.z = t->center.z - scenery.center.z);

	m = t->model_view;
	for ( j = 0; j < 16; j++ ) {
	    m[j] = v->MODEL_VIEW[j];
	}

	// Calculate the model_view transformation matrix for this tile
	// This is equivalent to doing a glTranslatef(x, y, z);
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];

	// temp ... calc current terrain elevation
	// calculate distance from vertical tangent line at
	// current position to center of tile.
	
	/* printf("distance squared = %.2f, bounding radius = %.2f\n", 
	       point_line_dist_squared(&(t->offset), &(v->view_pos), 
	       v->local_up), t->bounding_radius); */

	dist = point_line_dist_squared( &(t->center), &(v->abs_view_pos), 
				        v->local_up );
	if ( dist < FG_SQUARE(t->bounding_radius) ) {

	    // traverse fragment list for tile
	    current = t->fragment_list.begin();
	    last = t->fragment_list.end();

	    while ( current != last ) {
		frag_ptr = &(*current);
		current++;
		/* printf("distance squared = %.2f, bounding radius = %.2f\n", 
		       point_line_dist_squared( &(frag_ptr->center), 
					&(v->abs_view_pos), v->local_up),
		       frag_ptr->bounding_radius); */

		dist = point_line_dist_squared( &(frag_ptr->center), 
					&(v->abs_view_pos), v->local_up);
		if ( dist <= FG_SQUARE(frag_ptr->bounding_radius) ) {
		    if ( frag_ptr->intersect( &(v->abs_view_pos), 
					      &earth_center, 0, &result ) ) {
			// compute geocentric coordinates of tile center
			pp = fgCartToPolar3d(result);
			// convert to geodetic coordinates
			fgGeocToGeod(pp.lat, pp.radius, &lat_geod, 
				     &alt, &sea_level_r);
			// printf("alt = %.2f\n", alt);
			scenery.cur_elev = alt;
			// exit this loop since we found an intersection
			break;
		    }
		}
	    }
	}

	// Course (tile based) culling
	if ( viewable(&(t->offset), t->bounding_radius) ) {
	    // at least a portion of this tile could be viewable
	    
	    // xglPushMatrix();
	    // xglTranslatef(t->offset.x, t->offset.y, t->offset.z);

	    // traverse fragment list for tile
	    current = t->fragment_list.begin();
	    last = t->fragment_list.end();

	    while ( current != last ) {
		frag_ptr = &(*current);
		current++;
		
		if ( frag_ptr->display_list >= 0 ) {
		    // Fine (fragment based) culling
		    frag_offset.x = frag_ptr->center.x - scenery.center.x;
		    frag_offset.y = frag_ptr->center.y - scenery.center.y;
		    frag_offset.z = frag_ptr->center.z - scenery.center.z;

		    if ( viewable(&frag_offset, frag_ptr->bounding_radius*2) ) {
			// add to transient per-material property fragment list
			// frag_ptr->tile_offset.x = t->offset.x;
			// frag_ptr->tile_offset.y = t->offset.y;
			// frag_ptr->tile_offset.z = t->offset.z;

			mtl_ptr = frag_ptr->material_ptr;
			// printf(" lookup = %s\n", mtl_ptr->texture_name);
			if ( mtl_ptr->list_size < FG_MAX_MATERIAL_FRAGS ) {
			    mtl_ptr->list[mtl_ptr->list_size] = frag_ptr;
			    (mtl_ptr->list_size)++;
			} else {
			    fgPrintf( FG_TERRAIN, FG_ALERT,
				      "Overran material sorting array\n" );
			}

			// xglCallList(frag_ptr->display_list);
			drawn++;
		    } else {
			// printf("Culled a fragment %.2f %.2f %.2f %.2f\n",
			//        frag_ptr->center.x, frag_ptr->center.y,
			//        frag_ptr->center.z, frag_ptr->bounding_radius);
			culled++;
		    }
		}
	    }

	    // xglPopMatrix();
	} else {
	    culled += t->fragment_list.size();
	}
    }

    if ( (drawn + culled) > 0 ) {
	v->vfc_ratio = (double)culled / (double)(drawn + culled);
    } else {
	v->vfc_ratio = 0.0;
    }
    // printf("drawn = %d  culled = %d  saved = %.2f\n", drawn, culled, 
    //        v->vfc_ratio);

    // Pass 2
    // traverse the transient per-material fragment lists and render
    // out all fragments for each material property.
    map < string, fgMATERIAL, less<string> > :: iterator mapcurrent = 
	material_mgr.material_map.begin();
    map < string, fgMATERIAL, less<string> > :: iterator maplast = 
	material_mgr.material_map.end();

    xglPushMatrix();

    while ( mapcurrent != maplast ) {
        // (char *)key = (*mapcurrent).first;
        // (fgMATERIAL)value = (*mapcurrent).second;
	mtl_ptr = &(*mapcurrent).second;

	last_tile_ptr = NULL;

	size = mtl_ptr->list_size;
	if ( size > 0 ) {
	    if ( textures ) {
#ifdef GL_VERSION_1_1
		xglBindTexture(GL_TEXTURE_2D, mtl_ptr->texture_id);
#elif GL_EXT_texture_object
		xglBindTextureEXT(GL_TEXTURE_2D, mtl_ptr->texture_id);
#else
#  error port me
#endif
	    } else {
		xglMaterialfv (GL_FRONT, GL_AMBIENT, mtl_ptr->ambient);
		xglMaterialfv (GL_FRONT, GL_DIFFUSE, mtl_ptr->diffuse);
	    }

	    // printf("traversing = %s, size = %d\n", 
	    //       mtl_ptr->texture_name, size);
	    for ( i = 0; i < size; i++ ) {
		frag_ptr = mtl_ptr->list[i];
		
		// count up the number of polygons we are drawing in
		// case someone is interested.
		total_faces += frag_ptr->num_faces;

		if ( frag_ptr->tile_ptr == last_tile_ptr ) {
		    // same tile as last time, no transform necessary
		} else {
		    // new tile, new translate
		    // xglLoadMatrixf( frag_ptr->matrix );
		    t = frag_ptr->tile_ptr;
		    xglLoadMatrixd(t->model_view );
		}
	    
		// Woohoo!!!  We finally get to draw something!
		// printf("  display_list = %d\n", frag_ptr->display_list);
		xglCallList(frag_ptr->display_list);

		last_tile_ptr = frag_ptr->tile_ptr;
	    }
	}

        *mapcurrent++;
    }

    xglPopMatrix();

    v->tris_rendered = total_faces;

    fgPrintf( FG_TERRAIN, FG_DEBUG, "Rendered %d polygons this frame.\n", 
	      total_faces);
}


// $Log$
// Revision 1.32  1998/08/25 16:52:44  curt
// material.cxx material.hxx obj.cxx obj.hxx texload.c texload.h moved to
//   ../Objects
//
// Revision 1.31  1998/08/24 20:11:40  curt
// Tweaks ...
//
// Revision 1.30  1998/08/22  14:49:59  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.29  1998/08/20 15:12:06  curt
// Used a forward declaration of classes fgTILE and fgMATERIAL to eliminate
// the need for "void" pointers and casts.
// Quick hack to count the number of scenery polygons that are being drawn.
//
// Revision 1.28  1998/08/12 21:13:06  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.27  1998/07/24 21:42:09  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.26  1998/07/20 12:51:26  curt
// Added far clip plane to fragment clipping calculations and tie this to
// weather->visibility.  This way you can increase framerates by increasing
// for and lowering visibility.
//
// Revision 1.25  1998/07/13 21:02:01  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.24  1998/07/12 03:18:29  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.23  1998/07/08 14:47:23  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.22  1998/07/04 00:54:31  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.21  1998/06/27 16:54:59  curt
// Check for GL_VERSION_1_1 or GL_EXT_texture_object to decide whether to use
//   "EXT" versions of texture management routines.
//
// Revision 1.20  1998/06/17 21:36:42  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.19  1998/06/08 17:57:54  curt
// Working first pass at material proporty sorting.
//
// Revision 1.18  1998/06/06 01:09:32  curt
// I goofed on the log message in the last commit ... now fixed.
//
// Revision 1.17  1998/06/06 01:07:18  curt
// Increased per material fragment list size from 100 to 400.
// Now correctly draw viewable fragments in per material order.
//
// Revision 1.16  1998/06/05 22:39:55  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.15  1998/06/03 00:47:51  curt
// No .h for STL includes.
// Minor view culling optimizations.
//
// Revision 1.14  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.13  1998/05/24 02:49:10  curt
// Implimented fragment level view frustum culling.
//
// Revision 1.12  1998/05/23 14:09:23  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
// Revision 1.11  1998/05/20 20:53:55  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.10  1998/05/17 16:59:34  curt
// Frist pass at view frustum culling now operational.
//
// Revision 1.9  1998/05/16 13:09:58  curt
// Beginning to add support for view frustum culling.
// Added some temporary code to calculate bouding radius, until the
//   scenery generation tools and scenery can be updated.
//
// Revision 1.8  1998/05/07 23:15:21  curt
// Fixed a glTexImage2D() usage bug where width and height were mis-swapped.
// Added support for --tile-radius=n option.
//
// Revision 1.7  1998/05/06 03:16:42  curt
// Added an option to control square tile radius.
//
// Revision 1.6  1998/05/02 01:52:18  curt
// Playing around with texture coordinates.
//
// Revision 1.5  1998/04/30 12:35:32  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.4  1998/04/27 03:30:14  curt
// Minor transformation adjustments to try to keep scenery tiles closer to
// (0, 0, 0)  GLfloats run out of precision at the distances we need to model
// the earth, but we can do a bunch of pre-transformations using double math
// and then cast to GLfloat once everything is close in where we have less
// precision problems.
//
// Revision 1.3  1998/04/25 22:06:32  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/24 00:51:09  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.1  1998/04/22 13:22:48  curt
// C++ - ifing the code a bit.
//
// Revision 1.25  1998/04/18 04:14:07  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.24  1998/04/14 02:23:18  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.23  1998/04/08 23:30:08  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.22  1998/04/03 22:11:38  curt
// Converting to Gnu autoconf system.
//
// Revision 1.21  1998/03/23 21:23:05  curt
// Debugging output tweaks.
//
// Revision 1.20  1998/03/14 00:30:51  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.19  1998/02/20 00:16:25  curt
// Thursday's tweaks.
//
// Revision 1.18  1998/02/19 13:05:54  curt
// Incorporated some HUD tweaks from Michelle America.
// Tweaked the sky's sunset/rise colors.
// Other misc. tweaks.
//
// Revision 1.17  1998/02/16 13:39:46  curt
// Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
// tiles to occasionally be missing.
//
// Revision 1.16  1998/02/12 21:59:53  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.14  1998/02/09 21:30:19  curt
// Fixed a nagging problem with terrain tiles not "quite" matching up perfectly.
//
// Revision 1.13  1998/02/07 15:29:46  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.12  1998/02/01 03:39:55  curt
// Minor tweaks.
//
// Revision 1.11  1998/01/31 00:43:27  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.10  1998/01/29 00:51:40  curt
// First pass at tile cache, dynamic tile loading and tile unloading now works.
//
// Revision 1.9  1998/01/27 03:26:44  curt
// Playing with new fgPrintf command.
//
// Revision 1.8  1998/01/27 00:48:04  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.7  1998/01/26 15:55:25  curt
// Progressing on building dynamic scenery system.
//
// Revision 1.6  1998/01/24 00:03:30  curt
// Initial revision.
//
// Revision 1.5  1998/01/19 19:27:18  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.4  1998/01/19 18:40:38  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.3  1998/01/13 00:23:11  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.2  1998/01/08 02:22:27  curt
// Continue working on basic features.
//
// Revision 1.1  1998/01/07 23:50:51  curt
// "area" renamed to "tile"
//
// Revision 1.2  1998/01/07 03:29:29  curt
// Given an arbitrary lat/lon, we can now:
//   generate a unique index for the chunk containing the lat/lon
//   generate a path name to the chunk file
//   build a list of the indexes of all the nearby areas.
//
// Revision 1.1  1998/01/07 02:05:48  curt
// Initial revision.
