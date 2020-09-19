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

#include <config.h>

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

#include <iostream>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <clocale>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/scene/tgdb/GroundLightManager.hxx>

#include <osg/Texture>
#include <osg/BufferObject>
#include <osg/Version>

#include <Viewer/fgviewer.hxx>
#include "main.hxx"
#include <flightgearBuildId.h>
#include <Main/globals.hxx>
#include <Main/fg_init.hxx>
#include <Main/options.hxx>
#include <Main/fg_props.hxx>
#include <Main/sentryIntegration.hxx>
#include <GUI/MessageBox.hxx>

#include "fg_os.hxx"

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
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
                             LPSTR lpCmdLine, int nCmdShow) 
{
	// convert wchar_t args to UTF-8 which is what we expect cross-platform
    int     numArgs  = 0;
	LPWSTR* wideArgs = CommandLineToArgvW(GetCommandLineW(), &numArgs);

	std::vector<char*> utf8Args;
    utf8Args.resize(numArgs);

	for (int a = 0; a < numArgs; ++a) {
		const auto s = simgear::strutils::convertWStringToUtf8(wideArgs[a]);
		// note we leak these (strudp calls malloc) but not a big concern
        utf8Args[a] = strdup(s.c_str());
	}

	main(numArgs, utf8Args.data());
}
#endif

#if defined(__GNUC__)
#include <execinfo.h>
#include <cxxabi.h>
void segfault_handler(int signo) {

  fprintf(stderr, "Error: caught signal %d:\n", signo);

  #ifndef __OpenBSD__
  void *array[128];
  size_t size;
  size = backtrace(array, 128);
  if (size) {
    char** list = backtrace_symbols(array, size);
    size_t fnlen = 256;
    char* fname = (char*)malloc(fnlen);

    for (size_t i=1; i<size; i++) {
	char *begin = 0, *offset = 0, *end = 0;
	for (char *p = list[i]; *p; ++p) {
	    if (*p == '(') begin = p;
	    else if (*p == '+') offset = p;
	    else if (*p == ')' && offset) {
		end = p;
		break;
	    }
	}

	if (begin && offset && end && begin<offset) {
	    *begin++ = '\0'; *offset++ = '\0'; *end = '\0';

	    int status;
	    char* ret = abi::__cxa_demangle(begin, fname, &fnlen, &status);
	    if (status == 0) {
		fname = ret;
		fprintf(stderr, "  %s : %s+%s\n", list[i], fname, offset);
	    }
	    else
		fprintf(stderr, "  %s : %s()+%s\n", list[i], begin, offset);
	}
	else
	    fprintf(stderr, "  %s\n", list[i]);
    }

    free(fname);
    free(list);
  }
  #endif

  std::abort();
}
#endif

[[noreturn]] static void fg_terminate()
{
    cerr << "Running FlightGear's terminate handler. The program is going to "
      "exit due to a fatal error condition, sorry." << std::endl;
    std::abort();
}

// Detect SSE2 support for x86, it is always available for x86_64
#if defined(__i386__)
# if defined(SG_WINDOWS)
#  include <intrin.h>
#  define get_cpuid(a,b)	__cpuid(a,b)
# else
#  include <cpuid.h>
#  define get_cpuid(a,b)	__cpuid(b,a[0],a[1],a[2],a[3])
# endif
# define CPUID_GETFEATURES	1
# define CPUID_FEAT_EDX_SSE2	(1 << 26)
bool detectSIMD()
{
   static int regs[4] = {0,0,0,0};
   get_cpuid(regs, CPUID_GETFEATURES);
   return (regs[3] & CPUID_FEAT_EDX_SSE2);
}
#else
bool detectSIMD() { return true; }
#endif

int _bootstrap_OSInit;

// Main entry point; catch any exceptions that have made it this far.
int main ( int argc, char **argv )
{
    // we don't want to accidently show a GUI box and block startup in
    // non_GUI setups, so check this value early here, before options are
    // processed
    const bool headless = flightgear::Options::checkForArg(argc, argv, "disable-gui");
    flightgear::setHeadlessMode(headless);

#ifdef ENABLE_SIMD
  if (!detectSIMD()) {
    flightgear::fatalMessageBoxThenExit(
      "Fatal error",
      "SSE2 support not detected, but this version of FlightGear requires "
      "SSE2 hardware support.");
  }
#endif

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
    
#if defined(HAVE_SENTRY)
  flightgear::initSentry();
#endif

// if we're not using the normal crash-reported, install our
// custom segfault handler on Linux, in debug builds.
// NB On OpenBSD this seems to lose info about where the signal
// happened, so is disabled.
#if !defined(SG_WINDOWS) && !defined(NDEBUG) && !defined(__OpenBSD__)
    if (!flightgear::isSentryEnabled()) {
        signal(SIGSEGV, segfault_handler);
    }
#endif

    initFPE(flightgear::Options::checkForArg(argc, argv, "enable-fpe"));

    // pick up all user locale settings, but force C locale for numerical/sorting
    // conversions because we have lots of code which assumes standard
    // formatting
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "C");
    setlocale(LC_COLLATE, "C");

    if (flightgear::Options::checkForArg(argc, argv, "uninstall")) {
        return fgUninstall();
    }
    
    std::cerr << "Boostrap-1" << std::endl;

    bool fgviewer = flightgear::Options::checkForArg(argc, argv, "fgviewer");
    int exitStatus = EXIT_FAILURE;
    try {
        // http://code.google.com/p/flightgear-bugs/issues/detail?id=1231
        // ensure sglog is inited before atexit() is registered, so logging
        // is possible inside fgExitCleanup
        sglog();


#if OSG_VERSION_LESS_THAN(3, 5, 0)
        // similar to above, ensure some static maps inside OSG exist before
        // we register our at-exit handler, otherwise the statics are gone
        // when fg_terminate runs, which causes crashes.
        osg::Texture::getTextureObjectManager(0);
        osg::GLBufferObjectManager::getGLBufferObjectManager(0);
#endif
        std::set_terminate(fg_terminate);
        atexit(fgExitCleanup);
        
        std::cerr << "Boostrap-2" << std::endl;

        if (fgviewer) {
            exitStatus = fgviewerMain(argc, argv);
        } else {
            exitStatus = fgMainInit(argc, argv);
        }

    } catch (const sg_throwable &t) {
        std::string info;
        if (std::strlen(t.getOrigin()) != 0)
            info = std::string("received from ") + t.getOrigin();
        flightgear::fatalMessageBoxWithoutExit(
          "Fatal exception", t.getFormattedMessage(), info);
    } catch (const std::exception &e ) {
        flightgear::fatalMessageBoxWithoutExit("Fatal exception", e.what());
    } catch (const std::string &s) {
        flightgear::fatalMessageBoxWithoutExit("Fatal exception", s);
    } catch (const flightgear::FatalErrorException&) {
        // we already showed the message box, just carry on to exit
    } catch (const char *s) {
        std::cerr << "Fatal error (const char*): " << s << std::endl;
    } catch (...) {
        flightgear::sentryReportException("Unknown main loop exception");
        std::cerr << "Unknown exception in the main loop. Aborting..." << std::endl;
        if (errno)
            perror("Possible cause");
    }

#if defined(HAVE_QT)
    flightgear::shutdownQtApp();
#endif
    return exitStatus;
}

// do some clean up on exit.  Specifically we want to delete the sound-manager,
// so OpenAL device and context are released cleanly
void fgExitCleanup() {

    if (_bootstrap_OSInit != 0) {
        fgSetMouseCursor(MOUSE_CURSOR_POINTER);
        fgOSCloseWindow();
    }

    // you might imagine we'd call shutdownQtApp here, but it's not safe to do
    // so in an atexit handler, and crashes on Mac. Thiago states this explicitly:
    // https://bugreports.qt.io/browse/QTBUG-48709

    // on the common exit path globals is already deleted, and NULL,
    // so this only happens on error paths.
    delete globals;
    // avoid crash on exit (https://sourceforge.net/p/flightgear/codetickets/1935/)
    simgear::GroundLightManager::instance()->getRunwayLightStateSet()->clear();
    simgear::GroundLightManager::instance()->getTaxiLightStateSet()->clear();
    simgear::GroundLightManager::instance()->getGroundLightStateSet()->clear();

    simgear::shutdownLogging();
    flightgear::shutdownSentry();
}
