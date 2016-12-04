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

#ifndef GLES_UTILS_HXX
#define GLES_UTILS_HXX

#include <boost/utility.hpp>
#include <string>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

using namespace std;

class GLES_utils : private boost::noncopyable {
public:
  static GLES_utils& instance ();

  void init (const string &title);

  void register_display_func (void (*display_func) ());
  void register_idle_func (void (*idle_func) ());
  void register_keyboard_func (void (*keyboard_func) (unsigned char, int, int));
  void register_reshape_func (void (*reshape_func) (int, int));

  void main_loop ();

private:
  explicit GLES_utils ();
  virtual ~GLES_utils ();

  typedef struct {
#ifdef _RPI
    EGL_DISPMANX_WINDOW_T native_window;
#else
    EGLNativeWindowType native_window;
    Display *x_display;
    GLint width;
    GLint height;
#endif
    EGLint major_version;
    EGLint minor_version;
    EGLint num_configs;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
  } EGL_STATE_T;

  EGL_STATE_T m_State;

  void (*display_func) ();
  void (*idle_func) ();
  void (*keyboard_func) (unsigned char, int, int);
  void (*reshape_func) (int, int);

  void print_config_info (const int n, const EGLDisplay &display, EGLConfig &config);
  void init_egl (EGL_STATE_T &state, const GLuint flags);
#ifdef _RPI
  void init_dispmanx (EGL_DISPMANX_WINDOW_T &native_window);
#else
  void init_display (EGL_STATE_T &state, const string &title);
#endif
  GLboolean user_interrupt ();
};

#endif
