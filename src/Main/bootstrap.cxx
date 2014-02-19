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

#if defined(__linux__)
// set link for setting _GNU_SOURCE before including fenv.h
// http://man7.org/linux/man-pages/man3/fenv.3.html

  #ifndef _GNU_SOURCE
    #define _GNU_SOURCE
  #endif

  #include <fenv.h>
#endif

#ifndef _WIN32
#  include <unistd.h> // for gethostname()
#endif

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iostream>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>

#include <osg/Texture>
#include <osg/BufferObject>

#include <Viewer/fgviewer.hxx>
#include "main.hxx"
#include <Include/version.h>
#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Main/fg_props.hxx>
#include <GUI/MessageBox.hxx>

#include "fg_os.hxx"

#if defined(SG_MAC)
    #include <GUI/CocoaHelpers.h> // for transformToForegroundApp
#endif

#if defined(HAVE_CRASHRPT)
	#include <CrashRpt.h>

bool global_crashRptEnabled = false;

#endif

using std::cerr;
using std::endl;

std::string homedir;
std::string hostname;

// forward declaration.
void fgExitCleanup();

static void initFPE(bool enableExceptions);

#if defined(__linux__)

static void handleFPE(int);
static void
initFPE (bool fpeAbort)
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
    SG_LOG(SG_GENERAL, SG_ALERT, "Floating point interrupt (SIGFPE)");
    signal(SIGFPE, handleFPE);
}
#elif defined (SG_WINDOWS)

static void initFPE(bool fpeAbort)
{
// Enable floating-point exceptions for Windows
    if (fpeAbort) {
        // set following link for what this does (note it does set SSE
        // flags too, it's not just for the x87 FPU)
        // http://msdn.microsoft.com/en-us/library/e9b52ceh.aspx
        _control87( _EM_INEXACT, _MCW_EM );
    }
}

#else
static void initFPE(bool)
{
    // Ignore floating-point exceptions on FreeBSD, OS-X, other Unices
    signal(SIGFPE, SIG_IGN);
}
#endif

#if defined(SG_WINDOWS)
int main ( int argc, char **argv );
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             LPSTR lpCmdLine, int nCmdShow) {

  main( __argc, __argv );
}
#endif

static void fg_terminate()
{
    flightgear::fatalMessageBox("Fatal exception", "Uncaught exception on some thread");
}

int _bootstrap_OSInit;

// Main entry point; catch any exceptions that have made it this far.
int main ( int argc, char **argv )
{
#if defined(SG_WINDOWS)
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

    _bootstrap_OSInit = 0;

#if defined(HAVE_CRASHRPT)
	// Define CrashRpt configuration parameters
	CR_INSTALL_INFO info;  
	memset(&info, 0, sizeof(CR_INSTALL_INFO));  
	info.cb = sizeof(CR_INSTALL_INFO);    
	info.pszAppName = "FlightGear";
	info.pszAppVersion = FLIGHTGEAR_VERSION;
	info.pszEmailSubject = "FlightGear " FLIGHTGEAR_VERSION " crash report";
	info.pszEmailTo = "fgcrash@goneabitbursar.com";
	info.pszUrl = "http://fgfs.goneabitbursar.com/crashreporter/crashrpt.php";
	info.uPriorities[CR_HTTP] = 3; 
	info.uPriorities[CR_SMTP] = 2;  
	info.uPriorities[CR_SMAPI] = 1;

	// Install all available exception handlers
	info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;
  
	// Restart the app on crash 
	info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS; 

	// automatically install handlers for all threads
	info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS;

	// Define the Privacy Policy URL 
	info.pszPrivacyPolicyURL = "http://flightgear.org/crash-privacypolicy.html"; 
  
	// Install crash reporting
	int nResult = crInstall(&info);    
	if(nResult!=0) {
		char buf[1024];
		crGetLastErrorMsg(buf, 1024);
		flightgear::modalMessageBox("CrashRpt setup failed", 
			"Failed to setup crash-reporting engine, check the installation is not damaged.",
			buf);
	} else {
		global_crashRptEnabled = true;

		crAddProperty("hudson-build-id", HUDSON_BUILD_ID);
		char buf[16];
		::snprintf(buf, 16, "%d", HUDSON_BUILD_NUMBER);
		crAddProperty("hudson-build-number", buf);
	}
#endif

    initFPE(flightgear::Options::checkForArg(argc, argv, "enable-fpe"));

    bool fgviewer = flightgear::Options::checkForArg(argc, argv, "fgviewer");
    try {
        // http://code.google.com/p/flightgear-bugs/issues/detail?id=1231
        // ensure sglog is inited before atexit() is registered, so logging
        // is possible inside fgExitCleanup
        sglog();
        
        // similar to above, ensure some static maps inside OSG exist before
        // we register our at-exit handler, otherwise the statics are gone
        // when fg_terminate runs, which causes crashes.
        osg::Texture::getTextureObjectManager(0);
        osg::GLBufferObjectManager::getGLBufferObjectManager(0);
        
        std::set_terminate(fg_terminate);
        atexit(fgExitCleanup);
        if (fgviewer)
            fgviewerMain(argc, argv);
        else
            fgMainInit(argc, argv);
           
    } catch (const sg_throwable &t) {
        std::string info;
        if (std::strlen(t.getOrigin()) != 0)
            info = std::string("received from ") + t.getOrigin();
        flightgear::fatalMessageBox("Fatal exception", t.getFormattedMessage(), info);

    } catch (const std::exception &e ) {
        flightgear::fatalMessageBox("Fatal exception", e.what());
    } catch (const std::string &s) {
        flightgear::fatalMessageBox("Fatal exception", s);
    } catch (const char *s) {
        std::cerr << "Fatal error (const char*): " << s << std::endl;

    } catch (...) {
        std::cerr << "Unknown exception in the main loop. Aborting..." << std::endl;
        if (errno)
            perror("Possible cause");
    }

#if defined(HAVE_CRASHRPT)
	crUninstall();
#endif

    return 0;
}

// do some clean up on exit.  Specifically we want to delete the sound-manager,
// so OpenAL device and context are released cleanly
void fgExitCleanup() {

    if (_bootstrap_OSInit != 0) {
        fgSetMouseCursor(MOUSE_CURSOR_POINTER);

        fgOSCloseWindow();
    }
    
    // on the common exit path globals is already deleted, and NULL,
    // so this only happens on error paths.
    delete globals;
}

