/* -*- Mode: C++ -*-
 *
 * fg_debug.c -- Flight Gear debug utility functions
 *
 * Written by Paul Bleisch, started January 1998. 
 *
 * Copyright (C) 1998 Paul Bleisch, pbleisch@acm.org
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
 **************************************************************************/


#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <Include/cmdargs.h> // Line to command line arguments

#include "fg_debug.h"


static int             fg_DebugSem      = 1;
fgDebugClass           fg_DebugClass    = FG_NONE;  // Need visibility for
fgDebugPriority        fg_DebugPriority = FG_INFO; // command line processing.
static fgDebugCallback fg_DebugCallback = NULL;

FILE *fg_DebugOutput = NULL;  // Visibility needed for command line processor.
                              // This can be set to a FILE from the command
                              // line. If not, it will be set to stderr.

/* TODO: Actually make this thing thread safe */
#ifdef USETHREADS
#define FG_GRABDEBUGSEM  while( --fg_DebugSem < 0 ) { fg_DebugSem++; }
#define FG_RELEASEDEBUGSEM fg_DebugSem++;
#else
#define FG_GRABDEBUGSEM
#define FG_RELEASEDEBUGSEM
#endif

/* Used for convienence initialization from env variables.
 */
static struct {
    char *str;
    fgDebugClass dbg_class;
} fg_DebugClasses[] = {
    { "FG_NONE",      0x00000000 },
    { "FG_TERRAIN",   0x00000001 },
    { "FG_ASTRO",     0x00000002 },
    { "FG_FLIGHT",    0x00000004 },
    { "FG_INPUT",     0x00000008 },
    { "FG_GL",        0x00000010 },
    { "FG_VIEW",      0x00000020 },
    { "FG_COCKPIT",   0x00000040 },
    { "FG_GENERAL",   0x00000080 },
    { "FG_MATH",      0x00000100 },
    { "FG_EVENT",     0x00000200 },
    { "FG_AIRCRAFT",  0x00000400 },
    { "FG_AUTOPILOT", 0x00000800 },

    /* Do not edit below here, last entry should be null */
    { "FG_ALL",       0xFFFFFFFF },
    { NULL, 0 } 
};

static fgDebugClass fgDebugStrToClass( char *str );


/* fgInitDebug =============================================================*/
void fgInitDebug( void ) {
    char *pszClass, *pszPrio, *pszFile;

    // Support for log file/alt debug output via command line, environment or
    // reasonable default.

    /*
    if( strlen( logArgbuf ) > 3) {   // First check for command line option
	// Assumed that we will append.
	fg_DebugOutput = fopen(logArgbuf, "a+" );
    }
    */

    if( !fg_DebugOutput ) {          // If not set on command line, environment?
	pszFile = getenv( "FG_DEBUGFILE" );
	if( pszFile ) {          // There is such an environmental variable.
	    fg_DebugOutput = fopen( pszFile, "a+" );
	}
    }

    if( !fg_DebugOutput ) {         // If neither command line nor environment
	fg_DebugOutput = stderr;      // then we use the fallback position
    }
    
    FG_GRABDEBUGSEM;
    fg_DebugSem = fg_DebugSem;  /* shut up GCC */

    // Test command line option overridge of debug priority. If the value
    // is in range (properly optioned) the we will override both defaults
    // and the environmental value.

    /*
    if ((priorityArgValue >= FG_BULK) && (priorityArgValue <= FG_ABORT)) {
	fg_DebugPriority = priorityArgValue;
    } else {  // Either not set or out of range. We will not warn the user.
    */
	pszPrio = getenv( "FG_DEBUGPRIORITY" );
	if( pszPrio ) {
	    fg_DebugPriority = atoi( pszPrio );
	    fprintf( stderr,
		     "fg_debug.c: Environment overrides default debug priority (%d)\n",
		     fg_DebugPriority );
	}
    /* } */


    /*
    if ((debugArgValue >= FG_ALL) && (debugArgValue < FG_UNDEFD)) {
	fg_DebugPriority = priorityArgValue;
    } else {  // Either not set or out of range. We will not warn the user.
    */
	pszClass = getenv( "FG_DEBUGCLASS" );
	if( pszClass ) {
	    fg_DebugClass = fgDebugStrToClass( pszClass );
	    fprintf( stderr,
		     "fg_debug.c: Environment overrides default debug class (0x%08X)\n",
		     fg_DebugClass );
	}
    /* } */

    FG_RELEASEDEBUGSEM;
}

/* fgDebugStrToClass ======================================================*/
fgDebugClass fgDebugStrToClass( char *str ) {
    char *hex = "0123456789ABCDEF";
    char *hexl = "0123456789abcdef";
    char *pt, *p, *ph, ps = 1;
    unsigned int val = 0, i;
  
    if( str == NULL ) {
	return 0;
    }

    /* Check for 0xXXXXXX notation */
    p = strstr( str, "0x");
    if( p ) {
	p++; p++;
	while (*p) {
	    ph = strchr(hex,*p);
	    if ( ph ) {
 		val <<= 4;
		val += ph-hex;
		p++;
	    } else {
		ph = strchr(hexl,*p);
		if ( ph ) {
		    val <<= 4;
		    val += ph-hex;
		    p++;
		} else {
		    // fprintf( stderr, "Error in hex string '%s'\n", str ); 
		    return FG_NONE;
		}
	    }
	}
    } else {
	/* Must be in string format */
	p = str;
	ps = 1;
	while( ps ) {
	    while( *p && (*p==' ' || *p=='\t') ) p++; /* remove whitespace */
	    pt = p; /* mark token */
	    while( *p && (*p!='|') ) p++; /* find OR or EOS */
	    ps = *p; /* save value at p so we can attempt to be bounds safe */
	    *p++ = 0; /* terminate token */
	    /* determine value for token */
	    i=0; 
	    while( fg_DebugClasses[i].str && 
		   strncmp( fg_DebugClasses[i].str, pt, 
			    strlen(fg_DebugClasses[i].str)) ) i++;
	    if( fg_DebugClasses[i].str == NULL ) {
		fprintf( stderr,
			 "fg_debug.c: Could not find message class '%s'\n",
			 pt ); 
	    } else {
		val |= fg_DebugClasses[i].dbg_class;
	    }
	}
    }
    return (fgDebugClass)val;
}


/* fgSetDebugOutput =======================================================*/
void fgSetDebugOutput( FILE *out ) {
    FG_GRABDEBUGSEM;
    fflush( fg_DebugOutput );
    fg_DebugOutput = out;
    FG_RELEASEDEBUGSEM;
}


/* fgSetDebugLevels =======================================================*/
void fgSetDebugLevels( fgDebugClass dbg_class, fgDebugPriority prio ) {
    FG_GRABDEBUGSEM;
    fg_DebugClass = dbg_class;
    fg_DebugPriority = prio;
    FG_RELEASEDEBUGSEM;
}


/* fgRegisterDebugCallback ================================================*/
fgDebugCallback fgRegisterDebugCallback( fgDebugCallback callback ) {
    fgDebugCallback old;
    FG_GRABDEBUGSEM;
    old = fg_DebugCallback;
    fg_DebugCallback = callback;
    FG_RELEASEDEBUGSEM;
    return old;
}


/* fgPrintf ===============================================================*/
int fgPrintf( fgDebugClass dbg_class, fgDebugPriority prio, char *fmt, ... ) {
    char szOut[1024+1];
    va_list ap;
    int ret = 0;

    // If no action to take, then don't bother with the semaphore
    // activity Slight speed benefit.

    // printf("dbg_class = %d  fg_DebugClass = %d\n", dbg_class, fg_DebugClass);
    // printf("prio = %d  fg_DebugPriority = %d\n", prio, fg_DebugPriority);

    if( !(dbg_class & fg_DebugClass) ) {
	// Failed to match a specific debug class
	if ( prio < fg_DebugPriority ) {
	    // priority is less than requested

	    // "ret" is zero anyway. But we might think about changing
	    // it upon some error condition?
	    return ret;
	}
    }

    FG_GRABDEBUGSEM;

    /* ret = vsprintf( szOut, fmt, (&fmt+1)); (but it didn't work, thus ... */
    va_start (ap, fmt);
    ret = vsprintf( szOut, fmt, ap);
    va_end (ap);

    if( fg_DebugCallback!=NULL && fg_DebugCallback(dbg_class, prio, szOut) ) {
	FG_RELEASEDEBUGSEM;
	return ret;
    } else {
	fprintf( fg_DebugOutput, szOut );
	FG_RELEASEDEBUGSEM;
	if( prio == FG_EXIT ) {
	    exit(0);
	} else if( prio == FG_ABORT ) {
	    abort();
	}
    }
    return ret;  
}


