//
//  Written and (c) Torsten Dreyer - Torsten(at)t3r_dot_de
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <config.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#  include <direct.h>
#else
#  include <unistd.h>
#endif

#ifdef __APPLE__
#  include <CoreFoundation/CoreFoundation.h>
#endif

#include <chrono>
#include <iostream>
#include <thread>

#if defined (SG_MAC)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#elif defined (_GLES2)
#include <GLES2/gl2.h>
#else
#include <GL/glew.h> // Must be included before <GL/gl.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "FGGLApplication.hxx"
#include "FGPanelApplication.hxx"

#include <simgear/math/SGMisc.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/ResourceManager.hxx>

#include "panel_io.hxx"
#include "ApplicationProperties.hxx"


using namespace std;

inline static string
ParseArgs (int argc, char **argv, const string &token) {
  for (int i = 0; i < argc; i++) {
    const string arg (argv[i]);
    if (arg.find (token) == 0) {
      return arg.substr (token.length ());
    }
  }
  return "";
}

// define default location of fgdata (use the same as for fgfs)
inline static SGPath
platformDefaultRoot () {
#if defined(__CYGWIN__)
  return SGPath ("../data");
#elif defined(_WIN32)
  return SGPath ("..\\data");
#elif defined(__APPLE__)
  /*
   The following code looks for the base package inside the application
   bundle, in the standard Contents/Resources location.
   */
  CFURLRef resourcesUrl = CFBundleCopyResourcesDirectoryURL (CFBundleGetMainBundle ());

  // look for a 'data' subdir
  CFURLRef dataDir = CFURLCreateCopyAppendingPathComponent (NULL, resourcesUrl, CFSTR ("data"), true);

  // now convert down to a path, and the a c-string
  CFStringRef path = CFURLCopyFileSystemPath (dataDir, kCFURLPOSIXPathStyle);
  string root = CFStringGetCStringPtr (path, CFStringGetSystemEncoding ());

  CFRelease (resourcesUrl);
  CFRelease (dataDir);
  CFRelease (path);

  return SGPath (root);
#else
  return SGPath (PKGLIBDIR);
#endif
}

#include "FGPNGTextureLoader.hxx"
#include "FGRGBTextureLoader.hxx"

static FGPNGTextureLoader pngTextureLoader;
static FGRGBTextureLoader rgbTextureLoader;

FGPanelApplication::FGPanelApplication (int argc, char **argv) :
  FGGLApplication ("FlightGear Panel", argc, argv) {
  sglog().setLogLevels (SG_ALL, SG_WARN);
  FGCroppedTexture::registerTextureLoader ("png", &pngTextureLoader);
  FGCroppedTexture::registerTextureLoader ("rgb", &rgbTextureLoader);

  ApplicationProperties::root = platformDefaultRoot ().local8BitStr ();

  const string panelFilename (ParseArgs (argc, argv, "--panel="));
  const string fgRoot        (ParseArgs (argc, argv, "--fg-root="));

  if (fgRoot.length () > 0) {
    ApplicationProperties::root = fgRoot;
  }
  simgear::ResourceManager::instance ()->addBasePath (ApplicationProperties::root);

  if (panelFilename.length () == 0 ) {
    cerr << "Need a panel filename. Use --panel=path_to_filename" << endl;
    throw exception ();
  }

  // see if we got a valid fgdata path
  SGPath BaseCheck (ApplicationProperties::root);
  BaseCheck.append ("version");
  if (!BaseCheck.exists ()) {
    cerr << "Missing base package. Use --fg-root=path_to_fgdata" << endl;
    throw exception ();
  }

  try {
    const SGPath tpath (ApplicationProperties::GetRootPath (panelFilename.c_str ()));
    readProperties (tpath, ApplicationProperties::Properties);
  }
  catch (sg_io_exception & e) {
    cerr << e.getFormattedMessage () << endl;
    throw;
  }

  for (int i = 1; i < argc; i++) {
    const string arg (argv[i]);
    if (arg.find ("--prop:") == 0 ) {
      const string s2 (arg.substr (7));
      string::size_type p (s2.find ("="));
      if (p != string::npos) {
        const string propertyName (s2.substr (0, p));
        const string propertyValue (s2.substr (p + 1));
        ApplicationProperties::Properties->getNode (propertyName.c_str (), true )->setValue (propertyValue.c_str ());
      }
    }
  }

  const SGPropertyNode_ptr n (ApplicationProperties::Properties->getNode ("panel"));
  if (n != NULL) {
    panel = FGReadablePanel::read (n);
  }
  protocol = new FGPanelProtocol (ApplicationProperties::Properties->getNode ("communication", true));
  protocol->init ();
}

FGPanelApplication::~FGPanelApplication () {
}

void
FGPanelApplication::Run () {
#ifdef _GLES2
  const int mode (0);
#else
  const int mode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
#endif
  int w (panel == NULL ? 0 : panel->getWidth ());
  int h (panel == NULL ? 0 : panel->getHeight ());
  if (w == 0 && h == 0) {
    w = 1024;
    h = 768;
  } else if (w == 0) {
    w = h / 0.75;
  } else if (h == 0) {
    h = w * 0.75;
  }

  const bool gameMode (ApplicationProperties::Properties->getNode ( "game-mode", true)->getBoolValue ());
  FGGLApplication::Run (mode, gameMode, w, h);
}

void
FGPanelApplication::Init () {
#ifndef _GLES2
  glutSetCursor (GLUT_CURSOR_NONE);
#endif
  if (panel != NULL) {
    panel->init ();
  }
}

void
FGPanelApplication::Reshape (const int width, const int height) {
  this->width = width;
  this->height = height;
  glViewport (0, 0, GLsizei (width), GLsizei (height));
}

void
FGPanelApplication::Idle () {
#ifndef _GLES2
  const double d (glutGet (GLUT_ELAPSED_TIME));
#endif
  const double dt (Sleep ());
  if (dt == 0) {
    return;
  }
  if (panel != NULL) {
    panel->update (dt);
  }
#ifndef _GLES2
  glutSwapBuffers ();
#endif
  if (protocol != NULL) {
    protocol->update (dt);
  }
#ifndef _GLES2
  static double dsum = 0.0;
  static unsigned cnt = 0;
  dsum += glutGet (GLUT_ELAPSED_TIME) - d;
  cnt++;
  if (dsum > 1000.0) {
    ApplicationProperties::Properties->getNode ("/sim/frame-rate", true)->setDoubleValue (cnt * 1000.0 / dsum);
    dsum = 0.0;
    cnt = 0;
  }
#endif
}

void
FGPanelApplication::Key (const unsigned char key, const int x, const int y) {
  switch (key) {
  case 0x1b:
    exit(0);
    break;
  }
}

double
FGPanelApplication::Sleep () {
  SGTimeStamp current_time_stamp;
  static SGTimeStamp last_time_stamp;

  if (last_time_stamp.get_seconds () == 0) {
    last_time_stamp.stamp ();
  }
  const double model_hz (60);
  const double throttle_hz (ApplicationProperties::getDouble ("/sim/frame-rate-throttle-hz", 0.0));
  if (throttle_hz > 0.0) {
    // optionally throttle the frame rate (to get consistent frame
    // rates or reduce cpu usage.

    double frame_us (1.0e6 / throttle_hz);

    // sleep based timing loop.
    //
    // Calling sleep, even usleep() on linux is less accurate than
    // we like, but it does free up the cpu for other tasks during
    // the sleep so it is desirable.  Because of the way sleep()
    // is implemented in consumer operating systems like windows
    // and linux, you almost always sleep a little longer than the
    // requested amount.
    //
    // To combat the problem of sleeping too long, we calculate the
    // desired wait time and shorten it by 2000us (2ms) to avoid
    // [hopefully] over-sleep'ing.  The 2ms value was arrived at
    // via experimentation.  We follow this up at the end with a
    // simple busy-wait loop to get the final pause timing exactly
    // right.
    //
    // Assuming we don't oversleep by more than 2000us, this
    // should be a reasonable compromise between sleep based
    // waiting, and busy waiting.

    // sleep() will always overshoot by a bit so undersleep by
    // 2000us in the hopes of never oversleeping.
    frame_us -= 2000.0;
    if (frame_us < 0.0) {
      frame_us = 0.0;
    }
    current_time_stamp.stamp ();

    /* Convert to ms */
    const double elapsed_us ((current_time_stamp - last_time_stamp).toUSecs ());
    if (elapsed_us < frame_us) {
      const double requested_us (frame_us - elapsed_us);
      std::this_thread::sleep_for(std::chrono::milliseconds((int) (requested_us * 1000.0)));
    }

    // busy wait timing loop.
    //
    // This yields the most accurate timing.  If the previous
    // usleep() call is omitted this will peg the cpu
    // (which is just fine if FG is the only app you care about.)
    current_time_stamp.stamp ();
    const SGTimeStamp next_time_stamp (last_time_stamp + SGTimeStamp::fromSec (1e-6*frame_us));
    while (current_time_stamp < next_time_stamp) {
      current_time_stamp.stamp ();
    }

  } else {
    current_time_stamp.stamp ();
  }

  static double reminder = 0.0;
  static long global_multi_loop = 0;
  const double real_delta_time_sec ((double (current_time_stamp.toUSecs () - last_time_stamp.toUSecs ()) / 1000000.0) + reminder);
  last_time_stamp = current_time_stamp;
//fprintf(stdout,"\r%4.1lf ", 1/real_delta_time_sec );
//fflush(stdout);

  // round the real time down to a multiple of 1/model-hz.
  // this way all systems are updated the _same_ amount of dt.
  global_multi_loop = long (floor (real_delta_time_sec * model_hz));
  global_multi_loop = SGMisc<long>::max (0, global_multi_loop);
  reminder = real_delta_time_sec - double (global_multi_loop) / double (model_hz);
  return double (global_multi_loop) / double (model_hz);
}
