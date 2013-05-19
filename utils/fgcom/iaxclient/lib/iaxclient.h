/*
 * iaxclient: a cross-platform IAX softphone library
 *
 * Copyrights:
 * Copyright (C) 2003 HorizonLive.com, (c) 2004, Horizon Wimba, Inc.
 *
 * Contributors:
 * Steve Kann <stevek@stevek.com>
 * Frik Strecker <frik@gatherworks.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser (Library) General Public License
 */
#ifndef _iaxclient_h
#define _iaxclient_h

#ifdef __cplusplus
extern "C" {
#endif


/* This is the include file which declared all external API functions to
 * IAXCLIENT.  It should include all functions and declarations needed
 * by IAXCLIENT library users, but not include internal structures, or
 * require the inclusion of library internals (or sub-libraries) */

#include <stdio.h>
#if defined(WIN32)  ||  defined(_WIN32_WCE)
#include <windows.h>
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#ifdef BUILDING_DLL
#if defined(WIN32)  ||  defined(_WIN32_WCE)
# if defined(_MSC_VER)
#  define EXPORT  __declspec(dllexport)
# else
#  define EXPORT  __stdcall __declspec(dllexport)
# endif
#else
#  define EXPORT
#endif
#else
# define EXPORT
#endif

#if defined(WIN32)  ||  defined(_WIN32_WCE)
#if defined(_MSC_VER)
	typedef int (__stdcall *iaxc_sendto_t)(SOCKET, const char *, int, int, const struct sockaddr *, int);
	typedef int (__stdcall *iaxc_recvfrom_t)(SOCKET, char *, int, int, struct sockaddr *, int *);
#else
	typedef int PASCAL (*iaxc_sendto_t)(SOCKET, const char *, int, int, const struct sockaddr *, int);
	typedef int PASCAL (*iaxc_recvfrom_t)(SOCKET, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
#endif
#else
	typedef int (*iaxc_sendto_t)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
	typedef int (*iaxc_recvfrom_t)(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
#endif

/* Define audio type constants */
#define AUDIO_INTERNAL 0
#define AUDIO_INTERNAL_PA 1
#define AUDIO_INTERNAL_FILE 2
#define AUDIO_INTERNAL_ALSA 3
#define AUDIO_INTERNAL_OPENAL 4
#define AUDIO_EXTERNAL 99

/* payload formats : WARNING: must match libiax values!!! */
/* Data formats for capabilities and frames alike */
#define IAXC_FORMAT_G723_1       (1 << 0)        /* G.723.1 compression */
#define IAXC_FORMAT_GSM          (1 << 1)        /* GSM compression */
#define IAXC_FORMAT_ULAW         (1 << 2)        /* Raw mu-law data (G.711) */
#define IAXC_FORMAT_ALAW         (1 << 3)        /* Raw A-law data (G.711) */
#define IAXC_FORMAT_G726         (1 << 4)        /* ADPCM, 32kbps  */
#define IAXC_FORMAT_ADPCM        (1 << 5)        /* ADPCM IMA */
#define IAXC_FORMAT_SLINEAR      (1 << 6)        /* Raw 16-bit Signed Linear (8000 Hz) PCM */
#define IAXC_FORMAT_LPC10        (1 << 7)        /* LPC10, 180 samples/frame */
#define IAXC_FORMAT_G729A        (1 << 8)        /* G.729a Audio */
#define IAXC_FORMAT_SPEEX        (1 << 9)        /* Speex Audio */
#define IAXC_FORMAT_ILBC         (1 << 10)       /* iLBC Audio */

#define IAXC_FORMAT_MAX_AUDIO 	 (1 << 15)  /* Maximum audio format */
#define IAXC_FORMAT_JPEG         (1 << 16)       /* JPEG Images */
#define IAXC_FORMAT_PNG          (1 << 17)       /* PNG Images */
#define IAXC_FORMAT_H261         (1 << 18)       /* H.261 Video */
#define IAXC_FORMAT_H263         (1 << 19)       /* H.263 Video */
#define IAXC_FORMAT_H263_PLUS    (1 << 20)       /* H.263+ Video */
#define IAXC_FORMAT_MPEG4    	 (1 << 21)       /* MPEG4 Video */
#define IAXC_FORMAT_H264    	 (1 << 23)       /* H264 Video */
#define IAXC_FORMAT_THEORA    	 (1 << 24)       /* Theora Video */



#define IAXC_EVENT_TEXT			1
#define IAXC_EVENT_LEVELS		2
#define IAXC_EVENT_STATE		3
#define IAXC_EVENT_NETSTAT		4
#define IAXC_EVENT_URL			5	/* URL push via IAX(2) */
#define IAXC_EVENT_VIDEO		6	/* video data (pointer) */
#define IAXC_EVENT_REGISTRATION 	7

#define IAXC_CALL_STATE_FREE 		0
#define IAXC_CALL_STATE_ACTIVE		(1<<1)
#define IAXC_CALL_STATE_OUTGOING 	(1<<2)
#define IAXC_CALL_STATE_RINGING 	(1<<3)
#define IAXC_CALL_STATE_COMPLETE 	(1<<4)
#define IAXC_CALL_STATE_SELECTED 	(1<<5)
#define IAXC_CALL_STATE_BUSY		(1<<6)
#define IAXC_CALL_STATE_TRANSFER	(1<<7)

#define IAXC_TEXT_TYPE_STATUS		1
#define IAXC_TEXT_TYPE_NOTICE		2
#define IAXC_TEXT_TYPE_ERROR		3
/* FATAL ERROR: User Agent should probably display error, then die. */
#define IAXC_TEXT_TYPE_FATALERROR	4
#define IAXC_TEXT_TYPE_IAX		5

/* registration replys, corresponding to IAX_EVENTs*/
#define IAXC_REGISTRATION_REPLY_ACK     18   /* IAX_EVENT_REGACC  */
#define IAXC_REGISTRATION_REPLY_REJ     30   /* IAX_EVENT_REGREJ  */
#define IAXC_REGISTRATION_REPLY_TIMEOUT 6    /* IAX_EVENT_TIMEOUT */

#define IAXC_URL_URL				1	/* URL received */
#define IAXC_URL_LDCOMPLETE	2	/* URL loading complete */
#define IAXC_URL_LINKURL		3	/* URL link request */
#define IAXC_URL_LINKREJECT	4	/* URL link reject */
#define IAXC_URL_UNLINK			5	/* URL unlink */
 

#define IAXC_EVENT_BUFSIZ	256
struct iaxc_ev_levels {
	float input;
	float output;
};

struct iaxc_ev_text {
	int type;
	int callNo; /* call number for IAX text */
	char message[IAXC_EVENT_BUFSIZ];
};

struct iaxc_ev_call_state {
	int callNo;
	int state;
	int format;
	char remote[IAXC_EVENT_BUFSIZ];
	char remote_name[IAXC_EVENT_BUFSIZ];
	char local[IAXC_EVENT_BUFSIZ];
	char local_context[IAXC_EVENT_BUFSIZ];
};

struct iaxc_netstat {
        int jitter;
        int losspct;
        int losscnt;
        int packets;
        int delay;
        int dropped;
        int ooo;
};

struct iaxc_ev_netstats {
	int callNo;
	int rtt;
	struct iaxc_netstat local;
	struct iaxc_netstat remote;
};

struct iaxc_ev_url {
	int callNo;
	int type;
	char url[IAXC_EVENT_BUFSIZ];
};

struct iaxc_ev_video {
	int callNo;
	int format;
	int width;
	int height;
	unsigned char *data;
};

struct iaxc_ev_registration {
    int id;
    int reply;
    int msgcount;
};

typedef struct iaxc_event_struct {
	struct iaxc_event_struct *next;
	int type;
	union {
		struct iaxc_ev_levels 		levels;
		struct iaxc_ev_text 		text;
		struct iaxc_ev_call_state 	call;
		struct iaxc_ev_netstats 	netstats;
		struct iaxc_ev_url          url;
		struct iaxc_ev_video		video;
		struct iaxc_ev_registration reg;
	} ev;
} iaxc_event;

typedef int (*iaxc_event_callback_t)(iaxc_event e);
EXPORT void iaxc_set_event_callback(iaxc_event_callback_t func);

/* Sets iaxclient to post a pointer to a copy of event using o/s specific Post method */
EXPORT int iaxc_set_event_callpost(void *handle, int id);

/* frees event delivered via o/s specific Post method */
EXPORT void iaxc_free_event(iaxc_event *e); 


/* Event Accessors */
EXPORT struct iaxc_ev_levels *iaxc_get_event_levels(iaxc_event *e);
EXPORT struct iaxc_ev_text *iaxc_get_event_text(iaxc_event *e);
EXPORT struct iaxc_ev_call_state *iaxc_get_event_state(iaxc_event *e);

// Set Preferred UDP Port: 
// 0: Use the default port (4569)
// <0: Use a dynamically assigned port
// >0: Try to bind to the specified port
// NOTE: must be called before iaxc_initialize()
EXPORT void iaxc_set_preferred_source_udp_port(int sourceUdpPort);	

EXPORT unsigned short iaxc_get_bind_port();
EXPORT int iaxc_initialize(int audType, int nCalls);
EXPORT void iaxc_shutdown();
EXPORT void iaxc_set_formats(int preferred, int allowed);
EXPORT void iaxc_set_min_outgoing_framesize(int samples);
EXPORT void iaxc_set_callerid(char *name, char *number);
EXPORT void iaxc_process_calls();
EXPORT int iaxc_service_audio();
EXPORT int iaxc_start_processing_thread();
EXPORT int iaxc_stop_processing_thread();
EXPORT void iaxc_call(char *num);
EXPORT int iaxc_unregister( int id );
EXPORT int iaxc_register(char *user, char *pass, char *host);
EXPORT void iaxc_send_busy_on_incoming_call(int callNo);    //frik
EXPORT void iaxc_answer_call(int callNo); 
EXPORT void iaxc_blind_transfer_call(int callNo, char *number); 
EXPORT void iaxc_dump_all_calls(void);
EXPORT void iaxc_dump_call(void);
EXPORT void iaxc_reject_call(void);
EXPORT void iaxc_reject_call_number(int callNo);
EXPORT void iaxc_send_dtmf(char digit);
EXPORT void iaxc_send_text(char *text);
EXPORT void iaxc_send_url(char *url, int link); /* link == 1 ? AST_HTML_LINKURL : AST_HTML_URL */
EXPORT int iaxc_was_call_answered();
EXPORT void iaxc_millisleep(long ms);
EXPORT void iaxc_set_silence_threshold(double thr);
EXPORT void iaxc_set_audio_output(int mode);
EXPORT int iaxc_select_call(int callNo);
EXPORT int iaxc_first_free_call();
EXPORT int iaxc_selected_call();
EXPORT int iaxc_quelch(int callNo, int MOH);
EXPORT int iaxc_unquelch(int call);
EXPORT int iaxc_mic_boost_get( void ) ;
EXPORT int iaxc_mic_boost_set( int enable ) ;
EXPORT char* iaxc_version(char *ver);

/* Fine tune jitterbuffer control */
EXPORT void iaxc_set_jb_target_extra( long value );

/* application-defined networking; give substiture sendto and recvfrom functions,
 * must be called before iaxc_initialize! */
EXPORT void iaxc_set_networking(iaxc_sendto_t st, iaxc_recvfrom_t rf) ;

/* wrapper for libiax2 get_netstats */
EXPORT int iaxc_get_netstats(int call, int *rtt, struct iaxc_netstat *local, struct iaxc_netstat *remote);

#define IAXC_AD_INPUT           (1<<0)
#define IAXC_AD_OUTPUT          (1<<1)
#define IAXC_AD_RING            (1<<2)
#define IAXC_AD_INPUT_DEFAULT   (1<<3)
#define IAXC_AD_OUTPUT_DEFAULT  (1<<4)
#define IAXC_AD_RING_DEFAULT    (1<<5)

struct iaxc_audio_device {
	char *name;             /* name of the device */
	long capabilities;      /* flags, defined above */
	int devID;              /* driver-specific ID */
};

/* Get audio device information:
 * 	**devs: a pointer to an array of device structures, as declared above.  function
 * 	will give you a pointer to the proper array, which will be valid as long as iaxc is
 * 	initialized.
 *
 * 	*nDevs: a pointer to an int, to which the count of devices in the array devs will be
 * 	written
 *
 * 	*input, *output, *ring: the currently selected devices for input, output, ring will
 * 	be written to the int pointed to by these pointers.
 */
EXPORT int iaxc_audio_devices_get(struct iaxc_audio_device **devs, int *nDevs, int *input, int *output, int *ring); 
EXPORT int iaxc_audio_devices_set(int input, int output, int ring);

EXPORT double iaxc_input_level_get();
EXPORT double iaxc_output_level_get();
EXPORT int iaxc_input_level_set(double level);
EXPORT int iaxc_output_level_set(double level);


struct iaxc_sound {
	short 	*data;          /* sound data */
	long 	len;            /* length of sample */
	int 	malloced;	/* should the library free() the data after it is played? */
	int	channel;	/* 0 for outputSelected, 1 for ringSelected */
	int 	repeat;      	/* number of times to repeat (-1 = infinite) */
	long	pos;		/* internal use: current play position */
	int 	id;		/* internal use: sound ID */
	struct iaxc_sound *next; /* internal use: next in list */
};

/* play a sound.  sound = an iaxc_sound structure, ring: 0: play through output device; 1: play through "ring" device */
EXPORT int iaxc_play_sound(struct iaxc_sound *sound, int ring);

/* stop sound with ID "id" */
EXPORT int iaxc_stop_sound(int id);


#define IAXC_FILTER_DENOISE 	(1<<0)
#define IAXC_FILTER_AGC 	(1<<1)
#define IAXC_FILTER_ECHO 	(1<<2)
#define IAXC_FILTER_AAGC 	(1<<3) /* Analog (mixer-based) AGC */
#define IAXC_FILTER_CN 		(1<<4) /* Send CN frames when silence detected */
EXPORT int iaxc_get_filters(void);
EXPORT void iaxc_set_filters(int filters);
EXPORT int iaxc_set_files(FILE *input, FILE *output);

/* speex specific codec settings */
/* a good choice is (1,-1,-1,0,8000,3): 8kbps ABR */
/* Decode options:
	* decode_enhance: 1/0  perceptual enhancement for decoder 
	* quality: Generally, set either quality (0-9) or bitrate.  
	*    -1 for "default"
	* bitrate: in kbps.  Applies to CBR only; -1 for default.  
	*    (overrides "quality" for CBR mode)
	* vbr: Variable bitrate mode:  0/1
	* abr mode/rate:  0 for not ABR, bitrate for ABR mode
	* complexity:  algorithmic complexity.  Think -N for gzip.
	*    Higher numbers take more CPU for better quality.  3 is
	*    default and good choice.
*/
EXPORT void iaxc_set_speex_settings(int decode_enhance, float quality, int bitrate, int vbr, int abr, int complexity);

/* set/get video mode */
#define IAXC_VIDEO_MODE_NONE 			0  /* don't send video at all */
#define IAXC_VIDEO_MODE_ACTIVE 			1  /* send video */
#define IAXC_VIDEO_MODE_PREVIEW_RAW 		2  /* send video, and show raw preview */
#define IAXC_VIDEO_MODE_PREVIEW_ENCODED  	3  /* send video, and show encoded preview */
EXPORT int iaxc_video_mode_set(int mode);
EXPORT int iaxc_video_mode_get();

/* set allowed/preferred video encodings */
EXPORT void iaxc_video_format_set(int preferred, int allowed, int framerate, int bitrate, int width, int height);


#ifdef __cplusplus
}
#endif

#endif
