/* -*- Mode: C++ -*- *****************************************************
 * lowleveltime.h
 * Written by various people (I"ll look up the exact credits later)
 * Modified by Durk Talsma, July 1999 for use in FlightGear
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
 **************************************************************************/

/********************************************************************
 * This file redefines some low-level Unix-like time functions for  *
 * use with FlightGear. Most notably, localtime() is adapted to use *
 * a custom timezone, in order to get the 'local' time for a given  *
 * aircraft's position, and not only for the current location of the*
 * computer running the sim.                                        *
 *                                                                  *
 * Software adapted from glibc functions, by Durk Talsma. Started   *
 * July, 17, 1999.                                                  *
 ********************************************************************/

#ifndef _LOWLEVELTIME_H_
#define _LOWLEVELTIME_H_

#include <time.h>

/* adapted from zdump.c */
void show (const char *zone, time_t t, int v);

/* adapted from <time.h> */
struct tm * fgLocaltime (const time_t *t, char *tzName);

/* Prototype for the internal function to get information based on TZ.  */
extern struct tm *fgtz_convert (const time_t *t, int use_localtime,
				     struct tm *tp, char *tzName);

/* This structure contains all the information about a
   timezone given in the POSIX standard TZ envariable.  */
typedef struct
  {
    const char *name;

    /* When to change.  */
    enum { J0, J1, M } type;	/* Interpretation of:  */
    unsigned short int m, n, d;	/* Month, week, day.  */
    unsigned int secs;		/* Time of day.  */

    long int offset;		/* Seconds east of GMT (west if < 0).  */

    /* We cache the computed time of change for a
       given year so we don't have to recompute it.  */
    time_t change;	/* When to change to this zone.  */
    int computed_for;	/* Year above is computed for.  */
  } fgtz_rule;

/* tz_rules[0] is standard, tz_rules[1] is daylight.  */
static fgtz_rule fgtz_rules[2];

struct tzhead {
 	char	tzh_magic[4];		/* TZ_MAGIC */
	char	tzh_reserved[16];	/* reserved for future use */
	char	tzh_ttisgmtcnt[4];	/* coded number of trans. time flags */
	char	tzh_ttisstdcnt[4];	/* coded number of trans. time flags */
	char	tzh_leapcnt[4];		/* coded number of leap seconds */
	char	tzh_timecnt[4];		/* coded number of transition times */
	char	tzh_typecnt[4];		/* coded number of local time types */
	char	tzh_charcnt[4];		/* coded number of abbr. chars */
};


/* Defined in mktime.c.  */
extern const unsigned short int mon_yday[2][13];

#ifndef TZDIR
#define TZDIR	"/usr/local/etc/zoneinfo" /* Time zone object file directory */
#endif /* !defined TZDIR */



#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif /* !defined TZDEFAULT */

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

#endif 
