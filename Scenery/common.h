/**************************************************************************
 * common.h -- common utilities and support routines for the parser
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef COMMON_H
#define COMMON_H


/* Maximum length for an identifier */
#define MAX_IDENT_LEN 33  /* 32 + 1 for string terminator */

/* strip the enclosing quotes from a string, works within the current
   string and thus is destructive. */
char *strip_quotes(char *s);


#endif COMMON_H


/* $Log$
/* Revision 1.1  1997/05/16 16:07:04  curt
/* Initial revision.
/*
 */
