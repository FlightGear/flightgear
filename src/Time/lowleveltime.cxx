/* -*- Mode: C++ -*- *****************************************************
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


#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
//#include <libc-lock.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "lowleveltime.h"


/* BIG FAT WARNING: NOTICE THAT I HARDCODED ENDIANNES. PLEASE CHANGE THIS */
#ifndef BIG_ENDIAN 
#define BIG_ENDIAN 4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif

#ifndef BYTE_ORDER 
#define BYTE_ORDER LITTLE_ENDIAN
#endif


#ifndef BYTE_ORDER
#define BYTE_ORDER 
#endif

/* END OF BIG FAT WARNING */

//#include "tzfile.h"

#define	min(a, b)	((a) < (b) ? (a) : (b))
#define	max(a, b)	((a) > (b) ? (a) : (b))
#define	sign(x)		((x) < 0 ? -1 : 1)

struct leap
  {
    time_t transition;		/* Time the transition takes effect.  */
    long int change;		/* Seconds of correction to apply.  */
  };

/* Header for a list of buffers containing time zone strings.  */
struct tzstring_head
{
  struct tzstring_head *next;
  /* The buffer itself immediately follows the header.
     The buffer contains zero or more (possibly overlapping) strings.
     The last string is followed by 2 '\0's instead of the usual 1.  */
};


/* First in a list of buffers containing time zone strings.
   All the buffers but the last are read-only.  */
static struct
{
  struct tzstring_head head;
  char data[48];
} tzstring_list;

/* Size of the last buffer in the list, not counting its header.  */
static size_t tzstring_last_buffer_size = sizeof tzstring_list.data;


static char *old_fgtz = NULL;
static int use_fgtzfile = 1;
static int fgdaylight;
static char* fgtzname[2];
static long int fgtimezone;


static size_t num_transitions;
static time_t *transitions = NULL;
static unsigned char *type_idxs = NULL;
static size_t num_types;
static struct ttinfo *types = NULL;
static char *zone_names = NULL;
static size_t num_leaps;
static struct leap *leaps = NULL;


static void fgtzset_internal (int always, const char *tz);
static int fgtz_compute(time_t timer, const struct tm *tm);
static int fgcompute_change(fgtz_rule *rule, int year);
static struct ttinfo *fgfind_transition (time_t timer);
static void fgcompute_tzname_max (size_t chars);
static inline int decode (const void *ptr);
void fgtzfile_read (const char *file);
static void offtime (const time_t *t, long int offset, struct tm *tp);
static char *tzstring (const char* string);

/* tz_rules[0] is standard, tz_rules[1] is daylight.  */
static fgtz_rule fgtz_rules[2];

int fgtzfile_compute (time_t timer, int use_localtime,
		  long int *leap_correct, int *leap_hit);
struct ttinfo
  {
    long int offset;		/* Seconds east of GMT.  */
    unsigned char isdst;	/* Used to set tm_isdst.  */
    unsigned char idx;		/* Index into `zone_names'.  */
    unsigned char isstd;	/* Transition times are in standard time.  */
    unsigned char isgmt;	/* Transition times are in GMT.  */
  };



/* How many days come before each month (0-12).  */
const unsigned short int mon_yday[2][13] =
  {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
  };


/* The C Standard says that localtime and gmtime return the same pointer.  */
struct tm _fgtmbuf;


#ifndef isleap
/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
# define isleap(year) \
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif




/* Return the `struct tm' representation of *T in local time.  */
struct tm * fgLocaltime (const time_t *t, const char *tzName)
{
  return fgtz_convert (t, 1, &_fgtmbuf, tzName);
}


/* Return the `struct tm' representation of *TIMER in the local timezone.
   Use local time if USE_LOCALTIME is nonzero, UTC otherwise.  */
struct tm * fgtz_convert (const time_t *timer, int use_localtime, struct tm *tp, const char *tzName)
{
  long int leap_correction;
  long int offsetCorr;                 // ADDED TO RESOLVE NON-ANSI FIELDS IN struct tm
  int leap_extra_secs;

  if (timer == NULL)
    {
      //set_errno (EINVAL);
      return NULL;
    }

  //libc_lock_lock (tzset_lock);

  /* Update internal database according to current TZ setting.
     POSIX.1 8.3.7.2 says that localtime_r is not required to set tzname.
     This is a good idea since this allows at least a bit more parallelism.
     By analogy we apply the same rule to gmtime_r.  */
  fgtzset_internal (tp == &_fgtmbuf, tzName);

  if (use_fgtzfile)
    {
      if (! fgtzfile_compute (*timer, use_localtime,
			      &leap_correction, &leap_extra_secs))
	tp = NULL;
    }
  else
    {
      offtime (timer, 0, tp);
      if (! fgtz_compute (*timer, tp))
	tp = NULL;
      leap_correction = 0L;
      leap_extra_secs = 0;
    }

  if (tp)
    {
      if (use_localtime)
	{
	  tp->tm_isdst = fgdaylight;
	  //tp->tm_zone = fgtzname[fgdaylight];       // NON_ANSI
	  //tp->tm_gmtoff = -fgtimezone;              // NON_ANSI
	  offsetCorr = -fgtimezone;
	}
      else
	{
	  tp->tm_isdst = 0;
	  //tp->tm_zone = "GMT";                     // NON_ANSI
	  //tp->tm_gmtoff = 0L;                      // NON_ANSI
	  offsetCorr = -fgtimezone;
	}

      //offtime (timer, tp->tm_gmtoff - leap_correction, tp);
      offtime (timer, offsetCorr - leap_correction, tp);
      tp->tm_sec += leap_extra_secs;
    }

  //libc_lock_unlock (tzset_lock);

  return tp;
}



/* the following stuff is adapted from the tzCode package */

static size_t	longest;
static char *	abbr (struct tm * tmp);

void show(const char *zone, time_t t, int v)
{
	struct tm *	tmp;

	(void) printf("%-*s  ", (int) longest, zone);
	if (v)
		(void) printf("%.24s UTC = ", asctime(gmtime(&t)));
	tmp = fgLocaltime(&t, zone);
	(void) printf("%.24s", asctime(tmp));
	if (*abbr(tmp) != '\0')
		(void) printf(" %s", abbr(tmp));
	if (v) {
		(void) printf(" isdst=%d", tmp->tm_isdst);
#ifdef TM_GMTOFF
		(void) printf(" gmtoff=%ld", tmp->TM_GMTOFF);
#endif /* defined TM_GMTOFF */
	}
	(void) printf("\n");
}

static char *abbr(struct tm *tmp)
{
	register char *	result;
	static char	nada;

	if (tmp->tm_isdst != 0 && tmp->tm_isdst != 1)
		return &nada;
	result = fgtzname[tmp->tm_isdst];
	return (result == NULL) ? &nada : result;
}



/***********************************************************************/


/* Interpret the TZ envariable.  */
static void fgtzset_internal (int always, const char *tz)
{
  time_t now;
  time(&now);
  static int is_initialized = 0;
  //register const char *tz;
  register size_t l;
  char *tzbuf;
  unsigned short int hh, mm, ss;
  unsigned short int whichrule;

  if (is_initialized && !always)
    return;
  is_initialized = 1;

  /* Examine the TZ environment variable.  */
  //tz = getenv ("TZ");
  if (tz == NULL)
    /* No user specification; use the site-wide default.  */
    tz = TZDEFAULT;
  else if (*tz == '\0')
    /* User specified the empty string; use UTC explicitly.  */
    tz = "Universal";

#ifdef MACOS
  /* as you well know, mac paths contain leading colon, this code
     messes things up.... */
#else
  /* A leading colon means "implementation defined syntax".
     We ignore the colon and always use the same algorithm:
     try a data file, and if none exists parse the 1003.1 syntax.  */
  if (tz && *tz == ':')
    ++tz;
#endif

  /* Check whether the value changes since the last run.  */
  if (old_fgtz != NULL && tz != NULL && strcmp (tz, old_fgtz) == 0)
    /* No change, simply return.  */
    return;

  fgtz_rules[0].name = NULL;
  fgtz_rules[1].name = NULL;

  /* Save the value of `tz'.  */
  if (old_fgtz != NULL)
    free (old_fgtz);
  old_fgtz = tz ? strdup (tz) : NULL;

  /* Try to read a data file.  */
  fgtzfile_read (tz);
  if (use_fgtzfile)
    return;
  // The default behaviour of the originale tzset_internal (int always, char* tz) 
  // function is to set up a default timezone, in any casetz file_read() fails
  // Currently this leads to problems, because it modidifies the system timezone
  // and not the local aircraft timezone, contained in FlightGear. I could adapt 
  // this in future versions of this code, but doubt whether this is what we really
  // want. So right now, exit when timezone information reading failed. 
  // Guess I'll change that to something like 12 * (FG_LON / 180.0)
  // 
  // For now, I'll leave it like this.
  else
  {
    printf ("Timezone reading failed\n");
    exit(1);
  }
  // this emacs "comment out" function is cool!
 
//   // /* No data file found.  Default to UTC if nothing specified.  */
// //   printf ("1. Current local time          = %24s", asctime(localtime(&now)));
//   if (tz == NULL || *tz == '\0')
//     {
//       fgtz_rules[0].name = fgtz_rules[1].name = "UTC";
//       fgtz_rules[0].type = fgtz_rules[1].type = fgtz_rule::J0;
//       fgtz_rules[0].m = fgtz_rules[0].n = fgtz_rules[0].d = 0;
//       fgtz_rules[1].m = fgtz_rules[1].n = fgtz_rules[1].d = 0;
//       fgtz_rules[0].secs = fgtz_rules[1].secs = 0;
//       fgtz_rules[0].offset = fgtz_rules[1].offset = 0L;
//       fgtz_rules[0].change = fgtz_rules[1].change = (time_t) -1;
//       fgtz_rules[0].computed_for = fgtz_rules[1].computed_for = 0;
//       return;
//     }

//   /* Clear out old state and reset to unnamed UTC.  */
//   //printf ("2. Current local time          = %24s", asctime(localtime(&now)));
//   memset (fgtz_rules, 0, sizeof fgtz_rules);
//   fgtz_rules[0].name = fgtz_rules[1].name = "";

//   /* Get the standard timezone name.  */
//   tzbuf = malloc (strlen (tz) + 1);
//   if (! tzbuf)
//     {
//       /* Clear the old tz name so we will try again.  */
//       free (old_fgtz);
//       old_fgtz = NULL;
//       return;
//     }
//   //printf ("3. Current local time          = %24s", asctime(localtime(&now)));
//   if (sscanf (tz, "%[^0-9,+-]", tzbuf) != 1 ||
//       (l = strlen (tzbuf)) < 3)
//     {
//       free (tzbuf);
//       return;
//     }

//   fgtz_rules[0].name = tzstring (tzbuf);

//   tz += l;
//   //printf ("4. Current local time          = %24s", asctime(localtime(&now)));
//   /* Figure out the standard offset from UTC.  */
//   if (*tz == '\0' || (*tz != '+' && *tz != '-' && !isdigit (*tz)))
//     {
//       free (tzbuf);
//       return;
//     }
//   //printf ("5. Current local time          = %24s", asctime(localtime(&now)));
//   if (*tz == '-' || *tz == '+')
//     fgtz_rules[0].offset = *tz++ == '-' ? 1L : -1L;
//   else
//     fgtz_rules[0].offset = -1L;
//   switch (sscanf (tz, "%hu:%hu:%hu", &hh, &mm, &ss))
//     {
//     default:
//       free (tzbuf);
//       return;
//     case 1:
//       mm = 0;
//     case 2:
//       ss = 0;
//     case 3:
//       break;
//     }
//     //printf ("6. Current local time          = %24s", asctime(localtime(&now)));
//   fgtz_rules[0].offset *= (min (ss, 59) + (min (mm, 59) * 60) +
// 			 (min (hh, 23) * 60 * 60));

//   for (l = 0; l < 3; ++l)
//     {
//       while (isdigit(*tz))
// 	++tz;
//       if (l < 2 && *tz == ':')
// 	++tz;
//     }
//   //printf ("7. Current local time          = %24s", asctime(localtime(&now)));
//   /* Get the DST timezone name (if any).  */
//   if (*tz != '\0')
//     {
//       char *n = tzbuf + strlen (tzbuf) + 1;
//       if (sscanf (tz, "%[^0-9,+-]", n) != 1 ||
// 	  (l = strlen (n)) < 3)
// 	goto done_names;	/* Punt on name, set up the offsets.  */
//   //printf ("7.1 Current local time          = %24s", asctime(localtime(&now)));
//       fgtz_rules[1].name = tzstring (n);

//       tz += l;

//       /* Figure out the DST offset from GMT.  */
//       if (*tz == '-' || *tz == '+')
// 	fgtz_rules[1].offset = *tz++ == '-' ? 1L : -1L;
//       else
// 	fgtz_rules[1].offset = -1L;
//   //printf ("7.2 Current local time          = %24s", asctime(localtime(&now)));
//       switch (sscanf (tz, "%hu:%hu:%hu", &hh, &mm, &ss))
// 	{
// 	default:
// 	  /* Default to one hour later than standard time.  */
// 	  fgtz_rules[1].offset = fgtz_rules[0].offset + (60 * 60);
// 	  break;

// 	case 1:
// 	  mm = 0;
// 	case 2:
// 	  ss = 0;
// 	case 3:
// 	  fgtz_rules[1].offset *= (min (ss, 59) + (min (mm, 59) * 60) +
// 				 (min (hh, 23) * (60 * 60)));
// 	  break;
// 	}
//   //printf ("7.3 Current local time          = %24s", asctime(localtime(&now)));
//       for (l = 0; l < 3; ++l)
// 	{
// 	  while (isdigit (*tz))
// 	    ++tz;
// 	  if (l < 2 && *tz == ':')
// 	    ++tz;
// 	}
//   //printf ("7.4 Current local time          = %24s", asctime(localtime(&now)));
//       if (*tz == '\0' || (tz[0] == ',' && tz[1] == '\0'))
// 	{
// 	  /* There is no rule.  See if there is a default rule file.  */
//   //printf ("7.4.1 Current local time          = %24s", asctime(localtime(&now)));
// 	  tzfile_default (fgtz_rules[0].name, fgtz_rules[1].name,
// 			    fgtz_rules[0].offset, fgtz_rules[1].offset);
//   //printf ("7.4.2 Current local time          = %24s", asctime(localtime(&now)));
// 	  if (use_fgtzfile)
// 	    {
// 	      free (old_fgtz);
// 	      old_fgtz = NULL;
// 	      free (tzbuf);
// 	      return;
// 	    }
// 	}
//     }
//   else
//     {
//       /* There is no DST.  */
//       fgtz_rules[1].name = fgtz_rules[0].name;
//       free (tzbuf);
//       return;
//     }
//   //printf ("7.5 Current local time          = %24s", asctime(localtime(&now)));
//  done_names:
//   //printf ("8. Current local time          = %24s", asctime(localtime(&now)));
//   free (tzbuf);

//   /* Figure out the standard <-> DST rules.  */
//   for (whichrule = 0; whichrule < 2; ++whichrule)
//     {
//       register fgtz_rule *tzr = &fgtz_rules[whichrule];

//       /* Ignore comma to support string following the incorrect
// 	 specification in early POSIX.1 printings.  */
//       tz += *tz == ',';

//       /* Get the date of the change.  */
//       if (*tz == 'J' || isdigit (*tz))
// 	{
// 	  char *end;
// 	  tzr->type = *tz == 'J' ? fgtz_rule::J1 : fgtz_rule::J0;
// 	  if (tzr->type == fgtz_rule::J1 && !isdigit (*++tz))
// 	    return;
// 	  tzr->d = (unsigned short int) strtoul (tz, &end, 10);
// 	  if (end == tz || tzr->d > 365)
// 	    return;
// 	  else if (tzr->type == fgtz_rule::J1 && tzr->d == 0)
// 	    return;
// 	  tz = end;
// 	}
//       else if (*tz == 'M')
// 	{
// 	  int n;
// 	  tzr->type = fgtz_rule::M;
// 	  if (sscanf (tz, "M%hu.%hu.%hu%n",
// 		      &tzr->m, &tzr->n, &tzr->d, &n) != 3 ||
// 	      tzr->m < 1 || tzr->m > 12 ||
// 	      tzr->n < 1 || tzr->n > 5 || tzr->d > 6)
// 	    return;
// 	  tz += n;
// 	}
//       else if (*tz == '\0')
// 	{
// 	  /* United States Federal Law, the equivalent of "M4.1.0,M10.5.0".  */
// 	  tzr->type = fgtz_rule::M;
// 	  if (tzr == &fgtz_rules[0])
// 	    {
// 	      tzr->m = 4;
// 	      tzr->n = 1;
// 	      tzr->d = 0;
// 	    }
// 	  else
// 	    {
// 	      tzr->m = 10;
// 	      tzr->n = 5;
// 	      tzr->d = 0;
// 	    }
// 	}
//       else
// 	return;
//       //printf ("9. Current local time          = %24s", asctime(localtime(&now)));
//       if (*tz != '\0' && *tz != '/' && *tz != ',')
// 	return;
//       else if (*tz == '/')
// 	{
// 	  /* Get the time of day of the change.  */
// 	  ++tz;
// 	  if (*tz == '\0')
// 	    return;
// 	  switch (sscanf (tz, "%hu:%hu:%hu", &hh, &mm, &ss))
// 	    {
// 	    default:
// 	      hh = 2;		/* Default to 2:00 AM.  */
// 	    case 1:
// 	      mm = 0;
// 	    case 2:
// 	      ss = 0;
// 	    case 3:
// 	      break;
// 	    }
// 	  for (l = 0; l < 3; ++l)
// 	    {
// 	      while (isdigit (*tz))
// 		++tz;
// 	      if (l < 2 && *tz == ':')
// 		++tz;
// 	    }
// 	  tzr->secs = (hh * 60 * 60) + (mm * 60) + ss;
// 	}
//       else
// 	/* Default to 2:00 AM.  */
// 	tzr->secs = 2 * 60 * 60;

//       tzr->computed_for = -1;
//     }
// //   printf ("10. Current local time          = %24s", asctime(localtime(&now)));
// 
}

/************************************************************************/


/* Figure out the correct timezone for *TIMER and TM (which must be the same)
   and set `tzname', `timezone', and `daylight' accordingly.
   Return nonzero on success, zero on failure.  */
size_t fgtzname_cur_max;

static int fgtz_compute (time_t timer, const struct tm* tm)
  //     time_t timer;
  // const struct tm *tm;
{
  if (! fgcompute_change (&fgtz_rules[0], 1900 + tm->tm_year) ||
      ! fgcompute_change (&fgtz_rules[1], 1900 + tm->tm_year))
    return 0;

  fgdaylight = timer >= fgtz_rules[0].change && timer < fgtz_rules[1].change;
  fgtimezone = -fgtz_rules[fgdaylight].offset;
  fgtzname[0] = (char *) fgtz_rules[0].name;
  fgtzname[1] = (char *) fgtz_rules[1].name;

  {
    /* Keep tzname_cur_max up to date.  */
    size_t len0 = strlen (fgtzname[0]);
    size_t len1 = strlen (fgtzname[1]);
    if (len0 > fgtzname_cur_max)
      fgtzname_cur_max = len0;
    if (len1 > fgtzname_cur_max)
      fgtzname_cur_max = len1;
  }

  return 1;
}

/**********************************************************************/

/* Figure out the exact time (as a time_t) in YEAR
   when the change described by RULE will occur and
   put it in RULE->change, saving YEAR in RULE->computed_for.
   Return nonzero if successful, zero on failure.  */
static int fgcompute_change (fgtz_rule *rule, int year)
  //     tz_rule *rule;
  // int year;
{
  register time_t t;
  int y;

  if (year != -1 && rule->computed_for == year)
    /* Operations on times in 1969 will be slower.  Oh well.  */
    return 1;

  /* First set T to January 1st, 0:00:00 GMT in YEAR.  */
  t = 0;
  for (y = 1970; y < year; ++y)
    t += SECSPERDAY * (isleap (y) ? 366 : 365);

  switch (rule->type)
    {
    case fgtz_rule::J1:
      /* Jn - Julian day, 1 == January 1, 60 == March 1 even in leap years.
	 In non-leap years, or if the day number is 59 or less, just
	 add SECSPERDAY times the day number-1 to the time of
	 January 1, midnight, to get the day.  */
      t += (rule->d - 1) * SECSPERDAY;
      if (rule->d >= 60 && isleap (year))
	t += SECSPERDAY;
      break;

    case fgtz_rule::J0:
      /* n - Day of year.
	 Just add SECSPERDAY times the day number to the time of Jan 1st.  */
      t += rule->d * SECSPERDAY;
      break;

    case fgtz_rule::M:
      /* Mm.n.d - Nth "Dth day" of month M.  */
      {
	register int i, d, m1, yy0, yy1, yy2, dow;
	register const unsigned short int *myday =
	  &mon_yday[isleap (year)][rule->m];

	/* First add SECSPERDAY for each day in months before M.  */
	t += myday[-1] * SECSPERDAY;

	/* Use Zeller's Congruence to get day-of-week of first day of month. */
	m1 = (rule->m + 9) % 12 + 1;
	yy0 = (rule->m <= 2) ? (year - 1) : year;
	yy1 = yy0 / 100;
	yy2 = yy0 % 100;
	dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
	if (dow < 0)
	  dow += 7;

	/* DOW is the day-of-week of the first day of the month.  Get the
	   day-of-month (zero-origin) of the first DOW day of the month.  */
	d = rule->d - dow;
	if (d < 0)
	  d += 7;
	for (i = 1; i < rule->n; ++i)
	  {
	    if (d + 7 >= myday[0] - myday[-1])
	      break;
	    d += 7;
	  }

	/* D is the day-of-month (zero-origin) of the day we want.  */
	t += d * SECSPERDAY;
      }
      break;
    }

  /* T is now the Epoch-relative time of 0:00:00 GMT on the day we want.
     Just add the time of day and local offset from GMT, and we're done.  */

  rule->change = t - rule->offset + rule->secs;
  rule->computed_for = year;
  return 1;
}

/*************************************************************************/

int fgtzfile_compute (time_t timer, int use_localtime,
		  long int *leap_correct, int *leap_hit)
{
  register size_t i;

  if (use_localtime)
    {
      struct ttinfo *info = fgfind_transition (timer);
      fgdaylight = info->isdst;
      fgtimezone = -info->offset;
      for (i = 0;
	   i < num_types && i < sizeof (fgtzname) / sizeof (fgtzname[0]);
	   ++i)
	fgtzname[types[i].isdst] = &zone_names[types[i].idx];
      if (info->isdst < sizeof (fgtzname) / sizeof (fgtzname[0]))
	fgtzname[info->isdst] = &zone_names[info->idx];
    }

  *leap_correct = 0L;
  *leap_hit = 0;

  /* Find the last leap second correction transition time before TIMER.  */
  i = num_leaps;
  do
    if (i-- == 0)
      return 1;
  while (timer < leaps[i].transition);

  /* Apply its correction.  */
  *leap_correct = leaps[i].change;

  if (timer == leaps[i].transition && /* Exactly at the transition time.  */
      ((i == 0 && leaps[i].change > 0) ||
       leaps[i].change > leaps[i - 1].change))
    {
      *leap_hit = 1;
      while (i > 0 &&
	     leaps[i].transition == leaps[i - 1].transition + 1 &&
	     leaps[i].change == leaps[i - 1].change + 1)
	{
	  ++*leap_hit;
	  --i;
	}
    }

  return 1;
}

/**************************************************************************/

static struct ttinfo * fgfind_transition (time_t timer)
{
  size_t i;

  if (num_transitions == 0 || timer < transitions[0])
    {
      /* TIMER is before any transition (or there are no transitions).
	 Choose the first non-DST type
	 (or the first if they're all DST types).  */
      i = 0;
      while (i < num_types && types[i].isdst)
	++i;
      if (i == num_types)
	i = 0;
    }
  else
    {
      /* Find the first transition after TIMER, and
	 then pick the type of the transition before it.  */
      for (i = 1; i < num_transitions; ++i)
	if (timer < transitions[i])
	  break;
      i = type_idxs[i - 1];
    }

  return &types[i];
}


/**************************************************************************/
void fgtzfile_read (const char *file)
{
  static const char default_tzdir[] = TZDIR;
  size_t num_isstd, num_isgmt;
  register FILE *f;
  struct tzhead tzhead;
  size_t chars;
  register size_t i;
  struct ttinfo *info;

  use_fgtzfile = 0;

  if (transitions != NULL)
    free ((void *) transitions);
  transitions = NULL;
  if (type_idxs != NULL)
    free ((void *) type_idxs);
  type_idxs = NULL;
  if (types != NULL)
    free ((void *) types);
  types = NULL;
  if (zone_names != NULL)
    free ((void *) zone_names);
  zone_names = NULL;
  if (leaps != NULL)
    free ((void *) leaps);
  leaps = NULL;

  if (file == NULL)
    /* No user specification; use the site-wide default.  */
    file = TZDEFAULT;
  else if (*file == '\0')
    /* User specified the empty string; use UTC with no leap seconds.  */
    return;
  else
    {
      /* We must not allow to read an arbitrary file in a setuid
	 program.  So we fail for any file which is not in the
	 directory hierachy starting at TZDIR
	 and which is not the system wide default TZDEFAULT.  */
      //if (libc_enable_secure
      //  && ((*file == '/'
      //       && memcmp (file, TZDEFAULT, sizeof TZDEFAULT)
      //       && memcmp (file, default_tzdir, sizeof (default_tzdir) - 1))
      //      || strstr (file, "../") != NULL))
	/* This test is certainly a bit too restrictive but it should
	   catch all critical cases.  */
      //return;
    }

//   if (*file != '/') // if a relative path is used, append what file points to
//                     // to the path indicated by TZDIR.
//     {
//       const char *tzdir;
//       unsigned int len, tzdir_len;
//       char *_new;

//       tzdir = getenv ("TZDIR");
//       if (tzdir == NULL || *tzdir == '\0')
// 	{
// 	  tzdir = default_tzdir;
// 	  tzdir_len = sizeof (default_tzdir) - 1;
// 	}
//       else
// 	tzdir_len = strlen (tzdir);
//       len = strlen (file) + 1;
//       _new = (char *) alloca (tzdir_len + 1 + len);
//       memcpy (_new, tzdir, tzdir_len);
//       _new[tzdir_len] = '/';
//       memcpy (&_new[tzdir_len + 1], file, len);
//       file = _new;
//     }

  f = fopen (file, "rb");
  if (f == NULL)
    return;

  if (fread ((void *) &tzhead, sizeof (tzhead), 1, f) != 1)
    goto lose;

  num_transitions = (size_t) decode (tzhead.tzh_timecnt);
  num_types = (size_t) decode (tzhead.tzh_typecnt);
  chars = (size_t) decode (tzhead.tzh_charcnt);
  num_leaps = (size_t) decode (tzhead.tzh_leapcnt);
  num_isstd = (size_t) decode (tzhead.tzh_ttisstdcnt);
  num_isgmt = (size_t) decode (tzhead.tzh_ttisgmtcnt);

  if (num_transitions > 0)
    {
      transitions = (time_t *) malloc (num_transitions * sizeof(time_t));
      if (transitions == NULL)
	goto lose;
      type_idxs = (unsigned char *) malloc (num_transitions);
      if (type_idxs == NULL)
	goto lose;
    }
  if (num_types > 0)
    {
      types = (struct ttinfo *) malloc (num_types * sizeof (struct ttinfo));
      if (types == NULL)
	goto lose;
    }
  if (chars > 0)
    {
      zone_names = (char *) malloc (chars);
      if (zone_names == NULL)
	goto lose;
    }
  if (num_leaps > 0)
    {
      leaps = (struct leap *) malloc (num_leaps * sizeof (struct leap));
      if (leaps == NULL)
	goto lose;
    }

  if (sizeof (time_t) < 4)
      abort ();

  if (fread(transitions, 4, num_transitions, f) != num_transitions ||
      fread(type_idxs, 1, num_transitions, f) != num_transitions)
    goto lose;

  /* Check for bogus indices in the data file, so we can hereafter
     safely use type_idxs[T] as indices into `types' and never crash.  */
  for (i = 0; i < num_transitions; ++i)
    if (type_idxs[i] >= num_types)
      goto lose;

  if (BYTE_ORDER != BIG_ENDIAN || sizeof (time_t) != 4)
    {
      /* Decode the transition times, stored as 4-byte integers in
	 network (big-endian) byte order.  We work from the end of
	 the array so as not to clobber the next element to be
	 processed when sizeof (time_t) > 4.  */
      i = num_transitions;
      while (i-- > 0)
	transitions[i] = decode ((char *) transitions + i*4);
    }

  for (i = 0; i < num_types; ++i)
    {
      unsigned char x[4];
      if (fread (x, 1, 4, f) != 4 ||
	  fread (&types[i].isdst, 1, 1, f) != 1 ||
	  fread (&types[i].idx, 1, 1, f) != 1)
	goto lose;
      if (types[i].idx >= chars) /* Bogus index in data file.  */
	goto lose;
      types[i].offset = (long int) decode (x);
    }

  if (fread (zone_names, 1, chars, f) != chars)
    goto lose;

  for (i = 0; i < num_leaps; ++i)
    {
      unsigned char x[4];
      if (fread (x, 1, sizeof (x), f) != sizeof (x))
	goto lose;
      leaps[i].transition = (time_t) decode (x);
      if (fread (x, 1, sizeof (x), f) != sizeof (x))
	goto lose;
      leaps[i].change = (long int) decode (x);
    }

  for (i = 0; i < num_isstd; ++i)
    {
      int c = getc (f);
      if (c == EOF)
	goto lose;
      types[i].isstd = c != 0;
    }
  while (i < num_types)
    types[i++].isstd = 0;

  for (i = 0; i < num_isgmt; ++i)
    {
      int c = getc (f);
      if (c == EOF)
	goto lose;
      types[i].isgmt = c != 0;
    }
  while (i < num_types)
    types[i++].isgmt = 0;

  fclose (f);

  info = fgfind_transition (0);
  for (i = 0; i < num_types && i < sizeof (fgtzname) / sizeof (fgtzname[0]);
       ++i)
    fgtzname[types[i].isdst] = tzstring (&zone_names[types[i].idx]);
  if (info->isdst < sizeof (fgtzname) / sizeof (fgtzname[0]))
    fgtzname[info->isdst] = tzstring (&zone_names[info->idx]);

  fgcompute_tzname_max (chars);

  use_fgtzfile = 1;
  return;

 lose:;
  fclose(f);
}

/****************************************************************************/
static void fgcompute_tzname_max (size_t chars)
{
  extern size_t tzname_cur_max; /* Defined in tzset.c. */

  const char *p;

  p = zone_names;
  do
    {
      const char *start = p;
      while (*p != '\0')
	++p;
      if ((size_t) (p - start) > fgtzname_cur_max)
	fgtzname_cur_max = p - start;
    } while (++p < &zone_names[chars]);
}

/**************************************************************************/

//#include <endian.h>

/* Decode the four bytes at PTR as a signed integer in network byte order.  */
static inline int decode (const void *ptr)
{
  if ((BYTE_ORDER == BIG_ENDIAN) && sizeof (int) == 4)
    return *(const int *) ptr;
  else
    {
      const unsigned char *p = (unsigned char *)ptr;
      int result = *p & (1 << (CHAR_BIT - 1)) ? ~0 : 0;

      result = (result << 8) | *p++;
      result = (result << 8) | *p++;
      result = (result << 8) | *p++;
      result = (result << 8) | *p++;

      return result;
    }
}



#define	SECS_PER_HOUR	(60 * 60)
#define	SECS_PER_DAY	(SECS_PER_HOUR * 24)

/* Compute the `struct tm' representation of *T,
   offset OFFSET seconds east of UTC,
   and store year, yday, mon, mday, wday, hour, min, sec into *TP.  */

void offtime (const time_t *t, long int offset, struct tm *tp)
  //    const time_t *t;
  // long int offset;
  // struct tm *tp;
{
  register long int days, rem, y;
  register const unsigned short int *ip;

  days = *t / SECS_PER_DAY;
  rem = *t % SECS_PER_DAY;
  rem += offset;
  while (rem < 0)
    {
      rem += SECS_PER_DAY;
      --days;
    }
  while (rem >= SECS_PER_DAY)
    {
      rem -= SECS_PER_DAY;
      ++days;
    }
  tp->tm_hour = rem / SECS_PER_HOUR;
  rem %= SECS_PER_HOUR;
  tp->tm_min = rem / 60;
  tp->tm_sec = rem % 60;
  /* January 1, 1970 was a Thursday.  */
  tp->tm_wday = (4 + days) % 7;
  if (tp->tm_wday < 0)
    tp->tm_wday += 7;
  y = 1970;

#define LEAPS_THRU_END_OF(y) ((y) / 4 - (y) / 100 + (y) / 400)

  while (days < 0 || days >= (isleap (y) ? 366 : 365))
    {
      /* Guess a corrected year, assuming 365 days per year.  */
      long int yg = y + days / 365 - (days % 365 < 0);

      /* Adjust DAYS and Y to match the guessed year.  */
      days -= ((yg - y) * 365
	       + LEAPS_THRU_END_OF (yg - 1)
	       - LEAPS_THRU_END_OF (y - 1));
      y = yg;
    }
  tp->tm_year = y - 1900;
  tp->tm_yday = days;
  ip = mon_yday[isleap(y)];
  for (y = 11; days < ip[y]; --y)
    continue;
  days -= ip[y];
  tp->tm_mon = y;
  tp->tm_mday = days + 1;
}

/* Allocate a time zone string with given contents.
   The string will never be moved or deallocated.
   However, its contents may be shared with other such strings.  */
char *tzstring (const char* string)
  //const char *string;
{
  struct tzstring_head *h = &tzstring_list.head;
  size_t needed;
  char *p;

  /* Look through time zone string list for a duplicate of this one.  */
  for (h = &tzstring_list.head;  ;  h = h->next)
    {
      for (p = (char *) (h + 1);  p[0] | p[1];  ++p)
	if (strcmp (p, string) == 0)
	  return p;
      if (! h->next)
	break;
    }

  /* No duplicate was found.  Copy to the end of this buffer if there's room;
     otherwise, append a large-enough new buffer to the list and use it.  */
  ++p;
  needed = strlen (string) + 2; /* Need 2 trailing '\0's after last string.  */

  if ((size_t) ((char *) (h + 1) + tzstring_last_buffer_size - p) < needed)
    {
      size_t buffer_size = tzstring_last_buffer_size;
      while ((buffer_size *= 2) < needed)
	continue;
      if (! (h = h->next = (struct tzstring_head *)malloc (sizeof *h + buffer_size)))
	return NULL;
      h->next = NULL;
      tzstring_last_buffer_size = buffer_size;
      p = (char *) (h + 1);
    }

  return strncpy (p, string, needed);
}
