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

#include "iaxclient_lib.h"

#include <windows.h>
#include <winbase.h>

#include <stdio.h>

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

int iaxci_post_event_callback(iaxc_event ev) {
	iaxc_event *e;
	e = (iaxc_event *)malloc(sizeof(ev));
	*e = ev;

	if (!PostMessage((HWND)post_event_handle,post_event_id,(WPARAM) NULL, (LPARAM) e))
		free(e);
	return 0;
}

/* Increasing the Thread Priority.  See
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/scheduling_priorities.asp
 * for discussion on Win32 scheduling priorities.
 */

int iaxci_prioboostbegin() {
    if ( !SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL)  ) {
        fprintf(stderr, "SetThreadPriority failed: %ld.\n", GetLastError());
    }
    return 0;
}

int iaxci_prioboostend() {
    /* TODO */
    return 0;
}

