/**************************************************************************
 * views.h -- data structures and routines for managing and view parameters.
 *
 * Written by Curtis Olson, started August 1997.
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


#ifndef VIEWS_H
#define VIEWS_H


#include "../Include/types.h"
#include "../Flight/flight.h"
#include "../Math/mat3.h"


/* Define a structure containing view information */
struct fgVIEW {
    struct fgCartesianPoint view_pos;
    struct fgCartesianPoint cur_zero_elev;
    MAT3vec local_up, view_up, view_forward;
    double view_offset, goal_view_offset;
};


extern struct fgVIEW current_view;


/* Initialize a view structure */
void fgViewInit(struct fgVIEW *v);

/* Update the view parameters */
void fgViewUpdate(struct fgFLIGHT *f, struct fgVIEW *v);


#endif /* VIEWS_H */


/* $Log$
/* Revision 1.4  1997/12/17 23:13:36  curt
/* Began working on rendering a sky.
/*
 * Revision 1.3  1997/12/15 23:54:51  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/12/10 22:37:48  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/27 21:31:18  curt
 * Initial revision.
 *
 */
