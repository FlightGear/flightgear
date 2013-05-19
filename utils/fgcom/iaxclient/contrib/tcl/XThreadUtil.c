/* 
 * XThreadUtil.c
 *
 * Experiment after suggestion by Zoran Vasiljevic at comp.lang.tcl
 * to send a script from *any* thread to a thread with a Tcl interpreter.
 * This can be useful for extension developers in C that uses a library
 * that executes callbacks in other threads than the main thread where
 * the Tcl interpreter lives.
 * Most code duplicated from the thread package.
 *
 * XThreadUtil stands for "eXtension Thread Utilities".
 *
 * By Mats Bengtsson and Zoran Vasiljevic 2006
 *
 * Ê 1. XThread_RegisterThread 
 * Ê 2. XThread_UnregisterThread 
 * Ê 3. XThread_EvalInThread 
 *
 *   The 1. needs to be called from your master thread, i.e. the 
 *   one you would like to execute the callbacks within.
 *   The 2. needs to be called when your master thread exits (if ever) 
 *   or when you do not want to execute any callbacks.
 *   The 3. needs to be called from your IAX threads to post callbacks 
 *   to the master thread. 
 *
 *   See: http://wiki.tcl.tk/15342
 */

#include <stdio.h>
#include <string.h>

#if TARGET_API_MAC_CARBON
#	include <Tcl/tcl.h>
#else
#	include "tcl.h"
#endif

#ifndef TCL_TSD_INIT
#define TCL_TSD_INIT(keyPtr) \
  (ThreadSpecificData*)Tcl_GetThreadData((keyPtr),sizeof(ThreadSpecificData))
#endif

/*
 * This is used to register the interp for running scripts passed
 * to the thread over the event loop.
 */

typedef struct ThreadSpecificData {
    Tcl_Interp *interp;                   /* Interp to evaluate scripts */
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

/*
 * This is the event used to send commands to other threads.
 */

typedef struct ThreadEvent {
    Tcl_Event event;                      /* Must be first */
    struct ThreadSendData *sendData;      /* See below */
} ThreadEvent;

typedef int  (ThreadSendProc) _ANSI_ARGS_((Tcl_Interp*, ClientData));
typedef void (ThreadSendFree) _ANSI_ARGS_((ClientData));

static ThreadSendProc ThreadSendEval;     /* Does a regular Tcl_Eval */

/*
 * This is used to communicate commands between source and target threads.
 */

typedef struct ThreadSendData {
    ThreadSendProc *execProc;             /* Func to exec in remote thread */
    ClientData      clientData;           /* Ptr to pass to send function */
    ThreadSendFree *freeProc;             /* Function to free client data */
} ThreadSendData;


static void		
ThreadSend		_ANSI_ARGS_((Tcl_ThreadId targetId, ThreadSendData *send));

static int  	
ThreadEventProc	_ANSI_ARGS_((Tcl_Event *evPtr, int mask));

static void 	
ThreadFreeProc	_ANSI_ARGS_((ClientData clientData));

/*
 *----------------------------------------------------------------------
 *
 * XThread_RegisterThread --
 *
 *  Register thread as a target for sending scripts. The scripts will
 *  be executed in the passed interpreter.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Reserves the passed interp.
 *
 *----------------------------------------------------------------------
 */

void XThread_RegisterThread(Tcl_Interp *interp)
{
    ThreadSpecificData* tsdPtr = TCL_TSD_INIT(&dataKey);

    tsdPtr->interp = interp;
    Tcl_Preserve((ClientData)interp);
}

/*
 *----------------------------------------------------------------------
 *
 * XThread_UnregisterThread --
 *
 *  Makes this thread never execute any scripts passed to it.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Releases the registered interp, if any.
 *
 *----------------------------------------------------------------------
 */

void XThread_UnregisterThread()
{
    ThreadSpecificData* tsdPtr = TCL_TSD_INIT(&dataKey);

    if (tsdPtr->interp) {
        Tcl_Release((ClientData)tsdPtr->interp);
        tsdPtr->interp = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XThread_EvalInThread --
 *
 *  Run a script in another thread. The current (source) thread can be
 *  any thread, not necessarily created from Tcl. Any script results are
 *  ignored and control is immediately returned to the caller.
 *
 *  The target thread should register itself to allow executing of such
 *  scripts by the call to XThread_RegisterThread()
 *  
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Script may be executed in target thread.
 *
 *----------------------------------------------------------------------
 */

void XThread_EvalInThread(Tcl_ThreadId threadId, const char *script, int flags)
{
    ThreadSendData *sendPtr;
    int len = strlen(script);

    /*
     * Prepare job record for the target thread
     */

    sendPtr = (ThreadSendData*)Tcl_Alloc(sizeof(ThreadSendData));
    sendPtr->execProc   = ThreadSendEval;
    sendPtr->freeProc   = (ThreadSendFree*)Tcl_Free;
    sendPtr->clientData = (ClientData)strcpy(Tcl_Alloc(1+len), script);

    ThreadSend((Tcl_ThreadId)threadId, sendPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSend --
 *  @@@ Stripped down (Mats)
 *
 *  Run the procedure in other thread
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void
ThreadSend(targetId, send)
    Tcl_ThreadId targetId;       /* Thread Id of other thread. */
    ThreadSendData *send;        /* Pointer to structure with work to do */
{
    ThreadEvent *eventPtr;

    /* 
     * Create the event for target thread event queue.
     */

    eventPtr = (ThreadEvent*)Tcl_Alloc(sizeof(ThreadEvent));
    eventPtr->sendData = send;
    eventPtr->event.proc = ThreadEventProc;

    /*
     * Queue the event and poke the other thread's notifier.
     * The thread "id" should eventually visit it's event
     * loop in order to process this event.
     */

    Tcl_ThreadQueueEvent(targetId, (Tcl_Event*)eventPtr, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(targetId);

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadSendEval --
 *
 *  Evaluates Tcl script passed from source to target thread.
 *
 * Results:
 *  A standard Tcl result.
 *
 * Side effects:
 *
 *----------------------------------------------------------------------
 */

static int 
ThreadSendEval(interp, clientData)
    Tcl_Interp *interp;
    ClientData clientData;
{
    ThreadSendData *sendPtr = (ThreadSendData*)clientData;
    char *script = (char*)sendPtr->clientData;

    return Tcl_EvalEx(interp, script, -1, TCL_EVAL_GLOBAL);
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadEventProc --
 *
 *  Handle the event in the target thread.
 *  @@@ Stripped down (Mats)
 *
 * Results:
 *  Returns 1 to indicate that the event was processed.
 *
 * Side effects:
 *  Depends on the work to do.
 *
 *----------------------------------------------------------------------
 */

static int
ThreadEventProc(evPtr, mask)
    Tcl_Event *evPtr;           /* Really ThreadEvent */
    int mask;
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    ThreadEvent      *eventPtr = (ThreadEvent*)evPtr;
    ThreadSendData    *sendPtr = eventPtr->sendData;

    if (tsdPtr->interp != NULL) {
        if (sendPtr) {
            Tcl_CreateThreadExitHandler(ThreadFreeProc, (ClientData)sendPtr);
            (*sendPtr->execProc)(tsdPtr->interp, (ClientData)sendPtr);
            Tcl_DeleteThreadExitHandler(ThreadFreeProc, (ClientData)sendPtr);
        }
    }
    
    ThreadFreeProc((ClientData)sendPtr);

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * ThreadFreeProc --
 *
 *  Called when we are exiting and memory needs to be freed.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Clears up mem specified in ClientData
 *
 *----------------------------------------------------------------------
 */

static void
ThreadFreeProc(clientData)
    ClientData clientData;
{
    ThreadSendData *anyPtr = (ThreadSendData*)clientData;

    if (anyPtr) {
        if (anyPtr->clientData) {
            (*anyPtr->freeProc)(anyPtr->clientData);
        }
        Tcl_Free((char*)anyPtr);
    }
}
