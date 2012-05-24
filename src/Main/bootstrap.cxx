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

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#if defined(HAVE_FEENABLEEXCEPT)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fenv.h>
#elif defined(__linux__) && defined(__i386__)
#  include <fpu_control.h>
#endif

#ifndef _WIN32
#  include <unistd.h> // for gethostname()
#endif

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <cstring>
#include <iostream>
using std::cerr;
using std::endl;

#include <Viewer/fgviewer.hxx>
#include "main.hxx"
#include "globals.hxx"
#include "fg_props.hxx"


#include "fg_os.hxx"

string homedir;
string hostname;

// forward declaration.
void fgExitCleanup();

static bool fpeAbort = false;
static void initFPE();

#if defined(HAVE_FEENABLEEXCEPT)
static void handleFPE(int);
static void
initFPE ()
{
    if (fpeAbort) {
        int except = fegetexcept();
        feenableexcept(except | FE_DIVBYZERO | FE_INVALID);
    } else {
        signal(SIGFPE, handleFPE);
    }
}

static void handleFPE(int)
{
    feclearexcept(FE_ALL_EXCEPT);
    signal(SIGFPE, handleFPE);
}
#elif defined(__linux__) && defined(__i386__)

static void handleFPE(int);
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
#else
static void initFPE()
{
}
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
#if _MSC_VER
  // Don't show blocking "no disk in drive" error messages on Windows 7,
  // silently return errors to application instead.
  // See Microsoft MSDN #ms680621: "GUI apps should specify SEM_NOOPENFILEERRORBOX"
  SetErrorMode(SEM_NOOPENFILEERRORBOX);

  // Windows has no $HOME aka %HOME%, so we have to construct the full path.
  homedir = ::getenv("APPDATA");
  homedir.append("\\flightgear.org");

  hostname = ::getenv( "COMPUTERNAME" );
#else
  // Unix(alike) systems
  char _hostname[256];
  gethostname(_hostname, 256);
  hostname = _hostname;
  
  homedir = ::getenv( "HOME" );
  
  signal(SIGPIPE, SIG_IGN);
#endif

#ifdef PTW32_STATIC_LIB
    // Initialise static pthread win32 lib
    pthread_win32_process_attach_np ();
#endif
    _bootstrap_OSInit = 0;

#if defined(__FreeBSD__)
    // Ignore floating-point exceptions on FreeBSD
    signal(SIGFPE, SIG_IGN); 
#else
    // Maybe Enable floating-point exceptions on Linux
    for (int i = 0; i < argc; ++i) {
        if (!strcmp("--enable-fpe", argv[i])) {
            fpeAbort = true;
            break;
        }
    }
    initFPE();
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
  
    bool fgviewer = false;
    for (int i = 0; i < argc; ++i) {
        if (!strcmp("--fgviewer", argv[i])) {
            fgviewer = true;
            break;
        }
    }

    // FIXME: add other, more specific
    // exceptions.
    try {
        std::set_terminate(fg_terminate);
        atexit(fgExitCleanup);
        if (fgviewer)
            fgviewerMain(argc, argv);
        else
            fgMainInit(argc, argv);
            
        
    } catch (const sg_throwable &t) {
                            // We must use cerr rather than
                            // logging, since logging may be
                            // disabled.
        cerr << "Fatal error: " << t.getFormattedMessage() << endl;
        if (std::strlen(t.getOrigin()) != 0)
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
}

