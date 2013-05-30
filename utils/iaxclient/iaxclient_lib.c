/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003-2006, Horizon Wimba, Inc.
 * Copyright (C) 2007, Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Michael Van Donselaar <mvand@vandonselaar.org>
 * Shawn Lawrence <shawn.lawrence@terracecomm.com>
 * Frik Strecker <frik@gatherworks.com>
 * Mihai Balea <mihai AT hates DOT ms>
 * Peter Grayson <jpgrayson@gmail.com>
 * Bill Cholewka <bcholew@gmail.com>
 * Erik Bunce <kde@bunce.us>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License.
 */

#include <assert.h>

#if defined(WIN32) || defined(_WIN32_WCE)
#include <stdlib.h>
#endif

/* Win32 has _vsnprintf instead of vsnprintf */
#if ! HAVE_VSNPRINTF
# if HAVE__VSNPRINTF
#  define vsnprintf _vsnprintf
# endif
#endif

#include "iaxclient_lib.h"
#include "audio_portaudio.h"
#include "audio_encode.h"
#ifdef USE_VIDEO
#include "video.h"
#endif
#include "iax-client.h"
#include "jitterbuf.h"

#if STDC_HEADERS
# include <stdarg.h>
#else
# if HAVE_VARARGS_H
#  include <varargs.h>
# endif
#endif

#ifdef AUDIO_ALSA
#include "audio_alsa.h"
#endif

#define IAXC_ERROR  IAXC_TEXT_TYPE_ERROR
#define IAXC_STATUS IAXC_TEXT_TYPE_STATUS
#define IAXC_NOTICE IAXC_TEXT_TYPE_NOTICE

#define DEFAULT_CALLERID_NAME    "Not Available"
#define DEFAULT_CALLERID_NUMBER  "7005551212"

#undef JB_DEBUGGING

/* global test mode flag */
int test_mode = 0;

/* configurable jitterbuffer options */
static long jb_target_extra = -1;

struct iaxc_registration
{
	struct iax_session *session;
	struct timeval last;
	char host[256];
	char user[256];
	char pass[256];
	long refresh;
	int id;
	struct iaxc_registration *next;
};

static int next_registration_id = 0;
static struct iaxc_registration *registrations = NULL;

static struct iaxc_audio_driver audio_driver;

static int audio_format_capability;
static int audio_format_preferred;

// Audio callback behavior
// By default apps should let iaxclient handle audio
static unsigned int audio_prefs = 0;

void * post_event_handle = NULL;
int post_event_id = 0;

static int minimum_outgoing_framesize = 160; /* 20ms */

static MUTEX iaxc_lock;
static MUTEX event_queue_lock;

static int iaxci_bound_port = -1;

// default to use port 4569 unless set by iaxc_set_preferred_source_udp_port
static int source_udp_port = IAX_DEFAULT_PORTNO;

int iaxci_audio_output_mode = 0; // Normal

int selected_call; // XXX to be protected by mutex?
struct iaxc_call* calls;
int max_calls; // number of calls for this library session

static void service_network();
static int service_audio();

/* external global networking replacements */
static iaxc_sendto_t iaxc_sendto = (iaxc_sendto_t)sendto;
static iaxc_recvfrom_t iaxc_recvfrom = (iaxc_recvfrom_t)recvfrom;


static THREAD main_proc_thread;
#if defined(WIN32) || defined(_WIN32_WCE)
static THREADID main_proc_thread_id;
#endif

/* 0 running, 1 should quit, -1 not running */
static int main_proc_thread_flag = -1;

static iaxc_event_callback_t iaxc_event_callback = NULL;

// Internal queue of events, waiting to be posted once the library
// lock is released.
static iaxc_event *event_queue = NULL;

// Lock the library
static void get_iaxc_lock()
{
	MUTEXLOCK(&iaxc_lock);
}

int try_iaxc_lock()
{
	return MUTEXTRYLOCK(&iaxc_lock);
}

// Unlock the library and post any events that were queued in the meantime
void put_iaxc_lock()
{
	iaxc_event *prev, *event;

	MUTEXLOCK(&event_queue_lock);
	event = event_queue;
	event_queue = NULL;
	MUTEXUNLOCK(&event_queue_lock);

	MUTEXUNLOCK(&iaxc_lock);

	while (event)
	{
		iaxci_post_event(*event);
		prev = event;
		event = event->next;
		free(prev);
	}
}

EXPORT void iaxc_set_audio_output(int mode)
{
	iaxci_audio_output_mode = mode;
}

long iaxci_usecdiff(struct timeval * t0, struct timeval * t1)
{
	return (t0->tv_sec - t1->tv_sec) * 1000000L + t0->tv_usec - t1->tv_usec;
}

long iaxci_msecdiff(struct timeval * t0, struct timeval * t1)
{
	return iaxci_usecdiff(t0, t1) / 1000L;
}

EXPORT void iaxc_set_event_callback(iaxc_event_callback_t func)
{
	iaxc_event_callback = func;
}

EXPORT int iaxc_set_event_callpost(void *handle, int id)
{
	post_event_handle = handle;
	post_event_id = id;
	iaxc_event_callback = iaxci_post_event_callback;
	return 0;
}

EXPORT void iaxc_free_event(iaxc_event *e)
{
	free(e);
}

EXPORT struct iaxc_ev_levels *iaxc_get_event_levels(iaxc_event *e)
{
	return &e->ev.levels;
}

EXPORT struct iaxc_ev_text *iaxc_get_event_text(iaxc_event *e)
{
	return &e->ev.text;
}

EXPORT struct iaxc_ev_call_state *iaxc_get_event_state(iaxc_event *e)
{
	return &e->ev.call;
}

// Messaging functions
static void default_message_callback(const char * message)
{
	fprintf(stderr, "IAXCLIENT: %s\n", message);
}

// Post Events back to clients
void iaxci_post_event(iaxc_event e)
{
	if ( e.type == 0 )
	{
		iaxci_usermsg(IAXC_ERROR,
			"Error: something posted to us an invalid event");
		return;
	}

	if ( MUTEXTRYLOCK(&iaxc_lock) )
	{
		iaxc_event **tail;

		/* We could not obtain the lock. Queue the event. */
		MUTEXLOCK(&event_queue_lock);
		tail = &event_queue;
		e.next = NULL;
		while ( *tail )
			tail = &(*tail)->next;
		*tail = (iaxc_event *)malloc(sizeof(iaxc_event));
		memcpy(*tail, &e, sizeof(iaxc_event));
		MUTEXUNLOCK(&event_queue_lock);
		return;
	}

	/* TODO: This is not the best. Since we were able to get the
	 * lock, we decide that it is okay to go ahead and do the
	 * callback to the application. This is really nasty because
	 * it gives the appearance of serialized callbacks, but in
	 * reality, we could callback an application multiple times
	 * simultaneously. So, as things stand, an application must
	 * do some locking in their callback function to make it
	 * reentrant. Barf. More ideally, iaxclient would guarantee
	 * serialized callbacks to the application.
	 */
	MUTEXUNLOCK(&iaxc_lock);

	if ( iaxc_event_callback )
	{
		int rv;

		rv = iaxc_event_callback(e);

		if ( e.type == IAXC_EVENT_VIDEO )
		{
			/* We can free the frame data once it is off the
			 * event queue and has been processed by the client.
			 */
			free(e.ev.video.data);
		}
		else if ( e.type == IAXC_EVENT_AUDIO )
		{
			free(e.ev.audio.data);
		}

		if ( rv < 0 )
			default_message_callback(
				"IAXCLIENT: BIG PROBLEM, event callback returned failure!");
		// > 0 means processed
		if ( rv > 0 )
			return;

		// else, fall through to "defaults"
	}

	switch ( e.type )
	{
		case IAXC_EVENT_TEXT:
			default_message_callback(e.ev.text.message);
			// others we just ignore too
			return;
	}
}


void iaxci_usermsg(int type, const char *fmt, ...)
{
	va_list args;
	iaxc_event e;

	e.type = IAXC_EVENT_TEXT;
	e.ev.text.type = type;
	e.ev.text.callNo = -1;
	va_start(args, fmt);
	vsnprintf(e.ev.text.message, IAXC_EVENT_BUFSIZ, fmt, args);
	va_end(args);

	iaxci_post_event(e);
}


void iaxci_do_levels_callback(float input, float output)
{
	iaxc_event e;
	e.type = IAXC_EVENT_LEVELS;
	e.ev.levels.input = input;
	e.ev.levels.output = output;
	iaxci_post_event(e);
}

void iaxci_do_state_callback(int callNo)
{
	iaxc_event e;
	if ( callNo < 0 || callNo >= max_calls )
		return;
	e.type = IAXC_EVENT_STATE;
	e.ev.call.callNo = callNo;
	e.ev.call.state = calls[callNo].state;
	e.ev.call.format = calls[callNo].format;
	e.ev.call.vformat = calls[callNo].vformat;
	strncpy(e.ev.call.remote,        calls[callNo].remote,        IAXC_EVENT_BUFSIZ);
	strncpy(e.ev.call.remote_name,   calls[callNo].remote_name,   IAXC_EVENT_BUFSIZ);
	strncpy(e.ev.call.local,         calls[callNo].local,         IAXC_EVENT_BUFSIZ);
	strncpy(e.ev.call.local_context, calls[callNo].local_context, IAXC_EVENT_BUFSIZ);
	iaxci_post_event(e);
}

void iaxci_do_registration_callback(int id, int reply, int msgcount)
{
	iaxc_event e;
	e.type = IAXC_EVENT_REGISTRATION;
	e.ev.reg.id = id;
	e.ev.reg.reply = reply;
	e.ev.reg.msgcount = msgcount;
	iaxci_post_event(e);
}

void iaxci_do_audio_callback(int callNo, unsigned int ts, int source,
		int encoded, int format, int size, unsigned char *data)
{
	iaxc_event e;

	e.type = IAXC_EVENT_AUDIO;
	e.ev.audio.ts = ts;
	e.ev.audio.encoded = encoded;
	assert(source == IAXC_SOURCE_REMOTE || source == IAXC_SOURCE_LOCAL);
	e.ev.audio.source = source;
	e.ev.audio.size = size;
	e.ev.audio.callNo = callNo;
	e.ev.audio.format = format;

	e.ev.audio.data = (unsigned char *)malloc(size);

	if ( !e.ev.audio.data )
	{
		iaxci_usermsg(IAXC_ERROR,
				"failed to allocate memory for audio event");
		return;
	}

	memcpy(e.ev.audio.data, data, size);

	iaxci_post_event(e);
}

void iaxci_do_dtmf_callback(int callNo, char digit)
{
	iaxc_event e;
	e.type = IAXC_EVENT_DTMF;
	e.ev.dtmf.callNo = callNo;
	e.ev.dtmf.digit  = digit;
	iaxci_post_event(e);
}

static int iaxc_remove_registration_by_id(int id)
{
	struct iaxc_registration *curr, *prev;
	int count = 0;
	for ( prev = NULL, curr = registrations; curr != NULL;
			prev = curr, curr = curr->next )
	{
		if ( curr->id == id )
		{
			count++;
			if ( curr->session != NULL )
				iax_destroy( curr->session );
			if ( prev != NULL )
				prev->next = curr->next;
			else
				registrations = curr->next;
			free( curr );
			break;
		}
	}
	return count;
}

EXPORT int iaxc_first_free_call()
{
	int i;
	for ( i = 0; i < max_calls; i++ )
		if ( calls[i].state == IAXC_CALL_STATE_FREE )
			return i;

	return -1;
}


static void iaxc_clear_call(int toDump)
{
	// XXX libiax should handle cleanup, I think..
	calls[toDump].state = IAXC_CALL_STATE_FREE;
	calls[toDump].format = 0;
	calls[toDump].vformat = 0;
	calls[toDump].session = NULL;
	iaxci_do_state_callback(toDump);
}

/* select a call.  */
/* XXX Locking??  Start/stop audio?? */
EXPORT int iaxc_select_call(int callNo)
{
	// continue if already selected?
	//if ( callNo == selected_call ) return;

	if ( callNo >= max_calls )
	{
		iaxci_usermsg(IAXC_ERROR, "Error: tried to select out_of_range call %d", callNo);
		return -1;
	}

	// callNo < 0 means no call selected (i.e. all on hold)
	if ( callNo < 0 )
	{
		if ( selected_call >= 0 )
		{
			calls[selected_call].state &= ~IAXC_CALL_STATE_SELECTED;
		}
		selected_call = callNo;
		return 0;
	}

	// de-select and notify the old call if not also the new call
	if ( callNo != selected_call )
	{
		if ( selected_call >= 0 )
		{
			calls[selected_call].state &= ~IAXC_CALL_STATE_SELECTED;
			iaxci_do_state_callback(selected_call);
		}
		selected_call = callNo;
		calls[selected_call].state |= IAXC_CALL_STATE_SELECTED;
	}

	// if it's an incoming call, and ringing, answer it.
	if ( !(calls[selected_call].state & IAXC_CALL_STATE_OUTGOING) &&
	      (calls[selected_call].state & IAXC_CALL_STATE_RINGING) )
	{
		iaxc_answer_call(selected_call);
	} else
	{
		// otherwise just update state (answer does this for us)
		iaxci_do_state_callback(selected_call);
	}

	return 0;
}

/* external API accessor */
EXPORT int iaxc_selected_call()
{
	return selected_call;
}

EXPORT void iaxc_set_networking(iaxc_sendto_t st, iaxc_recvfrom_t rf)
{
	iaxc_sendto = st;
	iaxc_recvfrom = rf;
}

EXPORT void iaxc_set_jb_target_extra( long value )
{
	/* store in jb_target_extra, a static global */
	jb_target_extra = value;
}

static void jb_errf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	iaxci_usermsg(IAXC_ERROR, buf);
}

static void jb_warnf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];

	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end(args);

	iaxci_usermsg(IAXC_NOTICE, buf);
}

#ifdef JB_DEBUGGING
static void jb_dbgf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
#endif

static void setup_jb_output()
{
#ifdef JB_DEBUGGING
	jb_setoutput(jb_errf, jb_warnf, jb_dbgf);
#else
	jb_setoutput(jb_errf, jb_warnf, NULL);
#endif
}

// Note: Must be called before iaxc_initialize()
EXPORT void iaxc_set_preferred_source_udp_port(int port)
{
	source_udp_port = port;
}

/* For "slow" systems. See iax.c code */
EXPORT int iaxc_video_bypass_jitter(int mode)
{
	/* TODO:
	 * 1. When switching from jitter to no-jitter the buffer must be
	 *    flushed of queued video packet and must be sent a key-frame
	 *    to redraw the screen (partially done).
	 * 2. When switching from no-jitter to jitter we must drop all
	 *    enqueued events prior the mode change (must be touched
	 *    iax_sched_del and iax_get_event).
	 */
	return iax_video_bypass_jitter(calls[selected_call].session,mode);
}

EXPORT int iaxc_get_bind_port()
{
	return iaxci_bound_port;
}

EXPORT int iaxc_initialize(int num_calls)
{
	int i;
	int port;

	os_init();

	setup_jb_output();

	MUTEXINIT(&iaxc_lock);
	MUTEXINIT(&event_queue_lock);

	iaxc_set_audio_prefs(0);

	if ( iaxc_recvfrom != (iaxc_recvfrom_t)recvfrom )
		iax_set_networking(iaxc_sendto, iaxc_recvfrom);

	/* Note that iax_init() only sets up the receive port when the
	 * sendto/recvfrom functions have not been replaced. We need
	 * to call iaxc_init in either case because there is other
	 * initialization beyond the socket setup that needs to be done.
	 */
	if ( (port = iax_init(source_udp_port)) < 0 )
	{
		iaxci_usermsg(IAXC_ERROR,
				"Fatal error: failed to initialize iax with port %d",
				port);
		return -1;
	}

	if ( iaxc_recvfrom == (iaxc_recvfrom_t)recvfrom )
		iaxci_bound_port = port;
	else
		iaxci_bound_port = -1;

	/* tweak the jitterbuffer settings */
	iax_set_jb_target_extra( jb_target_extra );

	max_calls = num_calls;
	/* initialize calls */
	if ( max_calls <= 0 )
		max_calls = 1; /* 0 == Default? */

	/* calloc zeroes for us */
	calls = (struct iaxc_call *)calloc(sizeof(struct iaxc_call), max_calls);
	if ( !calls )
	{
		iaxci_usermsg(IAXC_ERROR, "Fatal error: can't allocate memory");
		return -1;
	}

	selected_call = -1;

	for ( i = 0; i < max_calls; i++ )
	{
		strncpy(calls[i].callerid_name,   DEFAULT_CALLERID_NAME,   IAXC_EVENT_BUFSIZ);
		strncpy(calls[i].callerid_number, DEFAULT_CALLERID_NUMBER, IAXC_EVENT_BUFSIZ);
	}

	if ( !test_mode )
	{
#ifndef AUDIO_ALSA
		if ( pa_initialize(&audio_driver, 8000) )
		{
			iaxci_usermsg(IAXC_ERROR, "failed pa_initialize");
			return -1;
		}
#else
		/* TODO: It is unknown whether this stuff for direct access to
		* alsa should be left in iaxclient. We're leaving it in here for
		* the time being, but unless it becomes clear that someone cares
		* about having it, it will be removed. Also note that portaudio
		* is capable of using alsa. This is another reason why this
		* direct alsa access may be unneeded.
		*/
		if ( alsa_initialize(&audio_driver, 8000) )
			return -1;
#endif
	}
#ifdef USE_VIDEO
	if ( video_initialize() )
		iaxci_usermsg(IAXC_ERROR,
				"iaxc_initialize: cannot initialize video!\n");
#endif

	/* Default audio format capabilities */
	audio_format_capability =
	    IAXC_FORMAT_ULAW |
	    IAXC_FORMAT_ALAW |
#ifdef CODEC_GSM
	    IAXC_FORMAT_GSM |
#endif
	    IAXC_FORMAT_SPEEX;
	audio_format_preferred = IAXC_FORMAT_SPEEX;

	return 0;
}

EXPORT void iaxc_shutdown()
{
	iaxc_dump_all_calls();

	get_iaxc_lock();

	if ( !test_mode )
	{
		audio_driver.destroy(&audio_driver);
#ifdef USE_VIDEO
		video_destroy();
#endif
	}
	
	/* destroy enocders and decoders for all existing calls */
	if ( calls ) 
	{
                int i;
		for ( i=0 ; i<max_calls ; i++ ) 
		{
			if ( calls[i].encoder )
				calls[i].encoder->destroy(calls[i].encoder);
			if ( calls[i].decoder )
				calls[i].decoder->destroy(calls[i].decoder);
			if ( calls[i].vencoder )
				calls[i].vencoder->destroy(calls[i].vencoder);
			if ( calls[i].vdecoder )
				calls[i].vdecoder->destroy(calls[i].vdecoder);
                }
		free(calls);
		calls = NULL;
	}
	put_iaxc_lock();
#ifdef WIN32
	closesocket(iax_get_fd()); //fd:
#endif

	free(calls);

	MUTEXDESTROY(&event_queue_lock);
	MUTEXDESTROY(&iaxc_lock);
}


EXPORT void iaxc_set_formats(int preferred, int allowed)
{
	audio_format_capability = allowed;
	audio_format_preferred = preferred;
}

EXPORT void iaxc_set_min_outgoing_framesize(int samples)
{
	minimum_outgoing_framesize = samples;
}

EXPORT void iaxc_set_callerid(const char * name, const char * number)
{
	int i;

	for ( i = 0; i < max_calls; i++ )
	{
		strncpy(calls[i].callerid_name,   name,   IAXC_EVENT_BUFSIZ);
		strncpy(calls[i].callerid_number, number, IAXC_EVENT_BUFSIZ);
	}
}

static void iaxc_note_activity(int callNo)
{
	if ( callNo < 0 )
		return;
	calls[callNo].last_activity = iax_tvnow();
}

static void iaxc_refresh_registrations()
{
	struct iaxc_registration *cur;
	struct timeval now;

	now = iax_tvnow();

	for ( cur = registrations; cur != NULL; cur = cur->next )
	{
		// If there is less than three seconds before the registration is about
		// to expire, renew it.
		if ( iaxci_usecdiff(&now, &cur->last) > (cur->refresh - 3) * 1000 *1000 )
		{
			if ( cur->session != NULL )
			{
				iax_destroy( cur->session );
			}
			cur->session = iax_session_new();
			if ( !cur->session )
			{
				iaxci_usermsg(IAXC_ERROR, "Can't make new registration session");
				return;
			}
			iax_register(cur->session, cur->host, cur->user, cur->pass, cur->refresh);
			cur->last = now;
		}
	}
}

#define LOOP_SLEEP 5 // In ms
static THREADFUNCDECL(main_proc_thread_func)
{
	static int refresh_registration_count = 0;

	THREADFUNCRET(ret);

	/* Increase Priority */
	iaxci_prioboostbegin();

	while ( !main_proc_thread_flag )
	{
		get_iaxc_lock();

		service_network();
		if ( !test_mode )
			service_audio();

		// Check registration refresh once a second
		if ( refresh_registration_count++ > 1000/LOOP_SLEEP )
		{
			iaxc_refresh_registrations();
			refresh_registration_count = 0;
		}

		put_iaxc_lock();

		iaxc_millisleep(LOOP_SLEEP);
	}

	/* Decrease priority */
	iaxci_prioboostend();

	main_proc_thread_flag = -1;

	return ret;
}

EXPORT int iaxc_start_processing_thread()
{
	main_proc_thread_flag = 0;

	if ( THREADCREATE(main_proc_thread_func, NULL, main_proc_thread,
				main_proc_thread_id) == THREADCREATE_ERROR)
		return -1;

	return 0;
}

EXPORT int iaxc_stop_processing_thread()
{
	if ( main_proc_thread_flag >= 0 )
	{
		main_proc_thread_flag = 1;
		THREADJOIN(main_proc_thread);
	}

	return 0;
}

static int service_audio()
{
	/* TODO: maybe we shouldn't allocate 8kB on the stack here. */
	short buf [4096];

	int want_send_audio =
		selected_call >= 0 &&
		((calls[selected_call].state & IAXC_CALL_STATE_OUTGOING) ||
		 (calls[selected_call].state & IAXC_CALL_STATE_COMPLETE))
		&& !(audio_prefs & IAXC_AUDIO_PREF_SEND_DISABLE);

	int want_local_audio =
		(audio_prefs & IAXC_AUDIO_PREF_RECV_LOCAL_RAW) ||
		(audio_prefs & IAXC_AUDIO_PREF_RECV_LOCAL_ENCODED);

	if ( want_local_audio || want_send_audio )
	{
		for ( ;; )
		{
			int to_read;
			int cmin;

			audio_driver.start(&audio_driver);

			/* use codec minimum if higher */
			cmin = want_send_audio && calls[selected_call].encoder ?
				calls[selected_call].encoder->minimum_frame_size :
				1;

			to_read = cmin > minimum_outgoing_framesize ?
				cmin : minimum_outgoing_framesize;

			/* Round up to the next multiple */
			if ( to_read % cmin )
				to_read += cmin - (to_read % cmin);

			if ( to_read > (int)(sizeof(buf) / sizeof(short)) )
			{
				fprintf(stderr,
					"internal error: to_read > sizeof(buf)\n");
				exit(1);
			}

			/* Currently pa gives us either all the bits we ask for or none */
			if ( audio_driver.input(&audio_driver, buf, &to_read) )
			{
				iaxci_usermsg(IAXC_ERROR, "ERROR reading audio\n");
				break;
			}

			/* Frame was not available */
			if ( !to_read )
				break;

			if ( audio_prefs & IAXC_AUDIO_PREF_RECV_LOCAL_RAW )
				iaxci_do_audio_callback(selected_call, 0,
						IAXC_SOURCE_LOCAL, 0, 0,
						to_read * 2, (unsigned char *)buf);

			if ( want_send_audio )
				audio_send_encoded_audio(&calls[selected_call],
						selected_call, buf,
						calls[selected_call].format &
							IAXC_AUDIO_FORMAT_MASK,
						to_read);
		}
	}
	else
	{
		static int i = 0;

		audio_driver.stop(&audio_driver);

		/*!
			\deprecated
			Q: Why do we continuously send IAXC_EVENT_LEVELS events
		   when there is no selected call?

		 A: So that certain users of iaxclient do not have to
		   reset their vu meters when a call ends -- they can just
		   count on getting level callbacks. This is a bit of a hack
		   so any applications relying on this behavior should maybe
		   be changed.
		 */
		if ( i++ % 50 == 0 )
			iaxci_do_levels_callback(AUDIO_ENCODE_SILENCE_DB,
					AUDIO_ENCODE_SILENCE_DB);
	}

	return 0;
}

/* handle IAX text events */
static void handle_text_event(struct iax_event *e, int callNo)
{
	iaxc_event ev;
	int        len;

	if ( callNo < 0 )
		return;

	memset(&ev, 0, sizeof(iaxc_event));
	ev.type = IAXC_EVENT_TEXT;
	ev.ev.text.type = IAXC_TEXT_TYPE_IAX;
	ev.ev.text.callNo = callNo;

	len = e->datalen <= IAXC_EVENT_BUFSIZ - 1 ? e->datalen : IAXC_EVENT_BUFSIZ - 1;
	strncpy(ev.ev.text.message, (char *) e->data, len);
	iaxci_post_event(ev);
}

/* handle IAX URL events */
void handle_url_event( struct iax_event *e, int callNo )
{
	iaxc_event ev;

	if ( callNo < 0 )
		return;

	ev.ev.url.callNo = callNo;
	ev.type = IAXC_EVENT_URL;
	strcpy( ev.ev.url.url, "" );

	switch ( e->subclass )
	{
		case AST_HTML_URL:
			ev.ev.url.type = IAXC_URL_URL;
			if ( e->datalen )
			{
				if ( e->datalen > IAXC_EVENT_BUFSIZ )
				{
					fprintf( stderr, "ERROR: URL too long %d > %d\n",
							e->datalen, IAXC_EVENT_BUFSIZ );
				} else
				{
					strncpy( ev.ev.url.url, (char *) e->data, e->datalen );
				}
			}
			/* fprintf( stderr, "URL:%s\n", ev.ev.url.url ); */
			break;
		case AST_HTML_LINKURL:
			ev.ev.url.type = IAXC_URL_LINKURL;
			/* fprintf( stderr, "LINKURL event\n" ); */
			break;
		case AST_HTML_LDCOMPLETE:
			ev.ev.url.type = IAXC_URL_LDCOMPLETE;
			/* fprintf( stderr, "LDCOMPLETE event\n" ); */
			break;
		case AST_HTML_UNLINK:
			ev.ev.url.type = IAXC_URL_UNLINK;
			/* fprintf( stderr, "UNLINK event\n" ); */
			break;
		case AST_HTML_LINKREJECT:
			ev.ev.url.type = IAXC_URL_LINKREJECT;
			/* fprintf( stderr, "LINKREJECT event\n" ); */
			break;
		default:
			fprintf( stderr, "Unknown URL event %d\n", e->subclass );
			break;
	}
	iaxci_post_event( ev );
}

/* DANGER: bad things can happen if iaxc_netstat != iax_netstat.. */
EXPORT int iaxc_get_netstats(int call, int *rtt, struct iaxc_netstat *local,
		struct iaxc_netstat *remote)
{
	return iax_get_netstats(calls[call].session, rtt,
			(struct iax_netstat *)local,
			(struct iax_netstat *)remote);
}

/* handle IAX text events */
static void generate_netstat_event(int callNo)
{
	iaxc_event ev;

	if ( callNo < 0 )
		return;

	ev.type = IAXC_EVENT_NETSTAT;
	ev.ev.netstats.callNo = callNo;

	/* only post the event if the session is valid, etc */
	if ( !iaxc_get_netstats(callNo, &ev.ev.netstats.rtt,
				&ev.ev.netstats.local, &ev.ev.netstats.remote))
		iaxci_post_event(ev);
}

static void handle_audio_event(struct iax_event *e, int callNo)
{
	int total_consumed = 0;
	short fr[4096];
	const int fr_samples = sizeof(fr) / sizeof(short);
	int samples, format;
#ifdef WIN32
	int cycles_max = 100; //fd:
#endif
	struct iaxc_call *call;

	if ( callNo < 0 )
		return;

	call = &calls[callNo];

	if ( callNo != selected_call )
	{
	    /* drop audio for unselected call? */
	    return;
	}

	samples = fr_samples;
	format = call->format & IAXC_AUDIO_FORMAT_MASK;

	do
	{
		int bytes_decoded;

		int mainbuf_delta = fr_samples - samples;

		bytes_decoded = audio_decode_audio(call,
				fr,
				e->data + total_consumed,
				e->datalen - total_consumed,
				format,
				&samples);

		if ( bytes_decoded < 0 )
		{
			iaxci_usermsg(IAXC_STATUS,
				"Bad or incomplete voice packet. Unable to decode. dropping");
			return;
		}

		/* Pass encoded audio back to the app if required */
		if ( audio_prefs & IAXC_AUDIO_PREF_RECV_REMOTE_ENCODED )
			iaxci_do_audio_callback(callNo, e->ts, IAXC_SOURCE_REMOTE,
					1, format & IAXC_AUDIO_FORMAT_MASK,
					e->datalen - total_consumed,
					e->data + total_consumed);

#ifdef WIN32
		//fd: start: for some reason it loops here. Try to avoid it
		cycles_max--;
		if ( cycles_max < 0 )
		{
			iaxc_millisleep(0);
		}
		//fd: end
#endif
		total_consumed += bytes_decoded;
		if ( audio_prefs & IAXC_AUDIO_PREF_RECV_REMOTE_RAW )
		{
			// audio_decode_audio returns the number of samples.
			// We are using 16 bit samples, so we need to double
			// the number to obtain the size in bytes.
			// format will also be 0 since this is raw audio
			int size = (fr_samples - samples - mainbuf_delta) * 2;
			iaxci_do_audio_callback(callNo, e->ts, IAXC_SOURCE_REMOTE,
					0, 0, size, (unsigned char *)fr);
		}

		if ( iaxci_audio_output_mode )
			continue;

		if ( !test_mode )
			audio_driver.output(&audio_driver, fr,
			    fr_samples - samples - mainbuf_delta);

	} while ( total_consumed < e->datalen );
}

#ifdef USE_VIDEO
static void handle_video_event(struct iax_event *e, int callNo)
{
	struct iaxc_call *call;

	if ( callNo < 0 )
		return;

	if ( e->datalen == 0 )
	{
		iaxci_usermsg(IAXC_STATUS, "Received 0-size packet. Unable to decode.");
		return;
	}

	call = &calls[callNo];

	if ( callNo != selected_call )
	{
		/* drop video for unselected call? */
		return;
	}

	if ( call->vformat )
	{
		if ( video_recv_video(call, selected_call, e->data,
					e->datalen, e->ts, call->vformat) < 0 )
		{
			iaxci_usermsg(IAXC_STATUS,
				"Bad or incomplete video packet. Unable to decode.");
			return;
		}
	}
}
#endif	/* USE_VIDEO */

static void iaxc_handle_network_event(struct iax_event *e, int callNo)
{
	if ( callNo < 0 )
		return;

	iaxc_note_activity(callNo);

	switch ( e->etype )
	{
	case IAX_EVENT_NULL:
		break;
	case IAX_EVENT_HANGUP:
		iaxci_usermsg(IAXC_STATUS, "Call disconnected by remote");
		// XXX does the session go away now?
		iaxc_clear_call(callNo);
		break;
	case IAX_EVENT_REJECT:
		iaxci_usermsg(IAXC_STATUS, "Call rejected by remote");
		iaxc_clear_call(callNo);
		break;
	case IAX_EVENT_ACCEPT:
		calls[callNo].format = e->ies.format & IAXC_AUDIO_FORMAT_MASK;
		calls[callNo].vformat = e->ies.format & IAXC_VIDEO_FORMAT_MASK;
		if ( !(e->ies.format & IAXC_VIDEO_FORMAT_MASK) )
		{
			iaxci_usermsg(IAXC_NOTICE,
					"Failed video codec negotiation.");
		}
		iaxci_usermsg(IAXC_STATUS,"Call %d accepted", callNo);
		break;
	case IAX_EVENT_ANSWER:
		calls[callNo].state &= ~IAXC_CALL_STATE_RINGING;
		calls[callNo].state |= IAXC_CALL_STATE_COMPLETE;
		iaxci_do_state_callback(callNo);
		iaxci_usermsg(IAXC_STATUS,"Call %d answered", callNo);
		//iaxc_answer_call(callNo);
		// notify the user?
		break;
	case IAX_EVENT_BUSY:
		calls[callNo].state &= ~IAXC_CALL_STATE_RINGING;
		calls[callNo].state |= IAXC_CALL_STATE_BUSY;
		iaxci_do_state_callback(callNo);
		iaxci_usermsg(IAXC_STATUS, "Call %d busy", callNo);
		break;
	case IAX_EVENT_VOICE:
		handle_audio_event(e, callNo);
		if ( (calls[callNo].state & IAXC_CALL_STATE_OUTGOING) &&
		     (calls[callNo].state & IAXC_CALL_STATE_RINGING) )
		{
			calls[callNo].state &= ~IAXC_CALL_STATE_RINGING;
			calls[callNo].state |= IAXC_CALL_STATE_COMPLETE;
			iaxci_do_state_callback(callNo);
			iaxci_usermsg(IAXC_STATUS,"Call %d progress",
				     callNo);
		}
		break;
#ifdef USE_VIDEO
	case IAX_EVENT_VIDEO:
		handle_video_event(e, callNo);
		break;
#endif
	case IAX_EVENT_TEXT:
		handle_text_event(e, callNo);
		break;
	case IAX_EVENT_RINGA:
		calls[callNo].state |= IAXC_CALL_STATE_RINGING;
		iaxci_do_state_callback(callNo);
		iaxci_usermsg(IAXC_STATUS,"Call %d ringing", callNo);
		break;
	case IAX_EVENT_PONG:
		generate_netstat_event(callNo);
		break;
	case IAX_EVENT_URL:
		handle_url_event(e, callNo);
		break;
	case IAX_EVENT_CNG:
		/* ignore? */
		break;
	case IAX_EVENT_TIMEOUT:
		iax_hangup(e->session, "Call timed out");
		iaxci_usermsg(IAXC_STATUS, "Call %d timed out.", callNo);
		iaxc_clear_call(callNo);
		break;
	case IAX_EVENT_TRANSFER:
		calls[callNo].state |= IAXC_CALL_STATE_TRANSFER;
		iaxci_do_state_callback(callNo);
		iaxci_usermsg(IAXC_STATUS,"Call %d transfer released", callNo);
		break;
	case IAX_EVENT_DTMF:
		iaxci_do_dtmf_callback(callNo,e->subclass);
		iaxci_usermsg(IAXC_STATUS, "DTMF digit %c received", e->subclass);
        	break;
	default:
		iaxci_usermsg(IAXC_STATUS, "Unknown event: %d for call %d", e->etype, callNo);
		break;
	}
}

EXPORT int iaxc_unregister( int id )
{
	int count = 0;
	get_iaxc_lock();
	count = iaxc_remove_registration_by_id(id);
	put_iaxc_lock();
	return count;
}

EXPORT int iaxc_register(const char * user, const char * pass, const char * host)
{
	return iaxc_register_ex(user, pass, host, 60);
}

EXPORT int iaxc_register_ex(const char * user, const char * pass, const char * host, int refresh)
{
	struct iaxc_registration *newreg;

	newreg = (struct iaxc_registration *)malloc(sizeof (struct iaxc_registration));
	if ( !newreg )
	{
		iaxci_usermsg(IAXC_ERROR, "Can't make new registration");
		return -1;
	}

	get_iaxc_lock();
	newreg->session = iax_session_new();
	if ( !newreg->session )
	{
		iaxci_usermsg(IAXC_ERROR, "Can't make new registration session");
		put_iaxc_lock();
		return -1;
	}

	newreg->last = iax_tvnow();
	newreg->refresh = refresh;  

	strncpy(newreg->host, host, 256);
	strncpy(newreg->user, user, 256);
	strncpy(newreg->pass, pass, 256);

	/* send out the initial registration with refresh seconds */
	iax_register(newreg->session, host, user, pass, refresh);

	/* add it to the list; */
	newreg->id = ++next_registration_id;
	newreg->next = registrations;
	registrations = newreg;

	put_iaxc_lock();
	return newreg->id;
}

static void codec_destroy( int callNo )
{
	if ( calls[callNo].encoder )
	{
		calls[callNo].encoder->destroy( calls[callNo].encoder );
		calls[callNo].encoder = NULL;
	}
	if ( calls[callNo].decoder )
	{
		calls[callNo].decoder->destroy( calls[callNo].decoder );
		calls[callNo].decoder = NULL;
	}
	if ( calls[callNo].vdecoder )
	{
		calls[callNo].vdecoder->destroy(calls[callNo].vdecoder);
		calls[callNo].vdecoder = NULL;
	}
	if ( calls[callNo].vencoder )
	{
		calls[callNo].vencoder->destroy(calls[callNo].vencoder);
		calls[callNo].vencoder = NULL;
	}
}

EXPORT int iaxc_call(const char * num)
{
	return iaxc_call_ex(num, NULL, NULL, 1);
}

EXPORT int iaxc_call_ex(const char *num, const char* callerid_name, const char* callerid_number, int video)
{
	int video_format_capability = 0;
	int video_format_preferred = 0;
	int callNo = -1;
	struct iax_session *newsession;
	char *ext = strstr(num, "/");

	get_iaxc_lock();

	// if no call is selected, get a new appearance
	if ( selected_call < 0 )
	{
		callNo = iaxc_first_free_call();
	} else
	{
		// use selected call if not active, otherwise, get a new appearance
		if ( calls[selected_call].state  & IAXC_CALL_STATE_ACTIVE )
		{
			callNo = iaxc_first_free_call();
		} else
		{
			callNo = selected_call;
		}
	}

	if ( callNo < 0 )
	{
		iaxci_usermsg(IAXC_STATUS, "No free call appearances");
		goto iaxc_call_bail;
	}

	newsession = iax_session_new();
	if ( !newsession )
	{
		iaxci_usermsg(IAXC_ERROR, "Can't make new session");
		goto iaxc_call_bail;
	}

	calls[callNo].session = newsession;

	codec_destroy( callNo );

	if ( ext )
	{
		strncpy(calls[callNo].remote_name, num, IAXC_EVENT_BUFSIZ);
		strncpy(calls[callNo].remote,    ++ext, IAXC_EVENT_BUFSIZ);
	} else
	{
		strncpy(calls[callNo].remote_name, num, IAXC_EVENT_BUFSIZ);
		strncpy(calls[callNo].remote,      "" , IAXC_EVENT_BUFSIZ);
	}

	if ( callerid_number != NULL )
		strncpy(calls[callNo].callerid_number, callerid_number, IAXC_EVENT_BUFSIZ);

	if ( callerid_name != NULL )
		strncpy(calls[callNo].callerid_name, callerid_name, IAXC_EVENT_BUFSIZ);

	strncpy(calls[callNo].local        , calls[callNo].callerid_name, IAXC_EVENT_BUFSIZ);
	strncpy(calls[callNo].local_context, "default", IAXC_EVENT_BUFSIZ);

	calls[callNo].state = IAXC_CALL_STATE_ACTIVE | IAXC_CALL_STATE_OUTGOING;

	/* reset activity and ping "timers" */
	iaxc_note_activity(callNo);
	calls[callNo].last_ping = calls[callNo].last_activity;

#ifdef USE_VIDEO
	if ( video )
		iaxc_video_format_get_cap(&video_format_preferred, &video_format_capability);
#endif

	iaxci_usermsg(IAXC_NOTICE, "Originating an %s call",
			video_format_preferred ? "audio+video" : "audio only");
	iax_call(calls[callNo].session, calls[callNo].callerid_number,
			calls[callNo].callerid_name, num, NULL, 0,
			audio_format_preferred | video_format_preferred,
			audio_format_capability | video_format_capability);

	// does state stuff also
	iaxc_select_call(callNo);

iaxc_call_bail:
	put_iaxc_lock();

	return callNo;
}

EXPORT void iaxc_send_busy_on_incoming_call(int callNo)
{
	if ( callNo < 0 )
		return;

	iax_busy(calls[callNo].session);
}

EXPORT void iaxc_answer_call(int callNo)
{
	if ( callNo < 0 )
		return;

	calls[callNo].state |= IAXC_CALL_STATE_COMPLETE;
	calls[callNo].state &= ~IAXC_CALL_STATE_RINGING;
	iax_answer(calls[callNo].session);
	iaxci_do_state_callback(callNo);
}

EXPORT void iaxc_blind_transfer_call(int callNo, const char * dest_extension)
{
	if ( callNo < 0 || !(calls[callNo].state & IAXC_CALL_STATE_ACTIVE) )
		return;

	iax_transfer(calls[callNo].session, dest_extension);
}

EXPORT void iaxc_setup_call_transfer(int sourceCallNo, int targetCallNo)
{
	if ( sourceCallNo < 0 || targetCallNo < 0 ||
			!(calls[sourceCallNo].state & IAXC_CALL_STATE_ACTIVE) ||
			!(calls[targetCallNo].state & IAXC_CALL_STATE_ACTIVE) )
		return;

	iax_setup_transfer(calls[sourceCallNo].session, calls[targetCallNo].session);
}

static void iaxc_dump_one_call(int callNo)
{
	if ( callNo < 0 )
		return;
	if ( calls[callNo].state == IAXC_CALL_STATE_FREE )
		return;

	iax_hangup(calls[callNo].session,"Dumped Call");
	iaxci_usermsg(IAXC_STATUS, "Hanging up call %d", callNo);
	iaxc_clear_call(callNo);
}

EXPORT void iaxc_dump_all_calls(void)
{
	int callNo;
	get_iaxc_lock();
	for ( callNo = 0; callNo < max_calls; callNo++ )
		iaxc_dump_one_call(callNo);
	put_iaxc_lock();
}


EXPORT void iaxc_dump_call_number( int callNo )
{
	if ( ( callNo >= 0 ) && ( callNo < max_calls ) )
	{
		get_iaxc_lock();
		iaxc_dump_one_call(callNo);
		put_iaxc_lock();
	}
}

EXPORT void iaxc_dump_call(void)
{
	if ( selected_call >= 0 )
	{
		get_iaxc_lock();
		iaxc_dump_one_call(selected_call);
		put_iaxc_lock();
	}
}

EXPORT void iaxc_reject_call(void)
{
	if ( selected_call >= 0 )
	{
		iaxc_reject_call_number(selected_call);
	}
}

EXPORT void iaxc_reject_call_number( int callNo )
{
	if ( ( callNo >= 0 ) && ( callNo < max_calls ) )
	{
		get_iaxc_lock();
		iax_reject(calls[callNo].session, "Call rejected manually.");
		iaxc_clear_call(callNo);
		put_iaxc_lock();
	}
}

EXPORT void iaxc_send_dtmf(char digit)
{
	if ( selected_call >= 0 )
	{
		get_iaxc_lock();
		if ( calls[selected_call].state & IAXC_CALL_STATE_ACTIVE )
			iax_send_dtmf(calls[selected_call].session,digit);
		put_iaxc_lock();
	}
}

EXPORT void iaxc_send_text(const char * text)
{
	if ( selected_call >= 0 )
	{
		get_iaxc_lock();
		if ( calls[selected_call].state & IAXC_CALL_STATE_ACTIVE )
			iax_send_text(calls[selected_call].session, text);
		put_iaxc_lock();
	}
}

EXPORT void iaxc_send_text_call(int callNo, const char * text)
{
	if ( callNo < 0 || !(calls[callNo].state & IAXC_CALL_STATE_ACTIVE) )
		return;

	get_iaxc_lock();
	if ( calls[callNo].state & IAXC_CALL_STATE_ACTIVE )
		iax_send_text(calls[callNo].session, text);
	put_iaxc_lock();
}

EXPORT void iaxc_send_url(const char * url, int link)
{
	if ( selected_call >= 0 )
	{
		get_iaxc_lock();
		if ( calls[selected_call].state & IAXC_CALL_STATE_ACTIVE )
			iax_send_url(calls[selected_call].session, url, link);
		put_iaxc_lock();
	}
}

static int iaxc_find_call_by_session(struct iax_session *session)
{
	int i;
	for ( i = 0; i < max_calls; i++ )
		if ( calls[i].session == session )
			return i;
	return -1;
}

static struct iaxc_registration *iaxc_find_registration_by_session(
		struct iax_session *session)
{
	struct iaxc_registration *reg;
	for (reg = registrations; reg != NULL; reg = reg->next)
		if ( reg->session == session )
			break;
	return reg;
}

static void iaxc_handle_regreply(struct iax_event *e, struct iaxc_registration *reg)
{
	iaxci_do_registration_callback(reg->id, e->etype, e->ies.msgcount);

	// XXX I think the session is no longer valid.. at least, that's
	// what miniphone does, and re-using the session doesn't seem to
	// work!
	iax_destroy(reg->session);
	reg->session = NULL;

	if ( e->etype == IAX_EVENT_REGREJ )
	{
		// we were rejected, so end the registration
		iaxc_remove_registration_by_id(reg->id);
	}
}

/* this is what asterisk does */
static int iaxc_choose_codec(int formats)
{
	int i;
	static int codecs[] =
	{
		IAXC_FORMAT_ULAW,
		IAXC_FORMAT_ALAW,
		IAXC_FORMAT_SLINEAR,
		IAXC_FORMAT_G726,
		IAXC_FORMAT_ADPCM,
		IAXC_FORMAT_GSM,
		IAXC_FORMAT_ILBC,
		IAXC_FORMAT_SPEEX,
		IAXC_FORMAT_LPC10,
		IAXC_FORMAT_G729A,
		IAXC_FORMAT_G723_1,

		/* To negotiate video codec */
		IAXC_FORMAT_JPEG,
		IAXC_FORMAT_PNG,
		IAXC_FORMAT_H261,
		IAXC_FORMAT_H263,
		IAXC_FORMAT_H263_PLUS,
		IAXC_FORMAT_MPEG4,
		IAXC_FORMAT_H264,
		IAXC_FORMAT_THEORA,
	};
	for ( i = 0; i < (int)(sizeof(codecs) / sizeof(int)); i++ )
		if ( codecs[i] & formats )
			return codecs[i];
	return 0;
}

static void iaxc_handle_connect(struct iax_event * e)
{
#ifdef USE_VIDEO
	int video_format_capability;
	int video_format_preferred;
#endif
	int video_format = 0;
	int format = 0;
	int callno;

	callno = iaxc_first_free_call();

	if ( callno < 0 )
	{
		iaxci_usermsg(IAXC_STATUS,
				"%i \n Incoming Call, but no appearances",
				callno);
		// XXX Reject this call!, or just ignore?
		//iax_reject(e->session, "Too many calls, we're busy!");
		iax_accept(e->session, audio_format_preferred & e->ies.capability);
		iax_busy(e->session);
		return;
	}

	/* negotiate codec */
	/* first, try _their_ preferred format */
	format = audio_format_capability & e->ies.format;
	if ( !format )
	{
		/* then, try our preferred format */
		format = audio_format_preferred & e->ies.capability;
	}

	if ( !format )
	{
		/* finally, see if we have one in common */
		format = audio_format_capability & e->ies.capability;

		/* now choose amongst these, if we got one */
		if ( format )
		{
			format = iaxc_choose_codec(format);
		}
	}

	if ( !format )
	{
		iax_reject(e->session, "Could not negotiate common codec");
		return;
	}

#ifdef USE_VIDEO
	iaxc_video_format_get_cap(&video_format_preferred,
			&video_format_capability);

	/* first, see if they even want video */
	video_format = (e->ies.format & IAXC_VIDEO_FORMAT_MASK);

	if ( video_format )
	{
		/* next, try _their_ preferred format */
		video_format &= video_format_capability;

		if ( !video_format )
		{
			/* then, try our preferred format */
			video_format = video_format_preferred &
				(e->ies.capability & IAXC_VIDEO_FORMAT_MASK);
		}

		if ( !video_format )
		{
			/* finally, see if we have one in common */
			video_format = video_format_capability &
				(e->ies.capability & IAXC_VIDEO_FORMAT_MASK);

			/* now choose amongst these, if we got one */
			if ( video_format )
			{
				video_format = iaxc_choose_codec(video_format);
			}
		}

		/* All video negotiations failed, then warn */
		if ( !video_format )
		{
			iaxci_usermsg(IAXC_NOTICE,
					"Notice: could not negotiate common video codec");
			iaxci_usermsg(IAXC_NOTICE,
					"Notice: switching to audio-only call");
		}
	}
#endif	/* USE_VIDEO */

	calls[callno].vformat = video_format;
	calls[callno].format = format;

	if ( e->ies.called_number )
		strncpy(calls[callno].local, e->ies.called_number,
				IAXC_EVENT_BUFSIZ);
	else
		strncpy(calls[callno].local, "unknown",
				IAXC_EVENT_BUFSIZ);

	if ( e->ies.called_context )
		strncpy(calls[callno].local_context, e->ies.called_context,
				IAXC_EVENT_BUFSIZ);
	else
		strncpy(calls[callno].local_context, "",
				IAXC_EVENT_BUFSIZ);

	if ( e->ies.calling_number )
		strncpy(calls[callno].remote, e->ies.calling_number,
				IAXC_EVENT_BUFSIZ);
	else
		strncpy(calls[callno].remote, "unknown",
				IAXC_EVENT_BUFSIZ);

	if ( e->ies.calling_name )
		strncpy(calls[callno].remote_name, e->ies.calling_name,
				IAXC_EVENT_BUFSIZ);
	else
		strncpy(calls[callno].remote_name, "unknown",
				IAXC_EVENT_BUFSIZ);

	iaxc_note_activity(callno);
	iaxci_usermsg(IAXC_STATUS, "Call from (%s)", calls[callno].remote);

	codec_destroy( callno );

	calls[callno].session = e->session;
	calls[callno].state = IAXC_CALL_STATE_ACTIVE|IAXC_CALL_STATE_RINGING;

	iax_accept(calls[callno].session, format | video_format);
	iax_ring_announce(calls[callno].session);

	iaxci_do_state_callback(callno);

	iaxci_usermsg(IAXC_STATUS, "Incoming call on line %d", callno);
}

static void service_network()
{
	struct iax_event *e = 0;
	int callNo;
	struct iaxc_registration *reg;

	while ( (e = iax_get_event(0)) )
	{
#ifdef WIN32
		iaxc_millisleep(0); //fd:
#endif
		// first, see if this is an event for one of our calls.
		callNo = iaxc_find_call_by_session(e->session);
		if ( e->etype == IAX_EVENT_NULL )
		{
			// Should we do something here?
			// Right now we do nothing, just go with the flow
			// and let the event be deallocated.
		} else if ( callNo >= 0 )
		{
			iaxc_handle_network_event(e, callNo);
		} else if ( (reg = iaxc_find_registration_by_session(e->session)) != NULL )
		{
			iaxc_handle_regreply(e,reg);
		} else if ( e->etype == IAX_EVENT_REGACK || e->etype == IAX_EVENT_REGREJ )
		{
			iaxci_usermsg(IAXC_ERROR, "Unexpected registration reply");
		} else if ( e->etype == IAX_EVENT_REGREQ )
		{
			iaxci_usermsg(IAXC_ERROR,
					"Registration requested by someone, but we don't understand!");
		} else if ( e->etype == IAX_EVENT_CONNECT )
		{
			iaxc_handle_connect(e);
		} else if ( e->etype == IAX_EVENT_TIMEOUT )
		{
			iaxci_usermsg(IAXC_STATUS,
					"Timeout for a non-existant session. Dropping",
					e->etype);
		} else
		{
			iaxci_usermsg(IAXC_STATUS,
					"Event (type %d) for a non-existant session. Dropping",
					e->etype);
		}
		iax_event_free(e);
	}
}

EXPORT int iaxc_audio_devices_get(struct iaxc_audio_device **devs, int *nDevs,
		int *input, int *output, int *ring)
{
	if ( test_mode )
		return 0;

	*devs = audio_driver.devices;
	*nDevs = audio_driver.nDevices;
	audio_driver.selected_devices(&audio_driver, input, output, ring);
	return 0;
}

EXPORT int iaxc_audio_devices_set(int input, int output, int ring)
{
	int ret;

	if ( test_mode )
		return 0;

	get_iaxc_lock();
	ret = audio_driver.select_devices(&audio_driver, input, output, ring);
	put_iaxc_lock();
	return ret;
}

EXPORT float iaxc_input_level_get()
{
	if ( test_mode )
		return 0;

	return audio_driver.input_level_get(&audio_driver);
}

EXPORT float iaxc_output_level_get()
{
	if ( test_mode )
		return 0;

	return audio_driver.output_level_get(&audio_driver);
}

EXPORT int iaxc_input_level_set(float level)
{
	if ( test_mode )
		return 0;

	return audio_driver.input_level_set(&audio_driver, level);
}

EXPORT int iaxc_output_level_set(float level)
{
	if ( test_mode )
		return 0;

	return audio_driver.output_level_set(&audio_driver, level);
}

EXPORT int iaxc_play_sound(struct iaxc_sound *s, int ring)
{
	int ret;

	if ( test_mode )
		return 0;

	get_iaxc_lock();
	ret = audio_driver.play_sound(s,ring);
	put_iaxc_lock();
	return ret;
}

EXPORT int iaxc_stop_sound(int id)
{
	int ret;

	if ( test_mode )
		return 0;

	get_iaxc_lock();
	ret = audio_driver.stop_sound(id);
	put_iaxc_lock();
	return ret;
}

EXPORT int iaxc_quelch(int callNo, int MOH)
{
	struct iax_session *session = calls[callNo].session;
	if ( !session )
		return -1;

	return iax_quelch_moh(session, MOH);
}

EXPORT int iaxc_unquelch(int call)
{
	return iax_unquelch(calls[call].session);
}

EXPORT int iaxc_mic_boost_get( void )
{
	return audio_driver.mic_boost_get( &audio_driver ) ;
}

EXPORT int iaxc_mic_boost_set( int enable )
{
	return audio_driver.mic_boost_set( &audio_driver, enable ) ;
}

EXPORT char* iaxc_version(char * ver)
{
#ifndef LIBVER
#define LIBVER ""
#endif
	strncpy(ver, LIBVER, IAXC_EVENT_BUFSIZ);
	return ver;
}

EXPORT unsigned int iaxc_get_audio_prefs(void)
{
	return audio_prefs;
}

EXPORT int iaxc_set_audio_prefs(unsigned int prefs)
{
	unsigned int prefs_mask =
		IAXC_AUDIO_PREF_RECV_LOCAL_RAW      |
		IAXC_AUDIO_PREF_RECV_LOCAL_ENCODED  |
		IAXC_AUDIO_PREF_RECV_REMOTE_RAW     |
		IAXC_AUDIO_PREF_RECV_REMOTE_ENCODED |
		IAXC_AUDIO_PREF_SEND_DISABLE;

	if ( prefs & ~prefs_mask )
		return -1;

	audio_prefs = prefs;
	return 0;
}

EXPORT void iaxc_set_test_mode(int tm)
{
	test_mode = tm;
}

EXPORT int iaxc_push_audio(void *data, unsigned int size, unsigned int samples)
{
	struct iaxc_call *call;

	if ( selected_call < 0 )
		return -1;

	call = &calls[selected_call];

	if ( audio_prefs & IAXC_AUDIO_PREF_SEND_DISABLE )
		return 0;

	//fprintf(stderr, "iaxc_push_audio: sending audio size %d\n", size);

	if ( iax_send_voice(call->session, call->format, data, size, samples) == -1 )
	{
		fprintf(stderr, "iaxc_push_audio: failed to send audio frame of size %d on call %d\n", size, selected_call);
		return -1;
	}

	return 0;
}

void iaxc_debug_iax_set(int enable)
{
#ifdef DEBUG_SUPPORT
	if (enable)
		iax_enable_debug();
	else
		iax_disable_debug();
#endif
}

