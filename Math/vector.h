/**************************************************************************
 * vector.h -- additional vector routines
 *
 * Written by Curtis Olson, started December 1997.
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


#ifndef _VECTOR_H
#define _VECTOR_H


#include <Math/mat3.h>


/* Map a vector onto the plane specified by normal */
void map_vec_onto_cur_surface_plane(MAT3vec normal, MAT3vec v0, MAT3vec vec,
				    MAT3vec result);


#endif /* _VECTOR_H */


/* $Log$
/* Revision 1.3  1998/01/22 02:59:39  curt
/* Changed #ifdef FILE_H to #ifdef _FILE_H
/*
 * Revision 1.2  1998/01/19 19:27:14  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.1  1997/12/22 04:13:18  curt
 * Initial revision.
 *
 */
