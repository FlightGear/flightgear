// bootstrap.cxx -- bootstrap routines: main()
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997 - 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined(__linux__) && defined(__i386__)
#  include <fpu_control.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include STL_IOSTREAM
SG_USING_STD(cerr);
SG_USING_STD(endl);

#include "main.hxx"
#include "globals.hxx"


#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#  include <float.h>
#  include <pthread.h>
#endif

#include "fg_os.hxx"

#ifdef macintosh
#  include <console.h>		// -dw- for command line dialog
#endif

char *homedir = ::getenv( "HOME" );
char *hostname = ::getenv( "HOSTNAME" );
bool free_hostname = false;

// foreward declaration.
void fgExitCleanup();

#if defined(__linux__) && defined(__i386__)

static void handleFPE (int);

static void
initFPE ()
{
    fpu_control_t fpe_flags = 0;
    _FPU_GETCW(fpe_flags);
//     fpe_flags &= ~_FPU_MASK_IM;	// invalid operation
//     fpe_flags &= ~_FPU_MASK_DM;	// denormalized operand
//     fpe_flags &= ~_FPU_MASK_ZM;	// zero-divide
//     fpe_flags &= ~_FPU_MASK_OM;	// overflow
//     fpe_flags &= ~_FPU_MASK_UM;	// underflow
//     fpe_flags &= ~_FPU_MASK_PM;	// precision (inexact result)
    _FPU_SETCW(fpe_flags);
    signal(SIGFPE, handleFPE);
}

static void
handleFPE (int num)
{
  initFPE();
  SG_LOG(SG_GENERAL, SG_ALERT, "Floating point interrupt (SIGFPE)");
}
#endif

#ifdef __APPLE__

typedef struct
{
  int  lo;
  int  hi;
} PSN;

extern "C" {
  short CPSGetCurrentProcess(PSN *psn);
  short CPSSetProcessName (PSN *psn, char *processname);
  short CPSEnableForegroundOperation(PSN *psn, int _arg2, int _arg3, int _arg4, int _arg5);
  short CPSSetFrontProcess(PSN *psn);
};

#define CPSEnableFG(psn) CPSEnableForegroundOperation(psn,0x03,0x3C,0x2C,0x1103)

#endif

#ifdef _MSC_VER
int main ( int argc, char **argv );
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             LPSTR lpCmdLine, int nCmdShow) {

  logbuf::has_no_console();
  main( __argc, __argv );
}
#endif

#if defined( sgi )
#include <sys/fpu.h>
#include <sys/sysmp.h>
#include <unistd.h>

/*
 *  set the special "flush zero" bit (FS, bit 24) in the Control Status
 *  Register of the FPU of R4k and beyond so that the result of any
 *  underflowing operation will be clamped to zero, and no exception of
 *  any kind will be generated on the CPU.  This has no effect on an
 *  R3000.
 */
void flush_fpe(void)
{
    union fpc_csr f;
    f.fc_word = get_fpc_csr();
    f.fc_struct.flush = 1;
    set_fpc_csr(f.fc_word);
}
#endif

static void fg_terminate() {
    cerr << endl <<
            "Uncaught Exception: you should see a meaningful error message\n"
            "here, but your GLUT (or SDL) library was apparently compiled\n"
            "and/or linked without exception support. Please complain to\n"
            "its provider!"
            << endl << endl;
    abort();
}

int _bootstrap_OSInit;

// Main entry point; catch any exceptions that have made it this far.
int main ( int argc, char **argv ) {

#ifdef PTW32_STATIC_LIB
    // Initialise static pthread win32 lib
    pthread_win32_process_attach_np ();
#endif
    _bootstrap_OSInit = 0;

#if defined(__linux__) && defined(__i386__)
    // Enable floating-point exceptions for Linux/x86
    initFPE();
#elif defined(__FreeBSD__)
    // Ignore floating-point exceptions on FreeBSD
    signal(SIGFPE, SIG_IGN);
#endif
#ifndef _MSC_VER
    signal(SIGPIPE, SIG_IGN);
#endif

#if defined(sgi)
    flush_fpe();

    // Bind all non-rendering threads to CPU1
    // This will reduce the jitter caused by them to an absolute minimum,
    // but it will only work with superuser authority.
    if ( geteuid() == 0 )
    {
       sysmp(MP_CLOCK, 0);		// bind the timer only to CPU0
       sysmp(MP_ISOLATE, 1 );		// Isolate CPU1
       sysmp(MP_NONPREEMPTIVE, 1 );	// disable process time slicing on CPU1
    }
#endif

    // Enable floating-point exceptions for Windows
#if defined( _MSC_VER ) && defined( DEBUG )
    // Christian, we should document what this does
    _control87( _EM_INEXACT, _MCW_EM );
#endif

#if defined( HAVE_BC5PLUS )
    _control87(MCW_EM, MCW_EM);  /* defined in float.h */
#endif

    // Keyboard focus hack
#if defined(__APPLE__) && !defined(OSX_BUNDLE)
    {
      PSN psn;

      fgOSInit (&argc, argv);
      _bootstrap_OSInit++;

      CPSGetCurrentProcess(&psn);
      CPSSetProcessName(&psn, "FlightGear");
      CPSEnableFG(&psn);
      CPSSetFrontProcess(&psn);
    }
#endif

    // FIXME: add other, more specific
    // exceptions.
    try {
        std::set_terminate(fg_terminate);
        atexit(fgExitCleanup);
        fgMainInit(argc, argv);
    } catch (const sg_throwable &t) {
                            // We must use cerr rather than
                            // logging, since logging may be
                            // disabled.
        cerr << "Fatal error: " << t.getFormattedMessage() << endl;
        if (!t.getOrigin().empty())
            cerr << " (received from " << t.getOrigin() << ')' << endl;

    } catch (const string &s) {
        cerr << "Fatal error: " << s << endl;

    } catch (const char *s) {
        cerr << "Fatal error: " << s << endl;

    } catch (...) {
        cerr << "Unknown exception in the main loop. Aborting..." << endl;
        if (errno)
            perror("Possible cause");
    }

    return 0;
}

// do some clean up on exit.  Specifically we want to call alutExit()
// which happens in the sound manager destructor.
void fgExitCleanup() {

    if (_bootstrap_OSInit != 0)
        fgSetMouseCursor(MOUSE_CURSOR_POINTER);

    delete globals;

    if (free_hostname && hostname != NULL)
        free(hostname);
}

