/**************************************************************************
 * sky.h -- model sky with an upside down "bowl"
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


/* (Re)generate the display list */
void fgSkyInit();

/* (Re)calculate the sky colors at each vertex */
void fgSkyColorsInit();

/* Draw the Sky */
void fgSkyRender();


/* $Log$
/* Revision 1.2  1997/12/22 23:45:49  curt
/* First stab at sunset/sunrise sky glow effects.
/*
 * Revision 1.1  1997/12/17 23:14:31  curt
 * Initial revision.
 * Begin work on rendering the sky. (Rather than just using a clear screen.)
 *
 */
