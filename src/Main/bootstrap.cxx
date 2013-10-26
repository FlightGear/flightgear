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

#include <cstring>
#include <iostream>
using std::cerr;
using std::endl;

#include <Viewer/fgviewer.hxx>
#include "main.hxx"
#include "globals.hxx"
#include "fg_props.hxx"


#include "fg_os.hxx"

std::string homedir;
std::string hostname;

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

#if defined(_MSC_VER) || defined(_WIN32)
int main ( int argc, char **argv );
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             LPSTR lpCmdLine, int nCmdShow) {

  main( __argc, __argv );
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
int main ( int argc, char **argv )
{
#if defined(_MSC_VER) || defined(_WIN32) 
  // Don't show blocking "no disk in drive" error messages on Windows 7,
  // silently return errors to application instead.
  // See Microsoft MSDN #ms680621: "GUI apps should specify SEM_NOOPENFILEERRORBOX"
  SetErrorMode(SEM_NOOPENFILEERRORBOX);

  hostname = ::getenv( "COMPUTERNAME" );
#else
  // Unix(alike) systems
  char _hostname[256];
  gethostname(_hostname, 256);
  hostname = _hostname;
    
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

    // Enable floating-point exceptions for Windows
#if defined( _MSC_VER ) && defined( DEBUG )
    // Christian, we should document what this does
    _control87( _EM_INEXACT, _MCW_EM );
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
        // http://code.google.com/p/flightgear-bugs/issues/detail?id=1231
        // ensure sglog is inited before atexit() is registered, so logging
        // is possible inside fgExitCleanup
        sglog();
        
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

    } catch (const std::exception &e ) {
        cerr << "Fatal error (std::exception): " << e.what() << endl;

    } catch (const std::string &s) {
        cerr << "Fatal error (std::string): " << s << endl;

    } catch (const char *s) {
        cerr << "Fatal error (const char*): " << s << endl;

    } catch (...) {
        cerr << "Unknown exception in the main loop. Aborting..." << endl;
        if (errno)
            perror("Possible cause");
    }

    return 0;
}

// do some clean up on exit.  Specifically we want to delete the sound-manager,
// so OpenAL device and context are released cleanly
void fgExitCleanup() {

    if (_bootstrap_OSInit != 0)
        fgSetMouseCursor(MOUSE_CURSOR_POINTER);

    // on the common exit path globals is already deleted, and NULL,
    // so this only happens on error paths.
    delete globals;
}

