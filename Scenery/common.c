/**************************************************************************
 * common.c -- common utilities and support routines for the parser
 *
 * Written by Curtis Olson, started May 1997.
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


#ifdef WIN32
#include <string.h>
#endif

#include <Scenery/common.h>


/* strip the enclosing quotes from a string, works within the current
   string and thus is destructive. "hello" ==> hello */
char *strip_quotes(char *s) {
    int len;

    /* strip first character */
    s++;

    /* toast last character */
    len = strlen(s);
    s[len-1] = '\0';

    return(s);
}


/* $Log$
/* Revision 1.4  1998/01/19 19:27:14  curt
/* Merged in make system changes from Bob Kuehne <rpk@sgi.com>
/* This should simplify things tremendously.
/*
 * Revision 1.3  1998/01/06 01:20:23  curt
 * Tweaks to help building with MSVC++
 *
 * Revision 1.2  1997/05/23 15:40:41  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:07:03  curt
 * Initial revision.
 *
 */
