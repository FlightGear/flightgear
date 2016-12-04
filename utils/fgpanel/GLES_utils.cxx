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

#include <assert.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>

#ifdef _RPI
#include <bcm_host.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif

#include "GLES_utils.hxx"

/// esCreateWindow flag - RGB color buffer
#define GLES_UTILS_WINDOW_RGB           0
/// esCreateWindow flag - ALPHA color buffer
#define GLES_UTILS_WINDOW_ALPHA         1
/// esCreateWindow flag - depth buffer
#define GLES_UTILS_WINDOW_DEPTH         2
/// esCreateWindow flag - stencil buffer
#define GLES_UTILS_WINDOW_STENCIL       4
/// esCreateWindow flat - multi-sample buffer
#define GLES_UTILS_WINDOW_MULTISAMPLE   8

GLES_utils::GLES_utils () :
  m_State (),
  display_func (NULL),
  idle_func (NULL),
  keyboard_func (NULL),
  reshape_func (NULL) {}

GLES_utils::~GLES_utils () {}

GLES_utils&
GLES_utils::instance () {
  static GLES_utils* const The_Instance (new GLES_utils);
  return *The_Instance;
}

void
GLES_utils::init (const string &title) {
#ifdef _RPI
  bcm_host_init ();
  init_dispmanx (m_State.native_window);
#else
  init_display (m_State, title);
#endif
  init_egl (m_State, GLES_UTILS_WINDOW_RGB);
}

void
GLES_utils::print_config_info (const int n, const EGLDisplay &display, EGLConfig &config) {
  int size;

  cout << "EGL Configuration " << n << " is" << endl;

  eglGetConfigAttrib (display, config, EGL_RED_SIZE, &size);
  cout << "EGL  Red size is " << size << endl;

  eglGetConfigAttrib (display, config, EGL_BLUE_SIZE, &size);
  cout << "EGL  Blue size is " << size << endl;

  eglGetConfigAttrib (display, config, EGL_GREEN_SIZE, &size);
  cout << "EGL  Green size is " << size << endl;

  eglGetConfigAttrib (display, config, EGL_BUFFER_SIZE, &size);
  cout << "EGL  Buffer size is " << size << endl;

  eglGetConfigAttrib (display, config,  EGL_BIND_TO_TEXTURE_RGB , &size);
  if (size == EGL_TRUE) {
    cout << "EGL  Can be bound to RGB texture" << endl;
  } else {
    cout << "EGL  Can't be bound to RGB texture" << endl;
  }

  eglGetConfigAttrib (display, config,  EGL_BIND_TO_TEXTURE_RGBA , &size);
  if (size == EGL_TRUE) {
    cout << "EGL  Can be bound to RGBA texture" << endl;
  } else {
    cout << "EGL  Can't be bound to RGBA texture" << endl;
  }
}

void
GLES_utils::init_egl (EGL_STATE_T &state, const GLuint flags) {
  EGLBoolean result;

  static const EGLint attribute_list[] = {
    EGL_RED_SIZE,       5,
    EGL_GREEN_SIZE,     6,
    EGL_BLUE_SIZE,      5,
    EGL_ALPHA_SIZE,     (flags & GLES_UTILS_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE,
    EGL_DEPTH_SIZE,     (flags & GLES_UTILS_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE,
    EGL_STENCIL_SIZE,   (flags & GLES_UTILS_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE,
    EGL_SAMPLE_BUFFERS, (flags & GLES_UTILS_WINDOW_MULTISAMPLE) ? 1 : 0,
    EGL_NONE
  };

  static const EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
#ifndef _RPI
    , EGL_NONE
#endif
  };

  EGLConfig *configs;

  // get an EGL display connection
#ifdef _RPI
  state.display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
#else
  state.display = eglGetDisplay ((EGLNativeDisplayType) state.x_display);
#endif
  assert (state.display != EGL_NO_DISPLAY);

  // initialize the EGL display connection
  result = eglInitialize (state.display, &(state.major_version), &(state.minor_version));
  assert (result != EGL_FALSE);

  result = eglGetConfigs (state.display, NULL, 0, &(state.num_configs));
  assert (result != EGL_FALSE);

  configs = (EGLConfig *) calloc (state.num_configs, sizeof (*configs));
  result = eglGetConfigs (state.display, configs, state.num_configs, &(state.num_configs));
  assert (result != EGL_FALSE);

  cout << "EGL version = " << state.major_version << "." << state.minor_version << endl;
  cout << "EGL has " << state.num_configs << " configs" << endl;
  for (int i = 0; i < state.num_configs; ++i) {
    print_config_info (i, state.display, configs[i]);
  }

  // get an appropriate EGL frame buffer configuration
  result = eglChooseConfig (state.display, attribute_list, &(state.config), 1, &(state.num_configs));
  assert (result != EGL_FALSE);

  // Choose the OpenGL ES API
  result = eglBindAPI (EGL_OPENGL_ES_API);
  assert (result != EGL_FALSE);

#ifdef _RPI
  state.surface = eglCreateWindowSurface (state.display,
                                          state.config,
                                          (EGLNativeWindowType) &(state.native_window),
                                          NULL);
#else
  state.surface = eglCreateWindowSurface (state.display,
                                          state.config,
                                          state.native_window,
                                          NULL);
#endif
  assert (state.surface != EGL_NO_SURFACE);

  // create an EGL rendering context
  state.context = eglCreateContext (state.display, state.config, EGL_NO_CONTEXT, context_attributes);
  assert (state.context != EGL_NO_CONTEXT);

  // connect the context to the surface
  result = eglMakeCurrent (state.display, state.surface, state.surface, state.context);
  assert (result != EGL_FALSE);
}

#ifdef _RPI
void
GLES_utils::init_dispmanx (EGL_DISPMANX_WINDOW_T &native_window) {
  int32_t success = 0;
  uint32_t screen_width;
  uint32_t screen_height;

  DISPMANX_ELEMENT_HANDLE_T dispman_element;
  DISPMANX_DISPLAY_HANDLE_T dispman_display;
  DISPMANX_UPDATE_HANDLE_T dispman_update;
  VC_RECT_T dst_rect;
  VC_RECT_T src_rect;

  // create an EGL window surface
  success = graphics_get_display_size (0 /* LCD */,
                                       &screen_width,
                                       &screen_height);
  assert (success >= 0);

  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = screen_width;
  dst_rect.height = screen_height;

  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = screen_width << 16;
  src_rect.height = screen_height << 16;

  dispman_display = vc_dispmanx_display_open (0 /* LCD */);
  dispman_update = vc_dispmanx_update_start (0);

  dispman_element = vc_dispmanx_element_add (dispman_update,
                                             dispman_display,
                                             0 /*layer*/,
                                             &dst_rect,
                                             0 /*src*/,
                                             &src_rect,
                                             DISPMANX_PROTECTION_NONE,
                                             0 /*alpha*/,
                                             0 /*clamp*/,
                                             DISPMANX_TRANSFORM_T (0) /*transform*/);

  // Build an EGL_DISPMANX_WINDOW_T from the Dispmanx window
  native_window.element = dispman_element;
  native_window.width = screen_width;
  native_window.height = screen_height;
  vc_dispmanx_update_submit_sync (dispman_update);

  cout << "Got a Dispmanx window" << endl;
}

GLboolean
GLES_utils::user_interrupt () {
  return GL_FALSE;
}
#else
void
GLES_utils::init_display (EGL_STATE_T &state, const string &title) {
    Window root;
    XSetWindowAttributes swa;
    XSetWindowAttributes xattr;
    Atom wm_state;
    XWMHints hints;
    XEvent xev;
    Window win;

    state.width = 1024;
    state.height = 768;

    /*
     * X11 native display initialization
     */

    state.x_display = XOpenDisplay (NULL);
    assert (state.x_display != NULL);

    root = DefaultRootWindow (state.x_display);

    swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;
    win = XCreateWindow (state.x_display,
                         root,
                         0,
                         0,
                         state.width,
                         state.height,
                         0,
                         CopyFromParent,
                         InputOutput,
                         CopyFromParent,
                         CWEventMask,
                         &swa);

    xattr.override_redirect = false;
    XChangeWindowAttributes (state.x_display, win, CWOverrideRedirect, &xattr);

    hints.input = true;
    hints.flags = InputHint;
    XSetWMHints (state.x_display, win, &hints);

    // make the window visible on the screen
    XMapWindow (state.x_display, win);
    XStoreName (state.x_display, win, title.c_str ());

    // get identifiers for the provided atom name strings
    wm_state = XInternAtom (state.x_display, "_NET_WM_STATE", false);

    memset (&xev, 0, sizeof (xev));
    xev.type                 = ClientMessage;
    xev.xclient.window       = win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = 1;
    xev.xclient.data.l[1]    = false;
    XSendEvent (state.x_display, DefaultRootWindow (state.x_display), false, SubstructureNotifyMask, &xev);

    state.native_window = (EGLNativeWindowType) win;
}

GLboolean
GLES_utils::user_interrupt () {
  XEvent xev;
  KeySym key;
  GLboolean user_interrupt = GL_FALSE;
  char text;

  // Pump all messages from X server. Keypresses are directed to keyfunc (if defined)
  while (XPending (m_State.x_display)) {
    XNextEvent (m_State.x_display, &xev);
    if (xev.type == KeyPress) {
      if (XLookupString (&xev.xkey, &text, 1, &key, 0) == 1) {
        if (keyboard_func != NULL) {
          keyboard_func (text, 0, 0);
        }
      }
    }
    if (xev.type == DestroyNotify) {
      user_interrupt = GL_TRUE;
    }
  }
  return user_interrupt;
}
#endif

void
GLES_utils::register_display_func (void (*display_func) ()) {
  this->display_func = display_func;
}

void
GLES_utils::register_idle_func (void (*idle_func) ()) {
  this->idle_func = idle_func;
}

void
GLES_utils::register_keyboard_func (void (*keyboard_func) (unsigned char, int, int)) {
  this->keyboard_func = keyboard_func;
}

void
GLES_utils::register_reshape_func (void (*reshape_func) (int, int)) {
  this->reshape_func = reshape_func;
}

void
GLES_utils::main_loop () {
  struct timeval t1, t2;
  struct timezone tz;
  float delta_time;
  float total_time = 0.0f;
  unsigned int frames = 0;

  gettimeofday (&t1 , &tz);

  while (user_interrupt () == GL_FALSE) {
    gettimeofday (&t2, &tz);
    delta_time = (float) (t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
    t1 = t2;

    if (reshape_func != NULL) {
#ifdef _RPI
      reshape_func (m_State.native_window.width, m_State.native_window.height);
#else
      reshape_func (m_State.width, m_State.height);
#endif
    }
    if (idle_func != NULL) {
      idle_func ();
    }
    if (display_func != NULL) {
      display_func ();
    }

    eglSwapBuffers (m_State.display, m_State.surface);

    total_time += delta_time;
    ++frames;
    if (total_time >  2.0f) {
      cout << frames << " frames rendered in " << total_time << " seconds -> FPS = " << (frames / total_time) << endl;
      total_time -= 2.0f;
      frames = 0;
    }
  }
}
