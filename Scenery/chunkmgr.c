/**************************************************************************
 * chunkmgr.c -- top level scenery chunk management system.
 *
 * Written by Curtis Olson, started July 1997.
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


#include "chunkmgr.h"


/* we'll use a fixed sized chunk list for now */
static struct chunk chunk_list[MAX_CHUNK_LIST];

/* ... and a fixed sized chunk cache */
static struct chunk chunk_cache[MAX_CHUNK_CACHE];


/* initialize the chunk management system */
void chunk_init() {
}


/* load all chunks within radius distance of the specified point if
 * they are not already loaded or cached.  Remove or cache any chunks that are
 * out of range. */
void chunk_update(double lat, double lon, double radius) {
}


/* $Log$
/* Revision 1.1  1997/07/31 23:19:33  curt
/* Initial revision.
/*
 */
