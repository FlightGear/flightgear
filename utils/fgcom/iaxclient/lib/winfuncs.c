/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003 HorizonLive.com, (c) 2004, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 *
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */

#include "iaxclient_lib.h"

#include <windows.h>
#include <winbase.h>

#include <stdio.h>

#if !defined(_WIN32_WCE)
#include <sys/timeb.h>

/* Win-doze doesnt have gettimeofday(). This sux. So, what we did is
provide some gettimeofday-like functionality that works for our purposes. */

/*
	changed 'struct timezone*' to 'void*' since
	timezone is defined as a long in MINGW and caused compile-time warnings.
	this should be okay since we don't use the passed value. 
*/


/* 
 * functions implementations
 */

void gettimeofday( struct timeval* tv, void* tz )
{
	struct _timeb curSysTime;

	_ftime(&curSysTime);
	tv->tv_sec = curSysTime.time;
	tv->tv_usec = curSysTime.millitm * 1000;
}
#endif

void os_init(void)
{
	WSADATA wsd;

	if(WSAStartup(0x0101,&wsd))
	{   // Error message?
	    exit(1);
	}
}

/* yes, it could have just been a #define, but that makes linking trickier */
EXPORT void iaxc_millisleep(long ms)
{
	Sleep(ms);
}

int post_event_callback(iaxc_event ev) {
	iaxc_event *e;
	e = malloc(sizeof(ev));
	*e = ev;

	if (!PostMessage(post_event_handle,post_event_id,(WPARAM) NULL, (LPARAM) e))
		free(e);
	return 0;
}

/* Increasing the Thread Priority.  See
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/scheduling_priorities.asp
 * for discussion on Win32 scheduling priorities.
 */

int iaxc_prioboostbegin() {
    if ( !SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL)  ) {
        fprintf(stderr, "SetThreadPriority failed: %ld.\n", GetLastError());
    }
    return 0;
}

int iaxc_prioboostend() {
    /* TODO */
    return 0;
}

