/**************************************************************************
 * obj.c -- routines to handle WaveFront .obj format files.
 *
 * Written by Curtis Olson, started October 1997.
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


#include <stdio.h>
#include <string.h>

#include "obj.h"

#include "../../Src/Math/mat3.h"


/* what do ya' know, here's some global variables */
double nodes[MAXNODES][3];
double normals[MAXNODES][3];

int ccw_list[MAXNODES];
int ccw_list_ptr;

int cw_list[MAXNODES];
int cw_list_ptr;

FILE *in, *out;


/* some simple list routines */

/* reset the list */
void list_init(int *list_ptr) {
    *list_ptr = 0;
}


/* add to list */
void list_add(int *list, int *list_ptr, int node) {
    if ( *list_ptr >= MAXNODES ) {
	printf("ERROR: list overflow in list_add()\n");
	exit(-1);
    }

    list[*list_ptr] = node;
    *list_ptr += 1;

    /* printf("list pointer = %d  adding %d\n", *list_ptr, node); */
}


/* dump list */
void dump_list(int *list, int list_ptr) {
    int i;

    if ( list_ptr < 3 ) {
	printf("List is empty ... skipping\n");
	return;
    }

    printf("Dumping list, size = %d\n", list_ptr);

    i = 0;
    while ( i < list_ptr ) { 
	/* do next strip */

	/* dump header */
	fprintf(out, "t %d %d %d\n", list[i], list[i+1], list[i+2]);
	/* printf("t %d %d %d\n", list[i], list[i+1], list[i+2]); */
	i += 3;

	/* dump rest of strip (until -1) */
	while ( (i < list_ptr) && (list[i] != -1) ) { 
	    fprintf(out, "q %d", list[i]);
	    i++;
	    if ( (i < list_ptr) && (list[i] != -1) ) { 
		fprintf(out, " %d", list[i]);
		i++;
	    }
	    fprintf(out, "\n");
	}

	i++;
    }
}


/* Check the direction the current triangle faces, compared to it's
 * pregenerated normal.  Returns the dot product between the target
 * normal and actual normal.  If the dot product is close to 1.0, they
 * nearly match.  If the are close to -1.0, the are nearly
 * opposite. */
double check_cur_face(int n1, int n2, int n3) {
    double v1[3], v2[3], approx_normal[3], dot_prod, temp;

    /* check for the proper rotation by calculating an approximate
     * normal and seeing if it is close to the precalculated normal */
    v1[0] = nodes[n2][0] - nodes[n1][0];
    v1[1] = nodes[n2][1] - nodes[n1][1];
    v1[2] = nodes[n2][2] - nodes[n1][2];
    v2[0] = nodes[n3][0] - nodes[n1][0];
    v2[1] = nodes[n3][1] - nodes[n1][1];
    v2[2] = nodes[n3][2] - nodes[n1][2];

    MAT3cross_product(approx_normal, v1, v2);
    MAT3_NORMALIZE_VEC(approx_normal,temp);
    dot_prod = MAT3_DOT_PRODUCT(normals[n1], approx_normal);

    /* not first triangle */
    /* if ( ((dot_prod < -0.5) && !is_backwards) ||
	 ((dot_prod >  0.5) && is_backwards) ) {
	printf("    Approx normal = %.2f %.2f %.2f\n", approx_normal[0], 
	       approx_normal[1], approx_normal[2]);
	printf("    Dot product = %.4f\n", dot_prod);
    } */
    /* angle = acos(dot_prod); */
    /* printf("Normal ANGLE = %.3f rads.\n", angle); */

    return(dot_prod);
}


/* Load a .obj file */
void obj_fix(char *infile, char *outfile) {
    char line[256];
    double dot_prod;
    int first, ncount, vncount, n1, n2, n3, n4;
    int is_ccw;

    if ( (in = fopen(infile, "r")) == NULL ) {
	printf("Cannot open file: %s\n", infile);
	exit(-1);
    }

    if ( (out = fopen(outfile, "w")) == NULL ) {
	printf("Cannot open file: %s\n", outfile);
	exit(-1);
    }

    list_init(&ccw_list_ptr);
    list_init(&cw_list_ptr);

    first = 1;
    ncount = 1;
    vncount = 1;

    printf("Reading file:  %s\n", infile);

    while ( fgets(line, 250, in) != NULL ) {
	if ( line[0] == '#' ) {
	    /* pass along the comments verbatim */
	    fprintf(out, "%s", line);
	} else if ( strlen(line) <= 1 ) {
	    /* pass along empty lines */
	    fprintf(out, "%s", line);
	} else if ( strncmp(line, "v ", 2) == 0 ) {
	    /* save vertex to memory and output to file */
            if ( ncount < MAXNODES ) {
                /* printf("vertex = %s", line); */
                sscanf(line, "v %lf %lf %lf\n", 
                       &nodes[ncount][0], &nodes[ncount][1], &nodes[ncount][2]);
		fprintf(out, "v %.2f %.2f %.2f\n", 
		       nodes[ncount][0], nodes[ncount][1], nodes[ncount][2]);
		ncount++;
            } else {
                printf("Read too many nodes ... dying :-(\n");
                exit(-1);
            }
	} else if ( strncmp(line, "vn ", 3) == 0 ) {
	    /* save vertex normals to memory and output to file */
            if ( vncount < MAXNODES ) {
                /* printf("vertex normal = %s", line); */
                sscanf(line, "vn %lf %lf %lf\n", 
                       &normals[vncount][0], &normals[vncount][1], 
                       &normals[vncount][2]);
		fprintf(out, "vn %.4f %.4f %.4f\n", normals[vncount][0], 
			normals[vncount][1], normals[vncount][2]);
                vncount++;
            } else {
                printf("Read too many vertex normals ... dying :-(\n");
                exit(-1);
            }
	} else if ( line[0] == 't' ) {
	    /* starting a new triangle strip */

	    printf("Starting a new triangle strip\n");

	    n1 = n2 = n3 = n4 = 0;

	    printf("new tri strip = %s", line);
	    sscanf(line, "t %d %d %d %d\n", &n1, &n2, &n3, &n4);

	    /* special case to handle a bug in our beloved tri striper */
	    if ( (n1 == 4) && (n2 == 2) && (n3 == 2) && (n4 == 1) ) {
		n2 = 3;
	    }

	    dot_prod = check_cur_face(n1, n2, n3);
	    if ( dot_prod < 0.0 ) {
		/* this stripe is backwards (CW) */
		is_ccw = 0;
		printf(" -> Starting a backwards stripe\n");
	    } else {
		/* this stripe is normal (CCW) */
		is_ccw = 1;
	    }

	    if ( is_ccw ) {
		if ( ccw_list_ptr ) {
		    list_add(ccw_list, &ccw_list_ptr, -1);
		}

		list_add(ccw_list, &ccw_list_ptr, n1);
		list_add(ccw_list, &ccw_list_ptr, n2);
		list_add(ccw_list, &ccw_list_ptr, n3);
	    } else {
		if ( cw_list_ptr ) {
		    list_add(cw_list, &cw_list_ptr, -1);
		}

		list_add(cw_list, &cw_list_ptr, n1);
		list_add(cw_list, &cw_list_ptr, n2);
		list_add(cw_list, &cw_list_ptr, n3);
	    }

	    if ( n4 > 0 ) {
		if ( is_ccw ) {
		    list_add(ccw_list, &ccw_list_ptr, n4);
		} else {
		    list_add(cw_list, &cw_list_ptr, n4);
		}
	    }
	} else if ( line[0] == 'f' ) {
	    /* pass along the unoptimized faces verbatim */
	    fprintf(out, "%s", line);
	} else if ( line[0] == 'q' ) {
	    /* continue a triangle strip */
	    n1 = n2 = 0;

	    /* printf("continued tri strip = %s ", line); */
	    sscanf(line, "q %d %d\n", &n1, &n2);

	    if ( is_ccw ) {
		list_add(ccw_list, &ccw_list_ptr, n1);
	    } else {
		list_add(cw_list, &cw_list_ptr, n1);
	    }

	    if ( n2 > 0 ) {
		if ( is_ccw ) {
		    list_add(ccw_list, &ccw_list_ptr, n2);
		} else {
		    list_add(cw_list, &cw_list_ptr, n2);
		}
	    }
	} else {
	    printf("Unknown line in %s = %s\n", infile, line);
	}
    }

    fprintf(out, "winding ccw\n");
    dump_list(ccw_list, ccw_list_ptr);

    fprintf(out, "winding cw\n");
    dump_list(cw_list, cw_list_ptr);

    fclose(in);
    fclose(out);
}


/* $Log$
/* Revision 1.5  1998/03/03 03:37:03  curt
/* Cumulative tweaks.
/*
 * Revision 1.4  1998/01/31 00:41:25  curt
 * Made a few changes converting floats to doubles.
 *
 * Revision 1.3  1998/01/19 19:51:07  curt
 * A couple final pre-release tweaks.
 *
 * Revision 1.2  1998/01/09 23:03:12  curt
 * Restructured to split 1deg x 1deg dem's into 64 subsections.
 *
 * Revision 1.1  1997/12/08 19:28:54  curt
 * Initial revision.
 *
 */
