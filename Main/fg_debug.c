/**************************************************************************
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
 * (Log is kept at end of this file)
 **************************************************************************/

#include <string.h>
#include <Main/fg_debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

static int fg_DebugSem = 1;
static fgDebugClass fg_DebugClass = FG_ALL;
static fgDebugPriority fg_DebugPriority = FG_INFO;
static fgDebugCallback fg_DebugCallback = NULL;

#ifndef __CYGWIN32__
    static FILE *fg_DebugOutput = stderr;
#else /* __CYGWIN32__ */
    static FILE *fg_DebugOutput = NULL;
#endif /* __CYGWIN32 */

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
  { "FG_NONE",    0x00000000 },
  { "FG_TERRAIN", 0x00000001 },
  { "FG_ASTRO",   0x00000002 },
  { "FG_FLIGHT",  0x00000004 },
  { "FG_INPUT",   0x00000008 },
  { "FG_GL",      0x00000010 },
  { "FG_VIEW",    0x00000020 },
  { "FG_COCKPIT", 0x00000040 },
  { "FG_GENERAL", 0x00000080 },
  { "FG_MATH",    0x00000100 },
  { "FG_EVENT",   0x00000200 },
  { "FG_AIRCRAFT",0x00000400 },

  /* Do not edit below here, last entry should be null */
  { "FG_ALL",     0xFFFFFFFF },
  { NULL, 0 } };

static fgDebugClass fgDebugStrToClass( char *str );


/* fgInitDebug =============================================================*/
void fgInitDebug( void )
{
  char *pszClass, *pszPrio;

#ifdef __CYGWIN32__
    fg_DebugOutput = stderr;
#endif /* __CYGWIN32 */

  FG_GRABDEBUGSEM;
  fg_DebugSem=fg_DebugSem;  /* shut up GCC */

  pszPrio = getenv( "FG_DEBUGPRIORITY" );
  if( pszPrio ) {
    fg_DebugPriority = atoi( pszPrio );
    fprintf( stderr, "fg_debug.c: Environment overrides default debug priority (%d)\n",
	     fg_DebugPriority );
  }

  pszClass = getenv( "FG_DEBUGCLASS" );
  if( pszClass ) {
    fg_DebugClass = fgDebugStrToClass( pszClass );
    fprintf( stderr, "fg_debug.c: Environment overrides default debug class (0x%08X)\n",
	     fg_DebugClass );
  }

  FG_RELEASEDEBUGSEM;
}

/* fgDebugStrToClass ======================================================*/
fgDebugClass fgDebugStrToClass( char *str )
{
  char *hex = "0123456789ABCDEF";
  char *hexl = "0123456789abcdef";
  char *pt, *p, *ph, ps=1;
  unsigned int val = 0, i;
  
  if( str == NULL ) {
    return 0;
  }

  /* Check for 0xXXXXXX notation */
  if( (p = strstr( str, "0x")) ) {
    p++; p++;
    while (*p) {
      if( (ph = strchr(hex,*p)) || (ph = strchr(hexl,*p)) ){
	val <<= 4;
	val += ph-hex;
	p++;
      }
      else {
	/* fprintf( stderr, "Error in hex string '%s'\n", str ); 
	 */
	return FG_NONE;
      }
    }
  }
  else {
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
	     strncmp( fg_DebugClasses[i].str, pt, strlen(fg_DebugClasses[i].str)) ) i++;
      if( fg_DebugClasses[i].str == NULL ) {
	fprintf( stderr, "fg_debug.c: Could not find message class '%s'\n", pt ); 
      } else {
	val |= fg_DebugClasses[i].dbg_class;
      }
    }
  }
  return (fgDebugClass)val;
}


/* fgSetDebugOutput =======================================================*/
void fgSetDebugOutput( FILE *out )
{
  FG_GRABDEBUGSEM;
  fflush( fg_DebugOutput );
  fg_DebugOutput = out;
  FG_RELEASEDEBUGSEM;
}


/* fgSetDebugLevels =======================================================*/
void fgSetDebugLevels( fgDebugClass dbg_class, fgDebugPriority prio )
{
  FG_GRABDEBUGSEM;
  fg_DebugClass = dbg_class;
  fg_DebugPriority = prio;
  FG_RELEASEDEBUGSEM;
}


/* fgRegisterDebugCallback ================================================*/
fgDebugCallback fgRegisterDebugCallback( fgDebugCallback callback )
{
  fgDebugCallback old;
  FG_GRABDEBUGSEM;
  old = fg_DebugCallback;
  fg_DebugCallback = callback;
  FG_RELEASEDEBUGSEM;
  return old;
}


/* fgPrintf ===============================================================*/
int fgPrintf( fgDebugClass dbg_class, fgDebugPriority prio, char *fmt, ... )
{
  char szOut[1024+1];
  va_list ap;
  int ret = 0;

  FG_GRABDEBUGSEM;

  if( !(dbg_class & fg_DebugClass) || (prio < fg_DebugPriority) ) {
    FG_RELEASEDEBUGSEM;
    return 0;
  } 

  /* ret = vsprintf( szOut, fmt, (&fmt+1)); (but it didn't work, thus ... */
  va_start (ap, fmt);
  ret = vsprintf( szOut, fmt, ap);
  va_end (ap);

  if( fg_DebugCallback!=NULL && fg_DebugCallback(dbg_class, prio, szOut) ) {
    FG_RELEASEDEBUGSEM;
    return ret;
  } 
  else {
    fprintf( fg_DebugOutput, szOut );
    FG_RELEASEDEBUGSEM;
    if( prio == FG_EXIT ) {
      exit(0);
    } 
    else if( prio == FG_ABORT ) {
      abort();
    }
  }
  return ret;  
} 

