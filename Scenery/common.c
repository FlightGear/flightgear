/**************************************************************************
 * common.c -- common utilities and support routines for the parser
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include "common.h"


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
/* Revision 1.1  1997/05/16 16:07:03  curt
/* Initial revision.
/*
 */
