/**************************************************************************
 * common.h -- common utilities and support routines for the parser
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


#ifndef _COMMON_H
#define _COMMON_H


/* Maximum length for an identifier */
#define MAX_IDENT_LEN 33  /* 32 + 1 for string terminator */

/* strip the enclosing quotes from a string, works within the current
   string and thus is destructive. */
char *strip_quotes(char *s);


#endif /* _COMMON_H */


/* $Log$
/* Revision 1.4  1998/01/22 02:59:40  curt
/* Changed #ifdef FILE_H to #ifdef _FILE_H
/*
 * Revision 1.3  1997/07/23 21:52:24  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.2  1997/05/23 15:40:41  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:07:04  curt
 * Initial revision.
 *
 */
