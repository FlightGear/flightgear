/* splittris.c -- reassemble the pieces produced by splittris
 *
 * Written by Curtis Olson, started January 1998.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
 * (Log is kept at end of this file) */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>   /* for atoi() */
#include <string.h>
#include <sys/stat.h> /* for stat() */
#include <unistd.h>   /* for stat() */

#include "assemtris.h"

#include <Include/fg_constants.h>
#include <Include/fg_types.h>
// #include <Math/fg_geodesy.h>
// #include <Math/mat3.h>
// #include <Math/polar.h>
#include <Bucket/bucketutils.h>


int nodecount = 0;

static double nodes[MAX_NODES][3];


struct fgBUCKET my_index;
struct fgBUCKET ne_index, nw_index, sw_index, se_index;
struct fgBUCKET north_index, south_index, east_index, west_index;


/* return the file base name ( foo/bar/file.ext = file.ext ) */
void extract_file(char *in, char *base) {
    int len, i;

    len = strlen(in);

    i = len - 1;
    while ( (i >= 0) && (in[i] != '/') ) {
	i--;
    }

    in += (i + 1);
    strcpy(base, in);
}


/* return the file path name ( foo/bar/file.ext = foo/bar ) */
void extract_path(char *in, char *base) {
    int len, i;

    len = strlen(in);
    strcpy(base, in);

    i = len - 1;
    while ( (i >= 0) && (in[i] != '/') ) {
	i--;
    }

    base[i] = '\0';
}


/* check if a file exists */
int file_exists(char *file) {
    struct stat stat_buf;
    int result;

    printf("checking %s ... ", file);

    result = stat(file, &stat_buf);

    if ( result != 0 ) {
	/* stat failed, no file */
	printf("not found.\n");
	return(0);
    } else {
	/* stat succeeded, file exists */
	printf("exists.\n");
	return(1);
    }
}


/* check to see if a shared object exists */
int shared_object_exists(char *basepath, char *ext, char *file) {
    char scene_path[256];
    long int index;

    if ( strcmp(ext, ".sw") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.sw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&west_index, scene_path);
	index = fgBucketGenIndex(&west_index);
	sprintf(file, "%s/%s/%ld.1.se", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&sw_index, scene_path);
	index = fgBucketGenIndex(&sw_index);
	sprintf(file, "%s/%s/%ld.1.ne", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&south_index, scene_path);
	index = fgBucketGenIndex(&south_index);
	sprintf(file, "%s/%s/%ld.1.nw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".se") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.se", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&east_index, scene_path);
	index = fgBucketGenIndex(&east_index);
	sprintf(file, "%s/%s/%ld.1.sw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&se_index, scene_path);
	index = fgBucketGenIndex(&se_index);
	sprintf(file, "%s/%s/%ld.1.nw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&south_index, scene_path);
	index = fgBucketGenIndex(&south_index);
	sprintf(file, "%s/%s/%ld.1.ne", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".ne") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.ne", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&east_index, scene_path);
	index = fgBucketGenIndex(&east_index);
	sprintf(file, "%s/%s/%ld.1.nw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&ne_index, scene_path);
	index = fgBucketGenIndex(&ne_index);
	sprintf(file, "%s/%s/%ld.1.sw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&north_index, scene_path);
	index = fgBucketGenIndex(&north_index);
	sprintf(file, "%s/%s/%ld.1.se", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".nw") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.nw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&west_index, scene_path);
	index = fgBucketGenIndex(&west_index);
	sprintf(file, "%s/%s/%ld.1.ne", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&nw_index, scene_path);
	index = fgBucketGenIndex(&nw_index);
	sprintf(file, "%s/%s/%ld.1.se", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&north_index, scene_path);
	index = fgBucketGenIndex(&north_index);
	sprintf(file, "%s/%s/%ld.1.sw", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".south") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.south", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&south_index, scene_path);
	index = fgBucketGenIndex(&south_index);
	sprintf(file, "%s/%s/%ld.1.north", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".north") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.north", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&north_index, scene_path);
	index = fgBucketGenIndex(&north_index);
	sprintf(file, "%s/%s/%ld.1.south", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".west") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.west", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&west_index, scene_path);
	index = fgBucketGenIndex(&west_index);
	sprintf(file, "%s/%s/%ld.1.east", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".east") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.east", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
	fgBucketGenBasePath(&east_index, scene_path);
	index = fgBucketGenIndex(&east_index);
	sprintf(file, "%s/%s/%ld.1.west", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    if ( strcmp(ext, ".body") == 0 ) {
	fgBucketGenBasePath(&my_index, scene_path);
	index = fgBucketGenIndex(&my_index);
	sprintf(file, "%s/%s/%ld.1.body", basepath, scene_path, index);
	if ( file_exists(file) ) {
	    return(1);
	}
    }

    return(0);
}


/* my custom file opening routine ... don't open if a shared edge or
 * vertex alread exists */
FILE *my_open(char *basename, char *basepath, char *ext) {
    FILE *fp;
    char filename[256];

    /* check if a shared object already exists */
    if ( shared_object_exists(basepath, ext, filename) ) {
	/* not an actual file open error, but we've already got the
         * shared edge, so we don't want to create another one */
	fp = fopen(filename, "r");
	printf("Opening %s\n", filename);
	return(fp);
    } else {
	/* open the file */
	printf("not opening\n");
	return(NULL);
    }
}


/* given a file pointer, read all the gdn (geodetic nodes from it.)
   The specified offset values (in arcsec) are used to overlap the
   edges of the tile slightly to cover gaps induced by floating point
   precision problems.  1 arcsec == about 100 feet so 0.01 arcsec ==
   about 1 foot */
void read_nodes(FILE *fp, double offset_lon, double offset_lat) {
    char line[256];

    while ( fgets(line, 250, fp) != NULL ) {
	if ( strncmp(line, "gdn ", 4) == 0 ) {
	    sscanf(line, "gdn %lf %lf %lf\n", &nodes[nodecount][0], 
		   &nodes[nodecount][1], &nodes[nodecount][2]);

	    nodes[nodecount][0] += offset_lon;
	    nodes[nodecount][1] += offset_lat;

	    /*
	    printf("read_nodes(%d) %.2f %.2f %.2f %s", nodecount, 
		   nodes[nodecount][0], nodes[nodecount][1], 
		   nodes[nodecount][2], line);
		   */
	    nodecount++;
	}
    }
}


/* load in nodes from the various split and shared pieces to
 * reconstruct a tile */
void build_node_list(char *basename, char *basepath) {
    FILE *ne, *nw, *se, *sw, *north, *south, *east, *west, *body;

    ne = my_open(basename, basepath, ".ne");
    read_nodes(ne, 0.1, 0.1);
    fclose(ne);

    nw = my_open(basename, basepath, ".nw");
    read_nodes(nw, -0.1, 0.1);
    fclose(nw);

    se = my_open(basename, basepath, ".se");
    read_nodes(se, 0.1, -0.1);
    fclose(se);

    sw = my_open(basename, basepath, ".sw");
    read_nodes(sw, -0.1, -0.1);
    fclose(sw);

    north = my_open(basename, basepath, ".north");
    read_nodes(north, 0.0, 0.1);
    fclose(north);

    south = my_open(basename, basepath, ".south");
    read_nodes(south, 0.0, -0.1);
    fclose(south);

    east = my_open(basename, basepath, ".east");
    read_nodes(east, 0.1, 0.0);
    fclose(east);

    west = my_open(basename, basepath, ".west");
    read_nodes(west, -0.1, 0.0);
    fclose(west);

    body = my_open(basename, basepath, ".body");
    read_nodes(body, 0.0, 0.0);
    fclose(body);
}


/* dump in WaveFront .obj format */
void dump_nodes(char *basename) {
    char file[256];
    FILE *fp;
    int i, len;

    /* generate output file name */
    strcpy(file, basename);
    len = strlen(file);
    file[len-2] = '\0';
    strcat(file, ".node");
    
    /* dump vertices */
    printf("Creating node file:  %s\n", file);
    printf("  writing vertices in .node format.\n");
    fp = fopen(file, "w");

    fprintf(fp, "%d 2 1 0\n", nodecount);

    /* now write out actual node data */
    for ( i = 0; i < nodecount; i++ ) {
	fprintf(fp, "%d %.2f %.2f %.2f 0\n", i + 1,
	       nodes[i][0], nodes[i][1], nodes[i][2]);
    }

    fclose(fp);
}


int main(int argc, char **argv) {
    char basename[256], basepath[256], temp[256];
    long int tmp_index;
    int len;

    strcpy(basename, argv[1]);

    /* find the base path of the file */
    extract_path(basename, basepath);
    extract_path(basepath, basepath);
    extract_path(basepath, basepath);
    printf("%s\n", basepath);

    /* find the index of the current file */
    extract_file(basename, temp);
    len = strlen(temp);
    if ( len >= 2 ) {
	temp[len-2] = '\0';
    }
    tmp_index = atoi(temp);
    printf("%ld\n", tmp_index);
    fgBucketParseIndex(tmp_index, &my_index);

    printf("bucket = %d %d %d %d\n", 
	   my_index.lon, my_index.lat, my_index.x, my_index.y);
    /* generate the indexes of the neighbors */
    fgBucketOffset(&my_index, &ne_index,  1,  1);
    fgBucketOffset(&my_index, &nw_index, -1,  1);
    fgBucketOffset(&my_index, &se_index,  1, -1);
    fgBucketOffset(&my_index, &sw_index, -1, -1);

    fgBucketOffset(&my_index, &north_index,  0,  1);
    fgBucketOffset(&my_index, &south_index,  0, -1);
    fgBucketOffset(&my_index, &east_index,  1,  0);
    fgBucketOffset(&my_index, &west_index, -1,  0);

    /*
    printf("Corner indexes = %ld %ld %ld %ld\n", 
	   ne_index, nw_index, sw_index, se_index);
    printf("Edge indexes = %ld %ld %ld %ld\n",
	   north_index, south_index, east_index, west_index);
	   */

    /* load the input data files */
    build_node_list(basename, basepath);

    /* dump in WaveFront .obj format */
    dump_nodes(basename);

    return(0);
}


/* $Log$
/* Revision 1.8  1998/06/01 17:58:19  curt
/* Added a slight border overlap to try to minimize pixel wide gaps between
/* tiles due to round off error.  This is not a perfect solution, but helps.
/*
 * Revision 1.7  1998/04/14 02:26:00  curt
 * Code reorganizations.  Added a Lib/ directory for more general libraries.
 *
 * Revision 1.6  1998/04/08 22:54:58  curt
 * Adopted Gnu automake/autoconf system.
 *
 * Revision 1.5  1998/03/03 16:00:52  curt
 * More c++ compile tweaks.
 *
 * Revision 1.4  1998/01/31 00:41:23  curt
 * Made a few changes converting floats to doubles.
 *
 * Revision 1.3  1998/01/27 18:37:00  curt
 * Lots of updates to get back in sync with changes made over in .../Src/
 *
 * Revision 1.2  1998/01/15 21:33:36  curt
 * Assembling triangles and building a new .node file with the proper shared
 * vertices now works.  Now we just have to use the shared normals and we'll
 * be all set.
 *
 * Revision 1.1  1998/01/15 02:45:26  curt
 * Initial revision.
 *
 */
