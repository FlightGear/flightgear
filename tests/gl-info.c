/*
From: Steve Baker <sbaker@link.com>
Sender: root@fatcity.com
To: OPENGL-GAMEDEV-L <OPENGL-GAMEDEV-L@fatcity.com>
Subject: Re: Win32 OpenGL Resource Page
Date: Fri, 24 Apr 1998 07:33:51 -0800
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <simgear/compiler.h>
#if defined( __APPLE__)
# include <OpenGL/OpenGL.h>
# include <GLUT/glut.h>
#else
# include <GL/gl.h>
# ifdef HAVE_GLUT_H
#  include <GL/glut.h>
# endif
#endif


void getPrints ( GLenum token, char *string )
{
  printf ( "%s = \"%s\"\n", string, glGetString ( token ) ) ;
}

void getPrint2f ( GLenum token, char *string )
{
  GLfloat f[2] ;
  glGetFloatv ( token, f ) ;
  printf ( "%s = %g,%g\n", string, f[0],f[1] ) ;
}

void getPrintf ( GLenum token, char *string )
{
  GLfloat f ;
  glGetFloatv ( token, &f ) ;
  printf ( "%s = %g\n", string, f ) ;
}

void getPrint2i ( GLenum token, char *string )
{
  GLint i[2] ;
  glGetIntegerv ( token, i ) ;
  printf ( "%s = %d,%d\n", string, i[0],i[1] ) ;
}

void getPrinti ( GLenum token, char *string )
{
  GLint i ;
  glGetIntegerv ( token, &i ) ;
  printf ( "%s = %d\n", string, i ) ;
}

int main ( int argc, char **argv )
{
#ifdef HAVE_GLUT_H
  glutInit            ( &argc, argv ) ;
  glutInitDisplayMode ( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH ) ;
  glutCreateWindow    ( "You should never see this window!"  ) ;

  getPrints ( GL_VENDOR      , "GL_VENDOR"     ) ;
  getPrints ( GL_RENDERER    , "GL_RENDERER"   ) ;
  getPrints ( GL_VERSION     , "GL_VERSION"    ) ;
  getPrints ( GL_EXTENSIONS  , "GL_EXTENSIONS" ) ;

  getPrinti ( GL_RED_BITS    , "GL_RED_BITS"   ) ;
  getPrinti ( GL_GREEN_BITS  , "GL_GREEN_BITS" ) ;
  getPrinti ( GL_BLUE_BITS   , "GL_BLUE_BITS"  ) ;
  getPrinti ( GL_ALPHA_BITS  , "GL_ALPHA_BITS" ) ;
  getPrinti ( GL_DEPTH_BITS  , "GL_DEPTH_BITS" ) ;
  getPrinti ( GL_INDEX_BITS  , "GL_INDEX_BITS" ) ;
  getPrinti ( GL_STENCIL_BITS, "GL_STENCIL_BITS" ) ;

  getPrinti ( GL_ACCUM_RED_BITS  , "GL_ACCUM_RED_BITS"   ) ;
  getPrinti ( GL_ACCUM_GREEN_BITS, "GL_ACCUM_GREEN_BITS" ) ;
  getPrinti ( GL_ACCUM_BLUE_BITS , "GL_ACCUM_BLUE_BITS"  ) ;
  getPrinti ( GL_ACCUM_ALPHA_BITS, "GL_ACCUM_ALPHA_BITS" ) ;

  getPrinti ( GL_AUX_BUFFERS, "GL_AUX_BUFFERS" ) ;

  getPrinti ( GL_MAX_ATTRIB_STACK_DEPTH    , "GL_MAX_ATTRIB_STACK_DEPTH"     ) ;
  getPrinti ( GL_MAX_NAME_STACK_DEPTH      , "GL_MAX_NAME_STACK_DEPTH"       ) ;
  getPrinti ( GL_MAX_TEXTURE_STACK_DEPTH   , "GL_MAX_TEXTURE_STACK_DEPTH"    ) ;
  getPrinti ( GL_MAX_PROJECTION_STACK_DEPTH, "GL_MAX_PROJECTION_STACK_DEPTH" ) ;
  getPrinti ( GL_MAX_MODELVIEW_STACK_DEPTH , "GL_MAX_MODELVIEW_STACK_DEPTH"  ) ;

  getPrinti ( GL_MAX_CLIP_PLANES    , "GL_MAX_CLIP_PLANES"     ) ;
  getPrinti ( GL_MAX_EVAL_ORDER     , "GL_MAX_EVAL_ORDER"      ) ;
  getPrinti ( GL_MAX_LIGHTS         , "GL_MAX_LIGHTS"          ) ;
  getPrinti ( GL_MAX_LIST_NESTING   , "GL_MAX_LIST_NESTING"    ) ;
  getPrinti ( GL_MAX_TEXTURE_SIZE   , "GL_MAX_TEXTURE_SIZE"    ) ;
  getPrint2i( GL_MAX_VIEWPORT_DIMS  , "GL_MAX_VIEWPORT_DIMS"   ) ;

  getPrintf ( GL_POINT_SIZE_GRANULARITY, "GL_POINT_SIZE_GRANULARITY" ) ;
  getPrint2f( GL_POINT_SIZE_RANGE      , "GL_POINT_SIZE_RANGE" ) ;

  printf("Default values:\n\n");

  getPrinti( GL_UNPACK_ALIGNMENT  , "GL_UNPACK_ALIGNMENT"   ) ;
  getPrinti( GL_UNPACK_ROW_LENGTH  , "GL_UNPACK_ROW_LENGTH"   ) ;
  getPrinti( GL_UNPACK_SKIP_PIXELS  , "GL_UNPACK_SKIP_PIXELS"   ) ;
  getPrinti( GL_UNPACK_SKIP_ROWS  , "GL_UNPACK_SKIP_ROWS"   ) ;
  getPrinti( GL_BLEND_SRC  , "GL_BLEND_SRC"   ) ;
  getPrinti( GL_BLEND_DST  , "GL_BLEND_DST"   ) ;
#else

  printf("GL Utility Toolkit (glut) was not found on this system.\n");
#endif

  return 0 ;
}
