/**************************************************************************
 * general.h -- a general house keeping data structure definition for 
 *              various info that might need to be accessible from all 
 *              parts of the sim.
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


#ifndef GENERAL_H
#define GENERAL_H


/* the general house keeping structure definition */
struct fgGENERAL {
    /* The flight gear "root" directory */
    char *root_dir;
};

/* general contains all the general house keeping parameters. */
extern struct fgGENERAL general;

#endif /* GENERAL_H */


/* $Log$
/* Revision 1.3  1997/12/10 22:37:34  curt
/* Prepended "fg" on the name of all global structures that didn't have it yet.
/* i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
/*
 * Revision 1.2  1997/08/27 03:29:38  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.1  1997/08/23 11:37:12  curt
 * Initial revision.
 *
 */
