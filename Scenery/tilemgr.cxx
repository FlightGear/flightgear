/* -*- Mode: C++ -*-
 *
 * tilemgr.c -- routines to handle dynamic management of scenery tiles
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Scenery/obj.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilecache.hxx>

#include <Aircraft/aircraft.h>
#include <Bucket/bucketutils.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/fg_types.h>


#define FG_LOCAL_X           7   /* should be odd */
#define FG_LOCAL_Y           7   /* should be odd */
#define FG_LOCAL_X_Y         49  /* At least FG_LOCAL_X times FG_LOCAL_Y */


/* closest (potentially viewable) tiles, centered on current tile.
 * This is an array of pointers to cache indexes. */
int tiles[FG_LOCAL_X_Y];


/* Initialize the Tile Manager subsystem */
int fgTileMgrInit( void ) {
    fgPrintf( FG_TERRAIN, FG_INFO, "Initializing Tile Manager subsystem.\n");
    return 1;
}


/* load a tile */
void fgTileMgrLoadTile( struct fgBUCKET *p, int *index) {
    fgPrintf( FG_TERRAIN, FG_DEBUG, "Updating for bucket %d %d %d %d\n", 
	   p->lon, p->lat, p->x, p->y);
    
    /* if not in cache, load tile into the next available slot */
    if ( (*index = fgTileCacheExists(p)) < 0 ) {
	*index = fgTileCacheNextAvail();
	fgTileCacheEntryFillIn(*index, p);
    }

    fgPrintf( FG_TERRAIN, FG_DEBUG, "Selected cache index of %d\n", *index);
}


/* given the current lon/lat, fill in the array of local chunks.  If
 * the chunk isn't already in the cache, then read it from disk. */
int fgTileMgrUpdate( void ) {
    fgFLIGHT *f;
    struct fgBUCKET p1, p2;
    static struct fgBUCKET p_last = {-1000, 0, 0, 0};
    int i, j, dw, dh;

    f = current_aircraft.flight;

    fgBucketFind(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p1);
    dw = FG_LOCAL_X / 2;
    dh = FG_LOCAL_Y / 2;

    if ( (p1.lon == p_last.lon) && (p1.lat == p_last.lat) &&
	 (p1.x == p_last.x) && (p1.y == p_last.y) ) {
	/* same bucket as last time */
	fgPrintf( FG_TERRAIN, FG_DEBUG, "Same bucket as last time\n");
    } else if ( p_last.lon == -1000 ) {
	/* First time through, initialize the system and load all
         * relavant tiles */

	fgPrintf( FG_TERRAIN, FG_DEBUG, "First time through ... \n");
	fgPrintf( FG_TERRAIN, FG_DEBUG, "Updating Tile list for %d,%d %d,%d\n",
		  p1.lon, p1.lat, p1.x, p1.y);

	/* wipe tile cache */
	fgTileCacheInit();

	/* build the local area list and update cache */
	for ( j = 0; j < FG_LOCAL_Y; j++ ) {
	    for ( i = 0; i < FG_LOCAL_X; i++ ) {
		fgBucketOffset(&p1, &p2, i - dw, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*FG_LOCAL_Y) + i]);
	    }
	}
    } else {
	/* We've moved to a new bucket, we need to scroll our
         * structures, and load in the new tiles */

	/* CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
           AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
           THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION. */

	if ( (p1.lon > p_last.lon) ||
	     ( (p1.lon == p_last.lon) && (p1.x > p_last.x) ) ) {
	    for ( j = 0; j < FG_LOCAL_Y; j++ ) {
		/* scrolling East */
		for ( i = 0; i < FG_LOCAL_X - 1; i++ ) {
		    tiles[(j*FG_LOCAL_Y) + i] = tiles[(j*FG_LOCAL_Y) + i + 1];
		}
		/* load in new column */
		fgBucketOffset(&p_last, &p2, dw + 1, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*FG_LOCAL_Y) + FG_LOCAL_X - 1]);
	    }
	} else if ( (p1.lon < p_last.lon) ||
		    ( (p1.lon == p_last.lon) && (p1.x < p_last.x) ) ) {
	    for ( j = 0; j < FG_LOCAL_Y; j++ ) {
		/* scrolling West */
		for ( i = FG_LOCAL_X - 1; i > 0; i-- ) {
		    tiles[(j*FG_LOCAL_Y) + i] = tiles[(j*FG_LOCAL_Y) + i - 1];
		}
		/* load in new column */
		fgBucketOffset(&p_last, &p2, -dw - 1, j - dh);
		fgTileMgrLoadTile(&p2, &tiles[(j*FG_LOCAL_Y) + 0]);
	    }
	}

	if ( (p1.lat > p_last.lat) ||
	     ( (p1.lat == p_last.lat) && (p1.y > p_last.y) ) ) {
	    for ( i = 0; i < FG_LOCAL_X; i++ ) {
		/* scrolling North */
		for ( j = 0; j < FG_LOCAL_Y - 1; j++ ) {
		    tiles[(j * FG_LOCAL_Y) + i] =
			tiles[((j+1) * FG_LOCAL_Y) + i];
		}
		/* load in new column */
		fgBucketOffset(&p_last, &p2, i - dw, dh + 1);
		fgTileMgrLoadTile(&p2, &tiles[((FG_LOCAL_Y-1)*FG_LOCAL_Y) + i]);
	    }
	} else if ( (p1.lat < p_last.lat) ||
		    ( (p1.lat == p_last.lat) && (p1.y < p_last.y) ) ) {
	    for ( i = 0; i < FG_LOCAL_X; i++ ) {
		/* scrolling South */
		for ( j = FG_LOCAL_Y - 1; j > 0; j-- ) {
		    tiles[(j * FG_LOCAL_Y) + i] = 
			tiles[((j-1) * FG_LOCAL_Y) + i];
		}
		/* load in new column */
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


/* Render the local tiles */
void fgTileMgrRender( void ) {
    fgFLIGHT *f;
    struct fgBUCKET p;
    fgCartesianPoint3d local_ref;
    GLint display_list;
    int i;
    int index;

    f = current_aircraft.flight;

    /* Find current translation offset */
    fgBucketFind(FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG, &p);
    index = fgTileCacheExists(&p);
    fgTileCacheEntryInfo(index, &display_list, &scenery.next_center );

    printf("Pos = (%.2f, %.2f) Current bucket = %d %d %d %d  Index = %ld\n", 
	   FG_Longitude * RAD_TO_DEG, FG_Latitude * RAD_TO_DEG,
	   p.lon, p.lat, p.x, p.y, fgBucketGenIndex(&p) );

    for ( i = 0; i < FG_LOCAL_X_Y; i++ ) {
	index = tiles[i];
	/* fgPrintf( FG_TERRAIN, FG_DEBUG, "Index = %d\n", index); */
	fgTileCacheEntryInfo(index, &display_list, &local_ref );

	if ( display_list >= 0 ) {
	    xglPushMatrix();
	    xglTranslatef(local_ref.x - scenery.center.x,
			  local_ref.y - scenery.center.y,
			  local_ref.z - scenery.center.z);
	    /* xglTranslatef(-scenery.center.x, -scenery.center.y, 
			  -scenery.center.z); */
	    xglCallList(display_list);
	    xglPopMatrix();
	}
    }
}


/* $Log$
/* Revision 1.6  1998/05/02 01:52:18  curt
/* Playing around with texture coordinates.
/*
 * Revision 1.5  1998/04/30 12:35:32  curt
 * Added a command line rendering option specify smooth/flat shading.
 *
 * Revision 1.4  1998/04/27 03:30:14  curt
 * Minor transformation adjustments to try to keep scenery tiles closer to
 * (0, 0, 0)  GLfloats run out of precision at the distances we need to model
 * the earth, but we can do a bunch of pre-transformations using double math
 * and then cast to GLfloat once everything is close in where we have less
 * precision problems.
 *
 * Revision 1.3  1998/04/25 22:06:32  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.2  1998/04/24 00:51:09  curt
 * Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
 * Tweaked the scenery file extentions to be "file.obj" (uncompressed)
 * or "file.obz" (compressed.)
 *
 * Revision 1.1  1998/04/22 13:22:48  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.25  1998/04/18 04:14:07  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.24  1998/04/14 02:23:18  curt
 * Code reorganizations.  Added a Lib/ directory for more general libraries.
 *
 * Revision 1.23  1998/04/08 23:30:08  curt
 * Adopted Gnu automake/autoconf system.
 *
 * Revision 1.22  1998/04/03 22:11:38  curt
 * Converting to Gnu autoconf system.
 *
 * Revision 1.21  1998/03/23 21:23:05  curt
 * Debugging output tweaks.
 *
 * Revision 1.20  1998/03/14 00:30:51  curt
 * Beginning initial terrain texturing experiments.
 *
 * Revision 1.19  1998/02/20 00:16:25  curt
 * Thursday's tweaks.
 *
 * Revision 1.18  1998/02/19 13:05:54  curt
 * Incorporated some HUD tweaks from Michelle America.
 * Tweaked the sky's sunset/rise colors.
 * Other misc. tweaks.
 *
 * Revision 1.17  1998/02/16 13:39:46  curt
 * Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
 * tiles to occasionally be missing.
 *
 * Revision 1.16  1998/02/12 21:59:53  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.14  1998/02/09 21:30:19  curt
 * Fixed a nagging problem with terrain tiles not "quite" matching up perfectly.
 *
 * Revision 1.13  1998/02/07 15:29:46  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.12  1998/02/01 03:39:55  curt
 * Minor tweaks.
 *
 * Revision 1.11  1998/01/31 00:43:27  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.10  1998/01/29 00:51:40  curt
 * First pass at tile cache, dynamic tile loading and tile unloading now works.
 *
 * Revision 1.9  1998/01/27 03:26:44  curt
 * Playing with new fgPrintf command.
 *
 * Revision 1.8  1998/01/27 00:48:04  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.7  1998/01/26 15:55:25  curt
 * Progressing on building dynamic scenery system.
 *
 * Revision 1.6  1998/01/24 00:03:30  curt
 * Initial revision.
 *
 * Revision 1.5  1998/01/19 19:27:18  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.4  1998/01/19 18:40:38  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.3  1998/01/13 00:23:11  curt
 * Initial changes to support loading and management of scenery tiles.  Note,
 * there's still a fair amount of work left to be done.
 *
 * Revision 1.2  1998/01/08 02:22:27  curt
 * Continue working on basic features.
 *
 * Revision 1.1  1998/01/07 23:50:51  curt
 * "area" renamed to "tile"
 *
 * Revision 1.2  1998/01/07 03:29:29  curt
 * Given an arbitrary lat/lon, we can now:
 *   generate a unique index for the chunk containing the lat/lon
 *   generate a path name to the chunk file
 *   build a list of the indexes of all the nearby areas.
 *
 * Revision 1.1  1998/01/07 02:05:48  curt
 * Initial revision.
 * */


