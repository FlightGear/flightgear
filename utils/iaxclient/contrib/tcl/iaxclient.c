/* 
 * iaxclient.c
 *
 *  Tcl interface to the iax2 client lib.
 *
 * Copyright (c) 2006 Mats Bengtsson
 * Copyright (c) 2006 Antonio Cano Damas (iGesTec)
 *
 * BSD-style license
 *
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* On my 10.2.8 box I need this fix. */
#if TARGET_API_MAC_CARBON
    typedef int socklen_t;
#endif
#include "iaxclient.h"

/* thread support */
#if defined(WIN32)  ||  defined(_WIN32_WCE)
// empty
#else
#	include <pthread.h>
#endif

#if TARGET_API_MAC_CARBON
#	include <Tcl/tcl.h>
#else
#	include "tcl.h"
#endif

#define PACKAGE_VERSION "0.2"

/*
 * There are two methods for sending scripts from the thread where the
 * iax callback is happening, which can be the main thread or any other
 * thread. One method uses polling of a static string, while the preferred
 * method uses thread events.
 */
#define USE_THREAD_EVENTS_METHOD 1

#define BETWEEN(a,x,b)     ((x) < (a) ? a :((x) > (b) ? b : x)) 
#define MAX_LINES 1

#ifndef M_PI
#define	M_PI		3.14159265358979323846	/* pi */
#endif

/*
 * Mapper from defines to string values.
 */

struct Mapper {
    int i;
    char *s;
};

struct Mapper mapEvent[] = {
    {IAXC_EVENT_TEXT,			"text"			},
    {IAXC_EVENT_LEVELS,      	"levels"		},
    {IAXC_EVENT_STATE,			"state"			},
    {IAXC_EVENT_NETSTAT,		"netstat" 		},
    {IAXC_EVENT_URL,			"url"			},
    {IAXC_EVENT_VIDEO,			"video"			},
    {IAXC_EVENT_REGISTRATION,	"registration"	},
    {0,							NULL			}
};

struct Mapper mapRegistration[] = {
    {IAXC_REGISTRATION_REPLY_ACK,		"ack"		},
    {IAXC_REGISTRATION_REPLY_REJ,		"rej"		},
    {IAXC_REGISTRATION_REPLY_TIMEOUT,	"timeout"	},
    {0,									NULL		}
};

struct Mapper mapCallState[] = {
    {IAXC_CALL_STATE_FREE,		"free"		},		/* = 0 beware! */
    {IAXC_CALL_STATE_ACTIVE,	"active"	},
    {IAXC_CALL_STATE_OUTGOING,	"outgoing"	},
    {IAXC_CALL_STATE_RINGING,	"ringing"	},
    {IAXC_CALL_STATE_COMPLETE,	"complete"	},
    {IAXC_CALL_STATE_SELECTED,	"selected"	},
    {IAXC_CALL_STATE_BUSY,		"busy"		},
    {IAXC_CALL_STATE_TRANSFER,	"transfer"	},
    {0,							NULL		}
};

struct Mapper mapFormat[] = {
    {IAXC_FORMAT_G723_1,	"G723_1"	},
    {IAXC_FORMAT_GSM,		"GSM"		},
    {IAXC_FORMAT_ULAW,		"ULAW"		},
    {IAXC_FORMAT_ALAW,		"ALAW"		},
    {IAXC_FORMAT_G726,		"G726"		},
    {IAXC_FORMAT_ADPCM,		"ADPCM"		},
    {IAXC_FORMAT_SLINEAR,	"SLINEAR"	},
    {IAXC_FORMAT_LPC10,		"LPC10"		},
    {IAXC_FORMAT_G729A,		"G729A"		},
    {IAXC_FORMAT_SPEEX,		"SPEEX"		},
    {IAXC_FORMAT_ILBC,		"ILBC"		},
    {IAXC_FORMAT_JPEG,      "JPEG"      },
    {IAXC_FORMAT_PNG,       "PNG"       },
    {IAXC_FORMAT_H261,      "H.261"     },
    {IAXC_FORMAT_H263,      "H.263"     },
    {IAXC_FORMAT_H263_PLUS, "H.263+"    },
    {IAXC_FORMAT_MPEG4,     "H264"      },
    {IAXC_FORMAT_THEORA,    "Theora"    },
    {0,						NULL		}
};

static int		AnswerObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int 		CallerIDObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		ApplyFiltersObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		SetDevicesObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		DevicesObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		DialObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		ChangelineObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		GetPortObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		FormatsObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		HangUpObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int      HoldObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		InfoObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int		LevelObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int		NotifyObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int		PlayToneObjCmd( ClientData clientData,	Tcl_Interp *interp,	int objc, Tcl_Obj *CONST objv[] );
static int 		RegisterObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		RejectObjCmd( ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[] );
static int		RingStopObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int		RingStartObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		SendTextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		SendToneObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int 		StateObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int      TransferObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int      UnholdObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int      UnregisterObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);
static int      ToneInitObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

static void		PollEvents(ClientData clientData);
static void		ExitHandler(ClientData clientData);
static int 		IAXCCallback(iaxc_event e);

static void 	EventText(struct iaxc_ev_text text);
static void 	EventLevels(struct iaxc_ev_levels levels);
static void 	EventState(struct iaxc_ev_call_state state);
static void 	EventNetStats(struct iaxc_ev_netstats netstats);
static void 	EventRegistration(struct iaxc_ev_registration reg);
static void 	EventUrl(struct iaxc_ev_url eurl);
static void 	EventVideo(struct iaxc_ev_video video);
static void 	EventUnknown(int type);

static int 		GetMapperIntFromString(Tcl_Interp *interp, struct Mapper mapper[], char *str, char *errStr, int *iPtr);
static Tcl_Obj* NewMapFlagStateObj(struct Mapper mapper[], int flag);
static Tcl_Obj* NewMapStateObj(int flag);
static char*	GetMapIntString(struct Mapper mapper[], int state);
static char* 	GetMapFlagDString(Tcl_DString *dsPtr, struct Mapper mapper[], int flag);
static char*	GetMapStateDString(Tcl_DString *dsPtr, int flag);

#if 0	// Just as a backup
static void 	EvalScriptAsync(Tcl_Obj *cmdObj);
static Tcl_Obj* NewMapIntStateObj(struct Mapper mapper[], int state);
#endif

extern void tone_dtmf(char tone, int samples, double vol, short *data);

#if USE_THREAD_EVENTS_METHOD
extern void XThread_RegisterThread(Tcl_Interp *interp);
extern void XThread_UnregisterThread();
extern void XThread_EvalInThread(Tcl_ThreadId threadId, const char *script, int flags);
#endif

/* 
 * These arrays MUST be kept in sync!
 */
static Tcl_Obj *sNotifyRecord[] = {
    NULL,		/* Text 		*/
    NULL,		/* Levels 		*/
    NULL,		/* State 		*/
    NULL,		/* NetStats 	*/
    NULL,		/* Url 			*/
    NULL,		/* Video 		*/
    NULL		/* Registration */
};

enum {
    kNotifyCmdText  		= 0L, 
    kNotifyCmdLevels,
    kNotifyCmdState,
    kNotifyCmdNetStats,
    kNotifyCmdUrl,
    kNotifyCmdVideo,
    kNotifyCmdRegistration
};

CONST char *notifyCmd[] = {
    "<Text>", 
    "<Levels>",
    "<State>", 
    "<NetStats>",
    "<Url>",
    "<Video>",
    "<Registration>",
    (char *) NULL
};

CONST char *devicesCmd[] = {
    "input", 
    "output", 
    "ring",
    (char *) NULL
};
enum {
    kIaxcInput                = 0L, 
    kIaxcOutput,
    kIaxcRing
};


static Tcl_Interp *sInterp = NULL;
static char *dtmf_tones = "123A456B789C*0#D";	/* valid touch tones */
static int sLastState = IAXC_CALL_STATE_FREE;	/* Keep cache of the state (bit) integer */

/* Storage for the ring tone. */
static struct iaxc_sound ringTone;

#define kNotifyCallbackCacheSize 4096
static char asyncCallbackCache[kNotifyCallbackCacheSize];

static Tcl_TimerToken sTimerToken = NULL;
static Tcl_ThreadId sMainThreadID;
#define kTimerPollEventsMillis 100

/* os-dependent macros, etc. From iaxclient. */
#if defined(WIN32)  ||  defined(_WIN32_WCE)
#define MUTEX CRITICAL_SECTION
#define MUTEXINIT(m) InitializeCriticalSection(m)
#define MUTEXLOCK(m) EnterCriticalSection(m)
#define MUTEXUNLOCK(m) LeaveCriticalSection(m)
#define MUTEXDESTROY(m) DeleteCriticalSection(m)
#else
#define MUTEX pthread_mutex_t
#define MUTEXINIT(m) pthread_mutex_init(m, NULL) //TODO: check error
#define MUTEXLOCK(m) pthread_mutex_lock(m)
#define MUTEXUNLOCK(m) pthread_mutex_unlock(m)
#define MUTEXDESTROY(m) pthread_mutex_destroy(m)
#endif

static MUTEX notifyRecordMutex;
static MUTEX asyncCallbackMutex;


/*
 *----------------------------------------------------------------------
 *
 * Iaxclient_Init --
 *
 *	The package initialization procedure.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *   Tcl commands created
 *----------------------------------------------------------------------
 */


DLLEXPORT int 
Iaxclient_Init(
    Tcl_Interp *interp)		/* Tcl interpreter. */
{
    int			i;
    typedef struct { 
        char                *cmdname;
        Tcl_ObjCmdProc      *proc;
        Tcl_CmdDeleteProc   *delproc;
    } CmdProcStruct;
	CmdProcStruct cmdList[] = {
		{"iaxclient::answer", AnswerObjCmd, NULL},
		{"iaxclient::applyfilters", ApplyFiltersObjCmd, NULL},
		{"iaxclient::callerid", CallerIDObjCmd, NULL},
		{"iaxclient::changeline", ChangelineObjCmd, NULL},
		{"iaxclient::devices", DevicesObjCmd, NULL},
		{"iaxclient::dial", DialObjCmd, NULL},
		{"iaxclient::formats", FormatsObjCmd, NULL},
		{"iaxclient::getport", GetPortObjCmd, NULL},
		{"iaxclient::hangup", HangUpObjCmd, NULL},
        {"iaxclient::hold", HoldObjCmd, NULL},
		{"iaxclient::info", InfoObjCmd, NULL},
		{"iaxclient::level", LevelObjCmd, NULL},
		{"iaxclient::notify", NotifyObjCmd, NULL},
		{"iaxclient::playtone", PlayToneObjCmd, NULL},
		{"iaxclient::register", RegisterObjCmd, NULL},
		{"iaxclient::reject", RejectObjCmd, NULL},
		{"iaxclient::ringstart", RingStartObjCmd, NULL},
		{"iaxclient::ringstop", RingStopObjCmd, NULL},
		{"iaxclient::sendtext", SendTextObjCmd, NULL},
		{"iaxclient::sendtone", SendToneObjCmd, NULL},
		{"iaxclient::setdevices", SetDevicesObjCmd, NULL},
		{"iaxclient::state", StateObjCmd, NULL},
		{"iaxclient::toneinit", ToneInitObjCmd, NULL},
        {"iaxclient::transfer", TransferObjCmd, NULL},
        {"iaxclient::unhold", UnholdObjCmd, NULL},
		{"iaxclient::unregister", UnregisterObjCmd, NULL},
		{NULL, NULL, NULL}
	};

    if (sInterp) {
		Tcl_SetObjResult( interp,  
			    Tcl_NewStringObj( "only one interpreter allowed :-(", -1 ));
		return TCL_ERROR;
    }
    sInterp = interp;
    if (Tcl_InitStubs( interp, "8.1", 0 ) == NULL) {
        return TCL_ERROR;
    }
    // Set Preferred UDP Port: 
    // 0: Use the default port (4569)    
    iaxc_set_preferred_source_udp_port(0);
    if (iaxc_initialize(AUDIO_INTERNAL_PA, MAX_LINES)) {
		Tcl_SetObjResult( interp,  
			    Tcl_NewStringObj( "cannot initialize iaxclient!", -1 ));
		return TCL_ERROR;
    }

	MUTEXINIT(&notifyRecordMutex);
	MUTEXINIT(&asyncCallbackMutex);

    iaxc_set_silence_threshold(-99.0); /* the default */
    iaxc_set_audio_output(0);	/* the default */
    iaxc_set_event_callback(IAXCCallback); 
    iaxc_start_processing_thread();
    ringTone.data = NULL;

    Tcl_CreateExitHandler( ExitHandler, (ClientData) NULL );

    i = 0 ;
    while(cmdList[i].cmdname) {
        Tcl_CreateObjCommand( interp, cmdList[i].cmdname, cmdList[i].proc,
                (ClientData) NULL, cmdList[i].delproc);
        i++ ;
    }
    sMainThreadID = Tcl_GetCurrentThread();

#if USE_THREAD_EVENTS_METHOD    
    XThread_RegisterThread(interp);
#else
    sTimerToken = Tcl_CreateTimerHandler(kTimerPollEventsMillis, PollEvents, NULL); 
#endif
    return Tcl_PkgProvide( interp, "iaxclient", PACKAGE_VERSION );
}

/*
 *----------------------------------------------------------------------
 *
 * Iaxclient_SafeInit --
 *
 *	The package initialization procedure for safe interpreters.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side Effects:
 *   Tcl commands created
 *----------------------------------------------------------------------
 */

DLLEXPORT int Iaxclient_SafeInit(Tcl_Interp *interp )
{
	return Iaxclient_Init( interp );
}

static int AnswerObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{	
    int result = TCL_OK;
	int	line = 0;

	/* We tell to the AnswerObj the Line where is incoming call, by default use line 0 */
	if (objc == 2) {
		if (Tcl_GetIntFromObj(interp, objv[1], &line) != TCL_OK) {
			result = TCL_ERROR;
		} else if (line < 0 || line > MAX_LINES) {
			// @@@ guess
			Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient::answer, callNo must be > 0 and < 9", -1));        
			result = TCL_ERROR;
		}
	}
	iaxc_answer_call(line);
	iaxc_select_call(line);

    return result;
}

static int CallerIDObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;

    if (objc == 3) {
        char *name;
        char *num;
        name = Tcl_GetStringFromObj(objv[1], NULL);
        num = Tcl_GetStringFromObj(objv[2], NULL);
        iaxc_set_callerid(name, num);
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "cid_name cid_number" );
        result = TCL_ERROR;
    }
    return result;
}

static int ChangelineObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;

    if (objc == 2) {
        int	line;
        if (Tcl_GetIntFromObj(interp, objv[1], &line) != TCL_OK) {
            result = TCL_ERROR;
        } else if (line < 0 || line > MAX_LINES) {
            // @@@ guess
            Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient::changeline, callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        } else {
            iaxc_select_call(line);
        }
    } else {
		Tcl_WrongNumArgs( interp, 1, objv, "newCallNo" );
		result = TCL_ERROR;
    }
    return result;
}

static int ApplyFiltersObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    // Clear filters
    int flag = ~(IAXC_FILTER_AGC | IAXC_FILTER_AAGC | IAXC_FILTER_CN |
                 IAXC_FILTER_DENOISE | IAXC_FILTER_ECHO);
	int AGC;
	int AAGC;
	int CN;
	int NoiseReduce;
	int EchoCancel;
    int result = TCL_OK;
	
    if (objc == 6) {
        if (Tcl_GetIntFromObj(interp, objv[1], &AGC) != TCL_OK) {
            result = TCL_ERROR;
        } 
        if (Tcl_GetIntFromObj(interp, objv[2], &AAGC) != TCL_OK) {
            result = TCL_ERROR;
        } 
        if (Tcl_GetIntFromObj(interp, objv[3], &CN) != TCL_OK) {
            result = TCL_ERROR;
        } 
        if (Tcl_GetIntFromObj(interp, objv[4], &NoiseReduce) != TCL_OK) {
            result = TCL_ERROR;
        } 
        if (Tcl_GetIntFromObj(interp, objv[5], &EchoCancel) != TCL_OK) {
            result = TCL_ERROR;
        } 

		//Clear Filters before apply new ones
		iaxc_set_filters(iaxc_get_filters() & flag);
		flag = 0;
	
		if(AGC)
			flag = IAXC_FILTER_AGC;

		if(AAGC)
			flag = IAXC_FILTER_AAGC;

		if(CN)
			flag = IAXC_FILTER_CN;

		if(NoiseReduce)
			flag |= IAXC_FILTER_DENOISE;

		if(EchoCancel)
			flag |= IAXC_FILTER_ECHO;

		iaxc_set_filters(iaxc_get_filters() | flag);
	}  else {
		Tcl_WrongNumArgs( interp, 1, objv, "AGC AAGC CN NoiseReduce EchoCancel" );
		result = TCL_ERROR;
    }
	return result;
}

static int SetDevicesObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	int	input = 0;
	int output = 0;
    int ring = 0;
    int index;
    int value;
    int ndevs;						/* audio dedvice count */
    struct iaxc_audio_device *devs; /* audio devices */
		
    if (objc == 3) {
        if (Tcl_GetIndexFromObj( interp, objv[1], devicesCmd, "command", TCL_EXACT, &index )
                != TCL_OK ) {
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[2], &value) != TCL_OK) {
            return TCL_ERROR;
        } 
        iaxc_audio_devices_get(&devs, &ndevs, &input, &output, &ring);
        switch (index) {
            case kIaxcInput: input = value; break;
            case kIaxcOutput: output = value; break;
            case kIaxcRing: ring = value; break;
        }
        iaxc_audio_devices_set(input, output, ring);
    } else {
		Tcl_WrongNumArgs( interp, 1, objv, "type deviceid" );
		return TCL_ERROR;
    }
    return TCL_OK;
}

static int DevicesObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    struct iaxc_audio_device *devs; /* audio devices */
    int ndevs = 0;						/* audio dedvice count */
    int input, output, ring;		/* audio device id's */
    int current = 0, i;
    int flag;
    int len;
	char *str;
	Tcl_Obj *listObj, *subListObj;

    static int mapFlag[32];
    mapFlag[kIaxcInput]  = IAXC_AD_INPUT;
    mapFlag[kIaxcOutput] = IAXC_AD_OUTPUT;
    mapFlag[kIaxcRing]   = IAXC_AD_RING;

    if (objc != 2 && objc != 3) {
        Tcl_WrongNumArgs( interp, 1, objv, "type ?-current?" );
	    return TCL_ERROR;
    }
	if (Tcl_GetIndexFromObj( interp, objv[1], devicesCmd, "command", TCL_EXACT, &index )
	        != TCL_OK ) {
	    return TCL_ERROR;
	}
    if (objc == 3) {
		str = Tcl_GetStringFromObj(objv[2], &len);
        if (strncmp(str, "-current", len)) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("Usage: iaxclient::devices type ?-current?", -1));
            return TCL_ERROR;
        }
    }
    flag = mapFlag[index];
    iaxc_audio_devices_get(&devs, &ndevs, &input, &output, &ring);

    listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
    if (objc == 3) {
        switch (index) {
            case kIaxcInput: current = input; break;
            case kIaxcOutput: current = output; break;
            case kIaxcRing: current = ring; break;
        }
        for (i = 0; i < ndevs; i++) {
            if ((devs[i].capabilities & flag) && (devs[i].devID == current)) {
                Tcl_ListObjAppendElement( interp, listObj, Tcl_NewStringObj(devs[i].name, -1) );				
                Tcl_ListObjAppendElement( interp, listObj, Tcl_NewIntObj(devs[i].devID) );
                break;			
            }
        }
        Tcl_SetObjResult( interp, listObj );
    } else {
        for (i = 0; i < ndevs; i++) {
            if (devs[i].capabilities & flag) {
                subListObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
                Tcl_ListObjAppendElement( interp, subListObj, Tcl_NewStringObj(devs[i].name, -1) );				
                Tcl_ListObjAppendElement( interp, subListObj, Tcl_NewIntObj(devs[i].devID) );				
                Tcl_ListObjAppendElement( interp, listObj, subListObj );
            }
        }
        Tcl_SetObjResult( interp, listObj );
    }
    return TCL_OK;
}

static int DialObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;
	char *num = NULL;
	int line;
		
	if (objc >= 2 ) {
		num = Tcl_GetStringFromObj(objv[1], NULL);
	} else {
		Tcl_WrongNumArgs( interp, 1, objv, "user:pass@server/nnn callNo" );
		result = TCL_ERROR;
    }
	
    if (objc == 2) {
			line = iaxc_selected_call();
	}

    if (objc == 3) {
        if (Tcl_GetIntFromObj(interp, objv[2], &line) != TCL_OK) {
            result = TCL_ERROR;
        } else if (line < 0 || line > MAX_LINES) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient:dial, callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        }
    } 

	if (result == TCL_OK) {
		iaxc_call(num);
		iaxc_select_call(line);
	}
    return result;	
}

static int FormatsObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    char *codec;
    int value;
    int result = TCL_OK;

    if (objc == 2) {
        codec = Tcl_GetStringFromObj(objv[1], NULL);
        result = GetMapperIntFromString(interp, mapFormat, codec, "iaxclient:formats, codec must be: ", &value);
        if (result == TCL_OK) {
             iaxc_set_formats(value,value);
        }
    } else {
        Tcl_WrongNumArgs(interp, 1, objv, "codec");
        result = TCL_ERROR;
    }
    return result;
}

static int GetPortObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{	
    Tcl_SetObjResult(interp, Tcl_NewIntObj(iaxc_get_bind_port()));
    return TCL_OK;
}

static int HangUpObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    iaxc_dump_call();
    return TCL_OK;
}

static int HoldObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;
	int selected = 0;

    if (objc == 1) {
			selected = iaxc_selected_call();
	}

    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &selected) != TCL_OK) {
            result = TCL_ERROR;
        } else if (selected < 0 || selected > MAX_LINES) {
            // @@@ guess
            Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient:hold, callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        }
	}
	
	if ( result == TCL_OK ) {
		iaxc_quelch(selected, 1);
		iaxc_select_call(-1);
	}
	
    return result;
}

static int InfoObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
/*
char* iaxc_version(char *ver);
EXPORT void iaxc_set_min_outgoing_framesize(int samples);
EXPORT void iaxc_set_silence_threshold(double thr);
*/
    return TCL_ERROR;
}

static int LevelObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    double level;
    CONST char *levelCmd[] = {
        "input", 
        "output", 
        (char *) NULL
    };
    enum {
        kIaxcLevelInput                = 0L, 
        kIaxcLevelOutput
    };

    if (objc != 2 && objc != 3) {
        Tcl_WrongNumArgs( interp, 1, objv, "type ?value?" );
	    return TCL_ERROR;
    }
	if (Tcl_GetIndexFromObj( interp, objv[1], levelCmd, "command", TCL_EXACT, &index )
	        != TCL_OK ) {
	    return TCL_ERROR;
	}
    if (index == kIaxcLevelInput) {
        if (objc == 3) {
            if (TCL_OK != Tcl_GetDoubleFromObj(interp, objv[2], &level)) {
                return TCL_ERROR;
            }
            iaxc_input_level_set(BETWEEN(0.0, level, 1.0));
        }
        level = iaxc_input_level_get();
        Tcl_SetObjResult( interp, Tcl_NewDoubleObj(level) );
    } else if (index == kIaxcLevelOutput) {
        if (objc == 3) {
            if (TCL_OK != Tcl_GetDoubleFromObj(interp, objv[2], &level)) {
                return TCL_ERROR;
            }
            iaxc_output_level_set(BETWEEN(0.0, level, 1.0));
        }
        level = iaxc_output_level_get();
        Tcl_SetObjResult( interp, Tcl_NewDoubleObj(level) );
    }
    return TCL_OK;
}

/*
 * This is always called from the Tcl main thread.
 */

static int NotifyObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int index;
    int len;

    if (objc != 2 && objc != 3) {
        Tcl_WrongNumArgs( interp, 1, objv, "eventType ?tclProc?" );
	    return TCL_ERROR;
    }
	if (Tcl_GetIndexFromObj( interp, objv[1], notifyCmd, "command", TCL_EXACT, &index )
	        != TCL_OK ) {
	    return TCL_ERROR;
	}

    MUTEXLOCK(&notifyRecordMutex);
    if (objc == 3) {
        if (sNotifyRecord[index]) {
            Tcl_DecrRefCount(sNotifyRecord[index]);
            sNotifyRecord[index] = NULL;
        }
        Tcl_GetStringFromObj(objv[2], &len);
        if (len > 0) {
            /*
             * Most command procedures do not have to be concerned about reference counting 
             * since they use an object's value immediately and don't retain a pointer to 
             * the object after they return. However, if they do retain a pointer to an 
             * object in a data structure, they must be careful to increment its reference 
             * count since the retained pointer is a new reference. 
             */
            //sNotifyRecord[index] = Tcl_DuplicateObj(objv[2]);
            sNotifyRecord[index] = objv[2];
            Tcl_IncrRefCount(sNotifyRecord[index]);
        }
    }
    if (sNotifyRecord[index]) {
        Tcl_SetObjResult(interp, sNotifyRecord[index]);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
    return TCL_OK;
}

static int PlayToneObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    struct iaxc_sound sound; /* sound to play */
    char tone;
    double vol = 100.0;
    int len = 2000;
    short *buff;
    int result = TCL_OK;

    /* Must get dynamic memory for this! */
    buff = (short *)calloc(len, sizeof(short));    

    memset(&sound, 0, sizeof(sound));
    sound.data = buff;
    sound.len = len;
    sound.malloced = 1;
    sound.repeat = 0;

    if (objc == 2) {
        char *s;
        int tlen;
        s = Tcl_GetStringFromObj(objv[1], &tlen);
        if (tlen != 1) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("must be a ring tone", -1));
            return TCL_ERROR;
        }
        if (!strchr(dtmf_tones, s[0])) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("must be a ring tone", -1));
            return TCL_ERROR;
        }
        tone = s[0];
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "tone" );
        return TCL_ERROR;
    }

    tone_dtmf(tone, 1600, vol, buff);
    tone_dtmf('x', 400, vol, buff+1600);
    iaxc_play_sound(&sound, 0);
    return result;
}

static int RegisterObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	char *user;
	char *pass;
	char *host;
    int result = TCL_OK;
	int session = 0;
    Tcl_Obj *listObj = NULL;

    if (objc == 4) {
        user = Tcl_GetStringFromObj(objv[1], NULL);
        pass = Tcl_GetStringFromObj(objv[2], NULL);
        host = Tcl_GetStringFromObj(objv[3], NULL);
	
        /* @@@ UTF stuff? */
#ifdef TIPIC_LIBS
	    iaxc_register(user, pass, host);
#else
	    session = iaxc_register(user, pass, host);
#endif
		
		/* Return SessionId */
		listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
		Tcl_ListObjAppendElement( interp, listObj, Tcl_NewIntObj( session ) );
		Tcl_SetObjResult(interp, listObj);		
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "user password host" );
        result = TCL_ERROR;
    }
    return result;
}

static int RejectObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;
	int	line = 0;

/* We tell to the AnswerObj the Line where is incoming call */
    if (objc == 1) {
			line = iaxc_selected_call();
	}
	
    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &line) != TCL_OK) {
            result = TCL_ERROR;
        } else if (line < 0 || line > MAX_LINES) {
            // @@@ guess
            Tcl_SetObjResult(interp, Tcl_NewStringObj("callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        }
    }

	if ( result == TCL_OK ) {
		iaxc_select_call(line);
		iaxc_reject_call();
	}
    return result;
}

static int RingStartObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
	int ringdev;

    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &ringdev) != TCL_OK) {
            return TCL_ERROR;
        }
		iaxc_play_sound(&ringTone, ringdev);
	}  else {
        Tcl_WrongNumArgs( interp, 1, objv, "ringdev" );
        return TCL_ERROR;
    }
	
	return TCL_OK;
}

static int RingStopObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    iaxc_stop_sound(ringTone.id);
    return TCL_OK;
}

static int SendTextObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    if (objc == 2) {
        char *text;
        /* Encoding ? */
        text = Tcl_GetStringFromObj(objv[1], NULL);
        iaxc_send_text(text);
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "text" );
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int SendToneObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    if (objc == 2) {
        char *s;
        int len;
        s = Tcl_GetStringFromObj(objv[1], &len);
        if (len != 1) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("must be a ring tone", -1));
            return TCL_ERROR;
        }
        if (!strchr(dtmf_tones, s[0])) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("must be a ring tone", -1));
            return TCL_ERROR;
        }
        iaxc_send_dtmf(s[0]);
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "tone" );
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int StateObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    if (objc == 1) {
        Tcl_SetObjResult(interp, NewMapStateObj(sLastState));
        return TCL_OK;
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, NULL );
        return TCL_ERROR;
    }
}

static int ToneInitObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) 
{
	int F1;
	int F2;
	int Dur;
	int Len;
	int Repeat;
    int i;
	
    int result = TCL_OK;

    if (objc == 6) {
        if (Tcl_GetIntFromObj(interp, objv[1], &F1) != TCL_OK) {
            result = TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[2], &F2) != TCL_OK) {
            result = TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[3], &Dur) != TCL_OK) {
            result = TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[4], &Len) != TCL_OK) {
            result = TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[5], &Repeat) != TCL_OK) {
            result = TCL_ERROR;
        }
	} else {
        Tcl_WrongNumArgs( interp, 1, objv, "F1 F2 Duration Length Repeat" );
        result = TCL_ERROR;
    }
	
	if ( result == TCL_OK ) {	
        if (ringTone.data) {
            iaxc_stop_sound(ringTone.id);
            free(ringTone.data);
        }

		// clear tone structures. (otherwise we free un-allocated memory in LoadTone)
		memset(&ringTone, 0, sizeof(ringTone));

		ringTone.len  = Len;
		ringTone.data = (short *)calloc(ringTone.len , sizeof(short));

		for(i = 0; i < Dur; i++ ) {
			ringTone.data[i] = (short)(0x7fff*0.4*sin((double)i*F1*M_PI/8000))
						+ (short)(0x7fff*0.4*sin((double)i*F2*M_PI/8000));
		}
		ringTone.repeat = Repeat;
	}
	
	return result;
}

static int TransferObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	int	selected;
	char *num = NULL;
    int result = TCL_OK;

    if (objc != 2 && objc != 3) {
        Tcl_WrongNumArgs( interp, 1, objv, "dstnumber ?callNo?" );
	    return TCL_ERROR;
    }
	
    if (objc >= 2) {
        num = Tcl_GetStringFromObj(objv[1], NULL);    
    }

	if (objc == 3) {
        if ( Tcl_GetIntFromObj(interp, objv[2], &selected) != TCL_OK ) {
            result = TCL_ERROR;
        } else if (selected < 0 || selected > MAX_LINES) {
            // @@@ guess
            Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient:transfer, callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        }
	} else {
		selected = iaxc_selected_call();
	}
	
	if ( result == TCL_OK ) {
		iaxc_blind_transfer_call(selected, num);
	}
	
	return result;
}


static int UnholdObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;
	int selected = 0;

    if (objc == 1) {
			selected = iaxc_selected_call();
	}

    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &selected) != TCL_OK) {
            result = TCL_ERROR;
        } else if (selected < 0 || selected > MAX_LINES) {
            // @@@ guess
            Tcl_SetObjResult(interp, Tcl_NewStringObj("iaxclient:unhold, callNo must be > 0 and < 9", -1));        
            result = TCL_ERROR;
        }
	}
	
	if ( result == TCL_OK ) {
		iaxc_unquelch(selected);
		iaxc_select_call(selected);
	}
	
    return result;
}

static int UnregisterObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    int result = TCL_OK;

    if (objc == 2) {
        int id;
        if (Tcl_GetIntFromObj(interp, objv[1], &id) != TCL_OK) {
            result = TCL_ERROR;
        } else {
#ifndef TIPIC_LIBS
            iaxc_unregister(id);
#endif
        }
    } else {
        Tcl_WrongNumArgs( interp, 1, objv, "id" );
        result = TCL_ERROR;
    }
    return result;
}

static void PollEvents(ClientData clientData)
{
    MUTEXLOCK(&asyncCallbackMutex);
    if (strlen(asyncCallbackCache) > 0) {
        Tcl_EvalEx(sInterp, asyncCallbackCache, -1, TCL_EVAL_GLOBAL);
        asyncCallbackCache[0] = '\0';
    }
    sTimerToken = Tcl_CreateTimerHandler(kTimerPollEventsMillis, PollEvents, NULL); 
    MUTEXUNLOCK(&asyncCallbackMutex);
}

static void ExitHandler(ClientData clientData)
{
    if (sTimerToken != NULL) {
        Tcl_DeleteTimerHandler(sTimerToken);
    }
    iaxc_dump_call();
    iaxc_millisleep(1000);
    iaxc_stop_processing_thread();
    iaxc_shutdown();

    MUTEXDESTROY(&notifyRecordMutex);
	MUTEXDESTROY(&asyncCallbackMutex);
}

/*
 * All iax callbacks come here.
 */

static int IAXCCallback(iaxc_event e) 
{
    switch(e.type) {
        case IAXC_EVENT_TEXT:
            EventText(e.ev.text);
            break;
        case IAXC_EVENT_LEVELS:
            EventLevels(e.ev.levels);
            break;
        case IAXC_EVENT_STATE:
            sLastState = e.ev.call.state;
            EventState(e.ev.call);
            break;
        case IAXC_EVENT_NETSTAT:
            EventNetStats(e.ev.netstats);
            break;
        case IAXC_EVENT_URL:
            EventUrl(e.ev.url);
            break;
        case IAXC_EVENT_VIDEO:
            EventVideo(e.ev.video);
            break;
        case IAXC_EVENT_REGISTRATION:
            EventRegistration(e.ev.reg);
            break;
        default:
            EventUnknown(e.type);
            break;
    }
    return 1;
}

/*
 * All these events come mostly from worker threads but can also
 * come from the main thread.
 * NEVER use Tcl_Objs here!
 */

static void EventText(struct iaxc_ev_text text)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdText]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdText], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        switch(text.type) {
            case IAXC_TEXT_TYPE_STATUS:
                Tcl_DStringAppendElement(&ds, "status");
                break;
            case IAXC_TEXT_TYPE_NOTICE:
                Tcl_DStringAppendElement(&ds, "notice");
                break;
            case IAXC_TEXT_TYPE_ERROR:
                Tcl_DStringAppendElement(&ds, "error");
                break;
            default:
                Tcl_DStringAppendElement(&ds, "-");
                break;
        }
        sprintf(buf, "%d", text.callNo);
        Tcl_DStringAppendElement(&ds, buf);
        Tcl_DStringAppendElement(&ds, text.message);
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventLevels(struct iaxc_ev_levels levels)
{
    if (sLastState == IAXC_CALL_STATE_FREE) {
        return;
    }
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdLevels]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdLevels], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(buf, "%.2g", levels.input);
        Tcl_DStringAppendElement(&ds, buf);
        sprintf(buf, "%.2g", levels.output);
        Tcl_DStringAppendElement(&ds, buf);
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventState(struct iaxc_ev_call_state call)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdState]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds, ds2;

        Tcl_DStringInit(&ds);
        Tcl_DStringInit(&ds2);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdState], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(buf, "%d", call.callNo);
        Tcl_DStringAppendElement(&ds, buf);
        Tcl_DStringAppendElement(&ds, GetMapStateDString(&ds2, call.state));        
        Tcl_DStringAppendElement(&ds, GetMapIntString(mapFormat, call.format));
        Tcl_DStringAppendElement(&ds, call.remote);
        Tcl_DStringAppendElement(&ds, call.remote_name);
        Tcl_DStringAppendElement(&ds, call.local);
        Tcl_DStringAppendElement(&ds, call.local_context);        
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
        Tcl_DStringFree(&ds2);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventNetStats(struct iaxc_ev_netstats netstats)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdNetStats]) {
        char *cmd;
        char str[32];
        int i, len;
        struct iaxc_netstat *ptr;
        static char *name[] = {
            "-local:", 
            "-remote:"
        };
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdNetStats], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(str, "-callno %d -rtt %d", netstats.callNo, netstats.rtt);
        Tcl_DStringAppendElement(&ds, str);
        for (i = 0, ptr = &(netstats.local); i <= 1; i++, ptr = &(netstats.remote)) {
            strcpy(str, name[i]);
            strcat(str, "jitter");
            Tcl_DStringAppendElement(&ds, str);
            sprintf(str, "%d", ptr->jitter);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "losspct");
            sprintf(str, "%d", ptr->losspct);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "losscnt");
            sprintf(str, "%d", ptr->losscnt);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "packets");
            sprintf(str, "%d", ptr->packets);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "delay");
            sprintf(str, "%d", ptr->delay);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "dropped");
            sprintf(str, "%d", ptr->dropped);
            Tcl_DStringAppendElement(&ds, str);
            strcpy(str, name[i]);
            strcat(str, "ooo");
            sprintf(str, "%d", ptr->ooo);
            Tcl_DStringAppendElement(&ds, str);
        }
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventUrl(struct iaxc_ev_url eurl)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdUrl]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdUrl], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(buf, "%d", eurl.callNo);
        Tcl_DStringAppendElement(&ds, buf);
        sprintf(buf, "%d", eurl.type);
        Tcl_DStringAppendElement(&ds, buf);
        Tcl_DStringAppendElement(&ds, eurl.url);
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventVideo(struct iaxc_ev_video video)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdVideo]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdVideo], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(buf, "%d", video.callNo);
        Tcl_DStringAppendElement(&ds, buf);
        Tcl_DStringAppendElement(&ds, GetMapIntString(mapFormat, video.format));
        sprintf(buf, "%d", video.width);
        Tcl_DStringAppendElement(&ds, buf);
        sprintf(buf, "%d", video.height);
        Tcl_DStringAppendElement(&ds, buf);
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventRegistration(struct iaxc_ev_registration reg)
{
    MUTEXLOCK(&notifyRecordMutex);
    if (sNotifyRecord[kNotifyCmdRegistration]) {
        char *cmd;
        char buf[32];
        int len;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        cmd = Tcl_GetStringFromObj(sNotifyRecord[kNotifyCmdRegistration], &len);
        Tcl_DStringAppend(&ds, cmd, len);
        sprintf(buf, "%d", reg.id);
        Tcl_DStringAppendElement(&ds, buf);
        Tcl_DStringAppendElement(&ds, GetMapIntString(mapRegistration, reg.reply));
        sprintf(buf, "%d", reg.msgcount);
        Tcl_DStringAppendElement(&ds, buf);
        XThread_EvalInThread(sMainThreadID, Tcl_DStringValue(&ds), 0);
        Tcl_DStringFree(&ds);
    }
    MUTEXUNLOCK(&notifyRecordMutex);
}

static void EventUnknown(int type)
{
    // empty
}

#if 0
static void EvalScriptAsync(Tcl_Obj *cmdObj)
{
    char *script;
    int len;

    script = Tcl_GetStringFromObj(cmdObj, &len);

#if USE_THREAD_EVENTS_METHOD
    XThread_EvalInThread(sMainThreadID, script, 0);
#else
    MUTEXLOCK(&asyncCallbackMutex);

    /* Do not add commands that do not fit. */
    if (strlen(asyncCallbackCache) + len < kNotifyCallbackCacheSize - 2) {
        strcat(asyncCallbackCache, script);
        strcat(asyncCallbackCache, "\n");
    }
    MUTEXUNLOCK(&asyncCallbackMutex);
#endif
}
#endif

static int GetMapperIntFromString(Tcl_Interp *interp, struct Mapper mapper[], char *str, char *errStr, int *iPtr)
{
    int result = TCL_ERROR;
    struct Mapper *m;

    for (m = mapper; m->s != NULL; m++) {
        if (!strcmp(m->s, str)) {
            *iPtr = m->i;
            result = TCL_OK;
            break;
        }
    }
    if (result == TCL_ERROR) {
        Tcl_Obj *resObj = Tcl_NewStringObj(errStr, -1 );
        for (m = mapper; m->s != NULL; m++) {
            Tcl_AppendStringsToObj(resObj, m->s, (char *)NULL);
            if ((m+1)->s != NULL) {
                Tcl_AppendStringsToObj(resObj, ", ", (char *)NULL);
            }
        }
        Tcl_SetObjResult(interp, resObj);
    }
    return result;
}

#if 0
static Tcl_Obj *NewMapIntStateObj(struct Mapper mapper[], int state)
{
    struct Mapper *m;
    Tcl_Obj *obj = NULL;

    for (m = mapper; m->s != NULL; m++) {
        if (m->i == state) {
            obj = Tcl_NewStringObj(m->s, -1);
            break;			
        }
    }
    return obj;
}
#endif

static char *GetMapIntString(struct Mapper mapper[], int state)
{
    struct Mapper *m;    
    for (m = mapper; m->s != NULL; m++) {
        if (m->i == state) {
            return m->s;
        }
    }
    return NULL;
}

static Tcl_Obj *NewMapFlagStateObj(struct Mapper mapper[], int flag)
{
    struct Mapper *m;
    Tcl_Obj *listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );

    for (m = mapper; m->s != NULL; m++) {
        if (m->i & flag) {
            Tcl_ListObjAppendElement(NULL, listObj, Tcl_NewStringObj(m->s, -1));				
        }
    }
    return listObj;
}

static char *GetMapFlagDString(Tcl_DString *dsPtr, struct Mapper mapper[], int flag)
{
    struct Mapper *m;

    /* This fails for flags = 0 */
    for (m = mapper; m->s != NULL; m++) {
        if (m->i & flag) {
            Tcl_DStringAppendElement(dsPtr, m->s);
        }
    }
    return Tcl_DStringValue(dsPtr);
}

static Tcl_Obj *NewMapStateObj(int flag)
{
    if (flag == IAXC_CALL_STATE_FREE) {
		Tcl_Obj *listObj = Tcl_NewListObj( 0, (Tcl_Obj **) NULL );
        Tcl_ListObjAppendElement(NULL, listObj, Tcl_NewStringObj("free", -1));
		return listObj;
    } else {
        return NewMapFlagStateObj(mapCallState, flag);
    }
}

static char *GetMapStateDString(Tcl_DString *dsPtr, int flag)
{
    if (flag == IAXC_CALL_STATE_FREE) {
        return "free";
    } else {
        return GetMapFlagDString(dsPtr, mapCallState, flag);
    }
}


/*
static int ToneLoadObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
	//char *Filename, int Repeat
    wxFFile     fTone;

    if(Filename.IsEmpty())
        return;

    fTone.Open(Filename, _T("r"));

    if(!fTone.IsOpened())
        return;

    // Free old tone, if there was one
    if(tone.data != NULL)
        free(tone.data);

    tone.len  = fTone.Length();
    tone.data = (short *)calloc(tone.len , sizeof(short));
    fTone.Read(&tone.data[0], tone.len);
    fTone.Close();

    tone.repeat = Repeat;
}
*/
