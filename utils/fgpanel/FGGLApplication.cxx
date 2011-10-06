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

#include "FGGLApplication.hxx"
#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#define snprintf sprintf_s
#endif
#if defined (SG_MAC)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <iostream>
#include <exception>
#include <stdio.h>

FGGLApplication * FGGLApplication::application = NULL;

FGGLApplication::FGGLApplication( const char * aName, int argc, char ** argv ) :
  gameMode(false),
  name( aName )
{
  if( application != NULL ) {
    std::cerr << "Only one instance of FGGLApplication allowed!" << std::endl;
    throw std::exception();
  }
  application = this;  

  glutInit( &argc, argv );
}

FGGLApplication::~FGGLApplication()
{
    if (gameMode)
        glutLeaveGameMode();
}

void FGGLApplication::DisplayCallback()
{
  if( application ) application->Display();
}

void FGGLApplication::IdleCallback()
{
  if( application ) application->Idle();
}

void FGGLApplication::KeyCallback( unsigned char key, int x, int y )
{
  if( application ) application->Key( key, x, y );
}

void FGGLApplication::ReshapeCallback( int width, int height )
{
  if( application ) application->Reshape( width, height );
}

void FGGLApplication::Run( int glutMode, bool gameMode, int width, int height, int bpp )
{
  glutInitDisplayMode(glutMode);
  if( gameMode ) {
    width = glutGet(GLUT_SCREEN_WIDTH);
    height = glutGet(GLUT_SCREEN_HEIGHT);
    char game_mode_str[20];
    snprintf(game_mode_str, 20, "%dx%d:%d", width, height, bpp );
    glutGameModeString( game_mode_str );
    glutEnterGameMode();
    this->gameMode = gameMode;
  } else {
    if( width == -1 ) 
      width = glutGet(GLUT_SCREEN_WIDTH);

    if( height == -1 )
      height = glutGet(GLUT_SCREEN_HEIGHT);

    glutInitDisplayMode(glutMode);
//    glutInitWindowSize(width, height);
    windowId = glutCreateWindow(name);
  }
  Init();

  glutKeyboardFunc(FGGLApplication::KeyCallback);
  glutIdleFunc(FGGLApplication::IdleCallback);
  glutDisplayFunc(FGGLApplication::DisplayCallback);
  glutReshapeFunc(FGGLApplication::ReshapeCallback);
  glutMainLoop();
}
