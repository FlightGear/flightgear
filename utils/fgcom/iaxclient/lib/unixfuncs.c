/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#define _BSD_SOURCE
#include <unistd.h>
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif
#include <time.h>
#include "iaxclient_lib.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifndef NULL
#define NULL (0)
#endif

/* Unix-specific functions */

void os_init(void)
{
}

void iaxc_millisleep(long ms)
{
	struct timespec req;

	req.tv_nsec = (ms%1000)*1000*1000;
	req.tv_sec = ms/1000;

        /* yes, it can return early.  We don't care */
        nanosleep(&req,NULL);
}


/* TODO: Implement for X/MacOSX? */
int iaxci_post_event_callback(iaxc_event ev)
{
#if 0
	iaxc_event *e;
	e = malloc(sizeof(ev));
	*e = ev;

	/* XXX Test return value? */
	PostMessage(post_event_handle,post_event_id,(WPARAM) NULL, (LPARAM) e);
#endif
	return 0;
}

#ifdef MACOSX
    /* Presently, OSX allows user-level processes to request RT
     * priority.  The API is nice, but the scheduler presently ignores
     * the parameters (but the API validates that you're not asking for
     * too much).  See
     * http://lists.apple.com/archives/darwin-development/2004/Feb/msg00079.html
     */
/* include mach stuff for declaration of thread_policy stuff */
#include <mach/mach.h>

int iaxci_prioboostbegin()
{
	struct thread_time_constraint_policy ttcpolicy;
	int params [2] = {CTL_HW,HW_BUS_FREQ};
	int hzms;
	size_t sz;
	int ret;

	/* get hz */
	sz = sizeof (hzms);
	sysctl (params, 2, &hzms, &sz, NULL, 0);

	/* make hzms actually hz per ms */
	hzms /= 1000;

	/* give us at least how much? 6-8ms every 10ms (we generally need less) */
	ttcpolicy.period = 10 * hzms; /* 10 ms */
	ttcpolicy.computation = 2 * hzms;
	ttcpolicy.constraint = 3 * hzms;
	ttcpolicy.preemptible = 1;

	if ( (ret = thread_policy_set(mach_thread_self(),
			THREAD_TIME_CONSTRAINT_POLICY, (int *)&ttcpolicy,
			THREAD_TIME_CONSTRAINT_POLICY_COUNT)) != KERN_SUCCESS )
	{
		fprintf(stderr, "thread_policy_set failed: %d.\n", ret);
	}
	return 0;
}

int iaxci_prioboostend()
{
    /* TODO */
    return 0;
}

#else


/* Priority boosting/monitoring:  Adapted from portaudio/pa_unix.c ,
 * which carries the following copyright notice:
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 * Linux OSS Implementation by douglas repetto and Phil Burk
 *
 * Copyright (c) 1999-2000 Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 */

/* It has been clarified by the authors that the request to send modifications
   is a request, and not a condition */

/* Theory:
 *  The main thread is boosted to a medium real-time priority.
 *  Two additional threads are created:
 *  Canary:  Runs as normal priority, updates a timevalue every second.
 *  WatchDog:  Runs as a higher real-time priority.  Checks to see that
 *	      Canary is running.  If Canary isn't running, lowers
 *	      priority of calling thread, which has presumably run away
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>

//#define DBUG(...) fprintf(stderr, __VA_ARGS__)
#define DBUG(...)
#define ERR_RPT(...) fprintf(stderr, __VA_ARGS__)

#define SCHEDULER_POLICY SCHED_RR
#define WATCHDOG_INTERVAL_USEC 1000000
#define WATCHDOG_MAX_SECONDS 3

typedef void *(*pthread_function_t)(void *);

typedef struct {
	int priority;
	pthread_t ThreadID;

	struct timeval CanaryTime;
	int CanaryRun;
	pthread_t CanaryThread;
	int IsCanaryThreadValid;

	int WatchDogRun;
	pthread_t WatchDogThread;
	int IsWatchDogThreadValid;

} prioboost;

static prioboost *pb;

static int CanaryProc( prioboost *b)
{
	int result = 0;
	struct sched_param schat = { 0 };

	/* set us up with normal priority, please */
	if( pthread_setschedparam(pthread_self(), SCHED_OTHER, &schat) != 0)
		return 1;

	while( b->CanaryRun)
	{
		usleep( WATCHDOG_INTERVAL_USEC );
		gettimeofday( &b->CanaryTime, NULL );
	}

	return result;
}

static int WatchDogProc( prioboost *b )
{
	struct sched_param    schp = { 0 };
	int                   maxPri;

	/* Run at a priority level above main thread so we can still run if it hangs. */
	/* Rise more than 1 because of rumored off-by-one scheduler bugs. */
	schp.sched_priority = b->priority + 4;
	maxPri = sched_get_priority_max(SCHEDULER_POLICY);
	if( schp.sched_priority > maxPri ) schp.sched_priority = maxPri;

	if (pthread_setschedparam(pthread_self(), SCHEDULER_POLICY, &schp) != 0)
	{
		ERR_RPT("WatchDogProc: cannot set watch dog priority!\n");
		goto killAudio;
	}

	DBUG("prioboost: WatchDog priority set to level %d!\n", schp.sched_priority);

	/* Compare watchdog time with audio and canary thread times. */
	/* Sleep for a while or until thread cancelled. */
	while( b->WatchDogRun )
	{

		int              delta;
		struct timeval   currentTime;

		usleep( WATCHDOG_INTERVAL_USEC );
		gettimeofday( &currentTime, NULL );

#if 0
		/* If audio thread is not advancing, then it must be hung so kill it. */
		delta = currentTime.tv_sec - b->EntryTime.tv_sec;
		DBUG(("WatchDogProc: audio delta = %d\n", delta ));
		if( delta > WATCHDOG_MAX_SECONDS )
		{
			goto killAudio;
		}
#endif

		/* If canary died, then lower audio priority and halt canary. */
		delta = currentTime.tv_sec - b->CanaryTime.tv_sec;
		DBUG("WatchDogProc: dogging, delta = %ld, mysec=%d\n", delta, currentTime.tv_sec);
		if( delta > WATCHDOG_MAX_SECONDS )
		{
			ERR_RPT("WatchDogProc: canary died!\n");
			goto lowerAudio;
		}
	}

	DBUG("WatchDogProc: exiting.\n");
	return 0;

lowerAudio:
	{
		struct sched_param    schat = { 0 };
		if( pthread_setschedparam(b->ThreadID, SCHED_OTHER, &schat) != 0)
		{
			ERR_RPT("WatchDogProc: failed to lower audio priority. errno = %d\n", errno );
			/* Fall through into killing audio thread. */
		}
		else
		{
			ERR_RPT("WatchDogProc: lowered audio priority to prevent hogging of CPU.\n");
			goto cleanup;
		}
	}

killAudio:
	ERR_RPT("WatchDogProc: killing hung audio thread!\n");
	//pthread_cancel( b->ThreadID);
	//pthread_join( b->ThreadID);
	exit(1);

cleanup:
	b->CanaryRun = 0;
	DBUG("WatchDogProc: cancel Canary\n");
	pthread_cancel( b->CanaryThread );
	DBUG("WatchDogProc: join Canary\n");
	pthread_join( b->CanaryThread, NULL );
	DBUG("WatchDogProc: forget Canary\n");
	b->IsCanaryThreadValid = 0;

#ifdef GNUSTEP
	GSUnregisterCurrentThread();  /* SB20010904 */
#endif
	return 0;
}

static void StopWatchDog( prioboost *b )
{
	/* Cancel WatchDog thread if there is one. */
	if( b->IsWatchDogThreadValid )
	{
		b->WatchDogRun = 0;
		DBUG("StopWatchDog: cancel WatchDog\n");
		pthread_cancel( b->WatchDogThread );
		pthread_join( b->WatchDogThread, NULL );
		b->IsWatchDogThreadValid = 0;
	}
	/* Cancel Canary thread if there is one. */
	if( b->IsCanaryThreadValid )
	{
		b->CanaryRun = 0;
		DBUG("StopWatchDog: cancel Canary\n");
		pthread_cancel( b->CanaryThread );
		DBUG("StopWatchDog: join Canary\n");
		pthread_join( b->CanaryThread, NULL );
		b->IsCanaryThreadValid = 0;
	}
}


static int StartWatchDog( prioboost *b)
{
	int  hres;
	int  result = 0;

	/* The watch dog watches for these timer updates */
	gettimeofday( &b->CanaryTime, NULL );

	/* Launch a canary thread to detect priority abuse. */
	b->CanaryRun = 1;
	hres = pthread_create(&(b->CanaryThread),
			NULL /*pthread_attr_t * attr*/,
			(pthread_function_t)CanaryProc, b);
	if( hres != 0 )
	{
		b->IsCanaryThreadValid = 0;
		result = 1;
		goto error;
	}
	b->IsCanaryThreadValid = 1;

	/* Launch a watchdog thread to prevent runaway audio thread. */
	b->WatchDogRun = 1;
	hres = pthread_create(&(b->WatchDogThread),
			NULL /*pthread_attr_t * attr*/,
			(pthread_function_t)WatchDogProc, b);
	if( hres != 0 )     {
		b->IsWatchDogThreadValid = 0;
		result = 1;
		goto error;
	}
	b->IsWatchDogThreadValid = 1;
	return result;

error:
	StopWatchDog( b );
	return result;
}

int iaxci_prioboostbegin()
{
	struct sched_param   schp = { 0 };
	prioboost *b = calloc(sizeof(*b),1);

	int result = 0;

	b->priority = (sched_get_priority_max(SCHEDULER_POLICY) -
			sched_get_priority_min(SCHEDULER_POLICY)) / 2;
	schp.sched_priority = b->priority;

	b->ThreadID = pthread_self();

	if (pthread_setschedparam(b->ThreadID, SCHEDULER_POLICY, &schp) != 0)
	{
		DBUG("prioboost: only superuser can use real-time priority.\n");
	}
	else
	{
		DBUG("prioboost: priority set to level %d!\n", schp.sched_priority);        /* We are running at high priority so we should have a watchdog in case audio goes wild. */
		result = StartWatchDog( b );
	}

	if(result == 0)  {
		pb = b;
	} else {
		pb = NULL;
		schp.sched_priority = 0;
		pthread_setschedparam(b->ThreadID, SCHED_OTHER, &schp);
	}

	return result;
}

int iaxci_prioboostend()
{
	if(pb) StopWatchDog(pb);
	return 0;
}

#endif

