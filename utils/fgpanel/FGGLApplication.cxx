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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <exception>
#include <stdio.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#if defined (SG_MAC)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#elif defined (_GLES2)
#include <GLES2/gl2.h>
#include "GLES_utils.hxx"
#else
#include <GL/glew.h> // Must be included before <GL/gl.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#endif

#include <simgear/compiler.h>

#include "FGGLApplication.hxx"

using namespace std;

FGGLApplication *FGGLApplication::application = NULL;

FGGLApplication::FGGLApplication (const char * a_name, int argc, char **argv) :
  gameMode (false),
  name (a_name) {
  if (application != NULL ) {
    cerr << "Only one instance of FGGLApplication allowed!" << endl;
    throw exception ();
  }
  application = this;

#ifdef _GLES2
  GLES_utils::instance ().init ("FG Panel");
#else
  glutInit (&argc, argv);
  glutInitContextVersion (2, 1);
  glutInitContextFlags (GLUT_CORE_PROFILE);
#endif
}

FGGLApplication::~FGGLApplication () {
#ifndef _GLES2
  if (gameMode) {
    glutLeaveGameMode ();
  }
#endif
}

void
FGGLApplication::DisplayCallback () {
  if (application) {
    application->Display ();
  }
}

void
FGGLApplication::IdleCallback () {
  if (application) {
    application->Idle ();
  }
}

void
FGGLApplication::KeyCallback (const unsigned char key, const int x, const int y) {
  if (application) {
    application->Key (key, x, y);
  }
}

void
FGGLApplication::ReshapeCallback (const int width, const int height) {
  if (application) {
    application->Reshape (width, height);
  }
}

void
FGGLApplication::Run (const int glutMode,
                      const bool gameMode,
                      int width,
                      int height,
                      const int bpp) {
#ifndef _GLES2
  glutInitDisplayMode (glutMode);
  if (gameMode) {
    width = glutGet (GLUT_SCREEN_WIDTH);
    height = glutGet (GLUT_SCREEN_HEIGHT);
    char game_mode_str[20];
    snprintf (game_mode_str, 20, "%dx%d:%d", width, height, bpp);
    glutGameModeString (game_mode_str);
    glutEnterGameMode ();
    this->gameMode = gameMode;
  } else {
    if (width == -1) {
      width = glutGet (GLUT_SCREEN_WIDTH);
    }
    if (height == -1) {
      height = glutGet (GLUT_SCREEN_HEIGHT);
    }
    glutInitDisplayMode (glutMode);
    glutInitWindowSize(width, height);
    windowId = glutCreateWindow (name);
  }
  const GLenum GLEW_err (glewInit ());
  if (GLEW_OK != GLEW_err) {
    cerr << "Unable to initialize GLEW " << glewGetErrorString (GLEW_err) << endl;
    exit (1);
  }
  if (!GLEW_VERSION_2_1) { // check that the machine supports the 2.1 API.
    cerr << "GLEW version 2.1 not supported : " << glewGetString (GLEW_VERSION) << endl;
    exit (1);
  }
  cout << "OpenGL version = " << glGetString (GL_VERSION) << endl;
#endif
  Init ();
#ifdef _GLES2
  GLES_utils::instance ().register_keyboard_func (FGGLApplication::KeyCallback);
  GLES_utils::instance ().register_idle_func (FGGLApplication::IdleCallback);
  GLES_utils::instance ().register_display_func (FGGLApplication::DisplayCallback);
  GLES_utils::instance ().register_reshape_func (FGGLApplication::ReshapeCallback);
  GLES_utils::instance ().main_loop ();
#else
  glutKeyboardFunc (FGGLApplication::KeyCallback);
  glutIdleFunc (FGGLApplication::IdleCallback);
  glutDisplayFunc (FGGLApplication::DisplayCallback);
  glutReshapeFunc (FGGLApplication::ReshapeCallback);
  glutMainLoop ();
#endif
}
