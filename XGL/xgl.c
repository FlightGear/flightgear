#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "xgl.h"
#include <GL/glut.h>

#ifdef XGL_TRACE

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

GLboolean xglIsEnabled ( GLenum cap )
{
  if ( xglTraceIsEnabled("glIsEnabled") )
    fprintf ( xglTraceFd, "  /* glIsEnabled ( (GLenum)%s ) ; */\n" , xglExpandGLenum ( (GLenum) cap ) ) ;

  return glIsEnabled ( cap ) ;
}

GLboolean xglIsList ( GLuint list )
{
  if ( xglTraceIsEnabled("glIsList") )
    fprintf ( xglTraceFd, "  /* glIsList ( (GLuint)%u ) ; */\n" , list ) ;

  return glIsList ( list ) ;
}

GLenum xglGetError (  )
{
  if ( xglTraceIsEnabled("glGetError") )
    fprintf ( xglTraceFd, "  /* glGetError (  ) ; */\n"  ) ;

  return glGetError (  ) ;
}

GLint xglRenderMode ( GLenum mode )
{
  if ( xglTraceIsEnabled("glRenderMode") )
    fprintf ( xglTraceFd, "  glRenderMode ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;

  return glRenderMode ( mode ) ;
}

GLuint xglGenLists ( GLsizei range )
{
  if ( xglTraceIsEnabled("glGenLists") )
    fprintf ( xglTraceFd, "  glGenLists ( (GLsizei)%d ) ;\n" , range ) ;

  return glGenLists ( range ) ;
}

const GLubyte* xglGetString ( GLenum name )
{
  if ( xglTraceIsEnabled("glGetString") )
    fprintf ( xglTraceFd, "  /* glGetString ( (GLenum)%s ) ; */\n" , xglExpandGLenum ( (GLenum) name ) ) ;

  return glGetString ( name ) ;
}

void xglAccum ( GLenum op, GLfloat value )
{
  if ( xglTraceIsEnabled("glAccum") )
    fprintf ( xglTraceFd, "  glAccum ( (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) op ), value ) ;
  if ( xglExecuteIsEnabled("glAccum") )
    glAccum ( op, value ) ;
}

void xglAlphaFunc ( GLenum func, GLclampf ref )
{
  if ( xglTraceIsEnabled("glAlphaFunc") )
    fprintf ( xglTraceFd, "  glAlphaFunc ( (GLenum)%s, (GLclampf)%ff ) ;\n" , xglExpandGLenum ( (GLenum) func ), ref ) ;
  if ( xglExecuteIsEnabled("glAlphaFunc") )
    glAlphaFunc ( func, ref ) ;
}

void xglArrayElementEXT ( GLint i )
{
  if ( xglTraceIsEnabled("glArrayElementEXT") )
    fprintf ( xglTraceFd, "  glArrayElementEXT ( (GLint)%d ) ;\n" , i ) ;
#ifdef GL_VERSION_1_1
    glArrayElement ( i ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glArrayElementEXT") )
    glArrayElementEXT ( i ) ;
#else
  fprintf ( xglTraceFd, "  glArrayElementEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglBegin ( GLenum mode )
{
  if ( xglTraceIsEnabled("glBegin") )
    fprintf ( xglTraceFd, "  glBegin ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glBegin") )
    glBegin ( mode ) ;
}

void xglBitmap ( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, GLubyte* bitmap )
{
  if ( xglTraceIsEnabled("glBitmap") )
    fprintf ( xglTraceFd, "  glBitmap ( (GLsizei)%d, (GLsizei)%d, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLubyte *)0x%08x ) ;\n" , width, height, xorig, yorig, xmove, ymove, bitmap ) ;
  if ( xglExecuteIsEnabled("glBitmap") )
    glBitmap ( width, height, xorig, yorig, xmove, ymove, bitmap ) ;
}

void xglBlendColorEXT ( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
  if ( xglTraceIsEnabled("glBlendColorEXT") )
    fprintf ( xglTraceFd, "  glBlendColorEXT ( (GLclampf)%ff, (GLclampf)%ff, (GLclampf)%ff, (GLclampf)%ff ) ;\n" , red, green, blue, alpha ) ;
#ifdef GL_EXT_blend_color
  if ( xglExecuteIsEnabled("glBlendColorEXT") )
    glBlendColorEXT ( red, green, blue, alpha ) ;
#else
  fprintf ( xglTraceFd, "  glBlendColorEXT isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglBlendEquationEXT ( GLenum mode )
{
  if ( xglTraceIsEnabled("glBlendEquationEXT") )
    fprintf ( xglTraceFd, "  glBlendEquationEXT ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
#ifdef GL_EXT_blend_minmax
  if ( xglExecuteIsEnabled("glBlendEquationEXT") )
    glBlendEquationEXT ( mode ) ;
#else
  fprintf ( xglTraceFd, "  glBlendEquationEXT isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglBlendFunc ( GLenum sfactor, GLenum dfactor )
{
  if ( xglTraceIsEnabled("glBlendFunc") )
    fprintf ( xglTraceFd, "  glBlendFunc ( (GLenum)%s, (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) sfactor ), xglExpandGLenum ( (GLenum) dfactor ) ) ;
  if ( xglExecuteIsEnabled("glBlendFunc") )
    glBlendFunc ( sfactor, dfactor ) ;
}

void xglCallList ( GLuint list )
{
  if ( xglTraceIsEnabled("glCallList") )
    fprintf ( xglTraceFd, "  glCallList ( (GLuint)%u ) ;\n" , list ) ;
  if ( xglExecuteIsEnabled("glCallList") )
    glCallList ( list ) ;
}

void xglCallLists ( GLsizei n, GLenum type, GLvoid* lists )
{
  if ( xglTraceIsEnabled("glCallLists") )
    fprintf ( xglTraceFd, "  glCallLists ( (GLsizei)%d, (GLenum)%s, (GLvoid *)0x%08x ) ;\n" , n, xglExpandGLenum ( (GLenum) type ), lists ) ;
  if ( xglExecuteIsEnabled("glCallLists") )
    glCallLists ( n, type, lists ) ;
}


void xglClear ( GLbitfield mask )
{
  if ( xglTraceIsEnabled("glClear") )
    switch ( mask )
    {
      case GL_COLOR_BUFFER_BIT :
        fprintf ( xglTraceFd, "  glClear ( GL_COLOR_BUFFER_BIT ) ;\n" ) ;
        break ;
      case GL_DEPTH_BUFFER_BIT :
        fprintf ( xglTraceFd, "  glClear ( GL_DEPTH_BUFFER_BIT ) ;\n" ) ;
        break ;
      case GL_ACCUM_BUFFER_BIT :
        fprintf ( xglTraceFd, "  glClear ( GL_ACCUM_BUFFER_BIT ) ;\n" ) ;
        break ;
      case GL_STENCIL_BUFFER_BIT :
        fprintf ( xglTraceFd, "  glClear ( GL_STENCIL_BUFFER_BIT ) ;\n" ) ;
        break ;
      case (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT) :
        fprintf ( xglTraceFd, "  glClear ( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT ) ;\n" ) ;
        break ;

      default :
        fprintf ( xglTraceFd, "  glClear ( (GLbitfield)0x%08x ) ;\n" , mask ) ; break ;
    }

  if ( xglExecuteIsEnabled("glClear") )
    glClear ( mask ) ;
}


void xglClearAccum ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
  if ( xglTraceIsEnabled("glClearAccum") )
    fprintf ( xglTraceFd, "  glClearAccum ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glClearAccum") )
    glClearAccum ( red, green, blue, alpha ) ;
}

void xglClearColor ( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
  if ( xglTraceIsEnabled("glClearColor") )
    fprintf ( xglTraceFd, "  glClearColor ( (GLclampf)%ff, (GLclampf)%ff, (GLclampf)%ff, (GLclampf)%ff ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glClearColor") )
    glClearColor ( red, green, blue, alpha ) ;
}

void xglClearDepth ( GLclampd depth )
{
  if ( xglTraceIsEnabled("glClearDepth") )
    fprintf ( xglTraceFd, "  glClearDepth ( (GLclampd)%f ) ;\n" , depth ) ;
  if ( xglExecuteIsEnabled("glClearDepth") )
    glClearDepth ( depth ) ;
}

void xglClearIndex ( GLfloat c )
{
  if ( xglTraceIsEnabled("glClearIndex") )
    fprintf ( xglTraceFd, "  glClearIndex ( (GLfloat)%ff ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glClearIndex") )
    glClearIndex ( c ) ;
}

void xglClearStencil ( GLint s )
{
  if ( xglTraceIsEnabled("glClearStencil") )
    fprintf ( xglTraceFd, "  glClearStencil ( (GLint)%d ) ;\n" , s ) ;
  if ( xglExecuteIsEnabled("glClearStencil") )
    glClearStencil ( s ) ;
}

void xglClipPlane ( GLenum plane, GLdouble* equation )
{
  if ( xglTraceIsEnabled("glClipPlane") )
    fprintf ( xglTraceFd, "  glClipPlane ( (GLenum)%s, (GLdouble *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) plane ), equation ) ;
  if ( xglExecuteIsEnabled("glClipPlane") )
    glClipPlane ( plane, equation ) ;
}

void xglColor3b ( GLbyte red, GLbyte green, GLbyte blue )
{
  if ( xglTraceIsEnabled("glColor3b") )
    fprintf ( xglTraceFd, "  glColor3b ( (GLbyte)%d, (GLbyte)%d, (GLbyte)%d ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3b") )
    glColor3b ( red, green, blue ) ;
}

void xglColor3bv ( GLbyte* v )
{
  if ( xglTraceIsEnabled("glColor3bv") )
    fprintf ( xglTraceFd, "  glColor3bv ( xglBuild3bv((GLbyte)%d,(GLbyte)%d,(GLbyte)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3bv") )
    glColor3bv ( v ) ;
}

void xglColor3d ( GLdouble red, GLdouble green, GLdouble blue )
{
  if ( xglTraceIsEnabled("glColor3d") )
    fprintf ( xglTraceFd, "  glColor3d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3d") )
    glColor3d ( red, green, blue ) ;
}

void xglColor3dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glColor3dv") )
    fprintf ( xglTraceFd, "  glColor3dv ( xglBuild3dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3dv") )
    glColor3dv ( v ) ;
}

void xglColor3f ( GLfloat red, GLfloat green, GLfloat blue )
{
  if ( xglTraceIsEnabled("glColor3f") )
    fprintf ( xglTraceFd, "  glColor3f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3f") )
    glColor3f ( red, green, blue ) ;
}

void xglColor3fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glColor3fv") )
    fprintf ( xglTraceFd, "  glColor3fv ( xglBuild3fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3fv") )
    glColor3fv ( v ) ;
}

void xglColor3i ( GLint red, GLint green, GLint blue )
{
  if ( xglTraceIsEnabled("glColor3i") )
    fprintf ( xglTraceFd, "  glColor3i ( (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3i") )
    glColor3i ( red, green, blue ) ;
}

void xglColor3iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glColor3iv") )
    fprintf ( xglTraceFd, "  glColor3iv ( xglBuild3iv((GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3iv") )
    glColor3iv ( v ) ;
}

void xglColor3s ( GLshort red, GLshort green, GLshort blue )
{
  if ( xglTraceIsEnabled("glColor3s") )
    fprintf ( xglTraceFd, "  glColor3s ( (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3s") )
    glColor3s ( red, green, blue ) ;
}

void xglColor3sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glColor3sv") )
    fprintf ( xglTraceFd, "  glColor3sv ( xglBuild3sv((GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3sv") )
    glColor3sv ( v ) ;
}

void xglColor3ub ( GLubyte red, GLubyte green, GLubyte blue )
{
  if ( xglTraceIsEnabled("glColor3ub") )
    fprintf ( xglTraceFd, "  glColor3ub ( (GLubyte)%u, (GLubyte)%u, (GLubyte)%u ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3ub") )
    glColor3ub ( red, green, blue ) ;
}

void xglColor3ubv ( GLubyte* v )
{
  if ( xglTraceIsEnabled("glColor3ubv") )
    fprintf ( xglTraceFd, "  glColor3ubv ( xglBuild3ubv((GLubyte)%d,(GLubyte)%d,(GLubyte)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3ubv") )
    glColor3ubv ( v ) ;
}

void xglColor3ui ( GLuint red, GLuint green, GLuint blue )
{
  if ( xglTraceIsEnabled("glColor3ui") )
    fprintf ( xglTraceFd, "  glColor3ui ( (GLuint)%u, (GLuint)%u, (GLuint)%u ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3ui") )
    glColor3ui ( red, green, blue ) ;
}

void xglColor3uiv ( GLuint* v )
{
  if ( xglTraceIsEnabled("glColor3uiv") )
    fprintf ( xglTraceFd, "  glColor3uiv ( xglBuild3uiv((GLuint)%d,(GLuint)%d,(GLuint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3uiv") )
    glColor3uiv ( v ) ;
}

void xglColor3us ( GLushort red, GLushort green, GLushort blue )
{
  if ( xglTraceIsEnabled("glColor3us") )
    fprintf ( xglTraceFd, "  glColor3us ( (GLushort)%u, (GLushort)%u, (GLushort)%u ) ;\n" , red, green, blue ) ;
  if ( xglExecuteIsEnabled("glColor3us") )
    glColor3us ( red, green, blue ) ;
}

void xglColor3usv ( GLushort* v )
{
  if ( xglTraceIsEnabled("glColor3usv") )
    fprintf ( xglTraceFd, "  glColor3usv ( xglBuild3usv((GLushort)%d,(GLushort)%d,(GLushort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glColor3usv") )
    glColor3usv ( v ) ;
}

void xglColor4b ( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha )
{
  if ( xglTraceIsEnabled("glColor4b") )
    fprintf ( xglTraceFd, "  glColor4b ( (GLbyte)%d, (GLbyte)%d, (GLbyte)%d, (GLbyte)%d ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4b") )
    glColor4b ( red, green, blue, alpha ) ;
}

void xglColor4bv ( GLbyte* v )
{
  if ( xglTraceIsEnabled("glColor4bv") )
    fprintf ( xglTraceFd, "  glColor4bv ( xglBuild4bv((GLbyte)%d,(GLbyte)%d,(GLbyte)%d,(GLbyte)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4bv") )
    glColor4bv ( v ) ;
}

void xglColor4d ( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha )
{
  if ( xglTraceIsEnabled("glColor4d") )
    fprintf ( xglTraceFd, "  glColor4d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4d") )
    glColor4d ( red, green, blue, alpha ) ;
}

void xglColor4dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glColor4dv") )
    fprintf ( xglTraceFd, "  glColor4dv ( xglBuild4dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4dv") )
    glColor4dv ( v ) ;
}

void xglColor4f ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
  if ( xglTraceIsEnabled("glColor4f") )
    fprintf ( xglTraceFd, "  glColor4f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4f") )
    glColor4f ( red, green, blue, alpha ) ;
}

void xglColor4fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glColor4fv") )
    fprintf ( xglTraceFd, "  glColor4fv ( xglBuild4fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4fv") )
    glColor4fv ( v ) ;
}

void xglColor4i ( GLint red, GLint green, GLint blue, GLint alpha )
{
  if ( xglTraceIsEnabled("glColor4i") )
    fprintf ( xglTraceFd, "  glColor4i ( (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4i") )
    glColor4i ( red, green, blue, alpha ) ;
}

void xglColor4iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glColor4iv") )
    fprintf ( xglTraceFd, "  glColor4iv ( xglBuild4iv((GLint)%d,(GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4iv") )
    glColor4iv ( v ) ;
}

void xglColor4s ( GLshort red, GLshort green, GLshort blue, GLshort alpha )
{
  if ( xglTraceIsEnabled("glColor4s") )
    fprintf ( xglTraceFd, "  glColor4s ( (GLshort)%d, (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4s") )
    glColor4s ( red, green, blue, alpha ) ;
}

void xglColor4sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glColor4sv") )
    fprintf ( xglTraceFd, "  glColor4sv ( xglBuild4sv((GLshort)%d,(GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4sv") )
    glColor4sv ( v ) ;
}

void xglColor4ub ( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
  if ( xglTraceIsEnabled("glColor4ub") )
    fprintf ( xglTraceFd, "  glColor4ub ( (GLubyte)%u, (GLubyte)%u, (GLubyte)%u, (GLubyte)%u ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4ub") )
    glColor4ub ( red, green, blue, alpha ) ;
}

void xglColor4ubv ( GLubyte* v )
{
  if ( xglTraceIsEnabled("glColor4ubv") )
    fprintf ( xglTraceFd, "  glColor4ubv ( xglBuild4ubv((GLubyte)%d,(GLubyte)%d,(GLubyte)%d,(GLubyte)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4ubv") )
    glColor4ubv ( v ) ;
}

void xglColor4ui ( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
  if ( xglTraceIsEnabled("glColor4ui") )
    fprintf ( xglTraceFd, "  glColor4ui ( (GLuint)%u, (GLuint)%u, (GLuint)%u, (GLuint)%u ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4ui") )
    glColor4ui ( red, green, blue, alpha ) ;
}

void xglColor4uiv ( GLuint* v )
{
  if ( xglTraceIsEnabled("glColor4uiv") )
    fprintf ( xglTraceFd, "  glColor4uiv ( xglBuild4uiv((GLuint)%d,(GLuint)%d,(GLuint)%d,(GLuint)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4uiv") )
    glColor4uiv ( v ) ;
}

void xglColor4us ( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
  if ( xglTraceIsEnabled("glColor4us") )
    fprintf ( xglTraceFd, "  glColor4us ( (GLushort)%u, (GLushort)%u, (GLushort)%u, (GLushort)%u ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColor4us") )
    glColor4us ( red, green, blue, alpha ) ;
}

void xglColor4usv ( GLushort* v )
{
  if ( xglTraceIsEnabled("glColor4usv") )
    fprintf ( xglTraceFd, "  glColor4usv ( xglBuild4usv((GLushort)%d,(GLushort)%d,(GLushort)%d,(GLushort)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glColor4usv") )
    glColor4usv ( v ) ;
}

void xglColorMask ( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
  if ( xglTraceIsEnabled("glColorMask") )
    fprintf ( xglTraceFd, "  glColorMask ( (GLboolean)%d, (GLboolean)%d, (GLboolean)%d, (GLboolean)%d ) ;\n" , red, green, blue, alpha ) ;
  if ( xglExecuteIsEnabled("glColorMask") )
    glColorMask ( red, green, blue, alpha ) ;
}

void xglColorMaterial ( GLenum face, GLenum mode )
{
  if ( xglTraceIsEnabled("glColorMaterial") )
    fprintf ( xglTraceFd, "  glColorMaterial ( (GLenum)%s, (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glColorMaterial") )
    glColorMaterial ( face, mode ) ;
}

void xglColorPointerEXT ( GLint size, GLenum type, GLsizei stride, GLsizei count, void* ptr )
{
  if ( xglTraceIsEnabled("glColorPointerEXT") )
    fprintf ( xglTraceFd, "  glColorPointerEXT ( (GLint)%d, (GLenum)%s, (GLsizei)%d, (GLsizei)%d, (void *)0x%08x ) ;\n" , size, xglExpandGLenum ( (GLenum) type ), stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glColorPointer ( size, type, stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glColorPointerEXT") )
    glColorPointerEXT ( size, type, stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glColorPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglCopyPixels ( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type )
{
  if ( xglTraceIsEnabled("glCopyPixels") )
    fprintf ( xglTraceFd, "  glCopyPixels ( (GLint)%d, (GLint)%d, (GLsizei)%d, (GLsizei)%d, (GLenum)%s ) ;\n" , x, y, width, height, xglExpandGLenum ( (GLenum) type ) ) ;
  if ( xglExecuteIsEnabled("glCopyPixels") )
    glCopyPixels ( x, y, width, height, type ) ;
}

void xglCullFace ( GLenum mode )
{
  if ( xglTraceIsEnabled("glCullFace") )
    fprintf ( xglTraceFd, "  glCullFace ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glCullFace") )
    glCullFace ( mode ) ;
}

void xglDeleteLists ( GLuint list, GLsizei range )
{
  if ( xglTraceIsEnabled("glDeleteLists") )
    fprintf ( xglTraceFd, "  glDeleteLists ( (GLuint)%u, (GLsizei)%d ) ;\n" , list, range ) ;
  if ( xglExecuteIsEnabled("glDeleteLists") )
    glDeleteLists ( list, range ) ;
}

void xglDepthFunc ( GLenum func )
{
  if ( xglTraceIsEnabled("glDepthFunc") )
    fprintf ( xglTraceFd, "  glDepthFunc ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) func ) ) ;
  if ( xglExecuteIsEnabled("glDepthFunc") )
    glDepthFunc ( func ) ;
}

void xglDepthMask ( GLboolean flag )
{
  if ( xglTraceIsEnabled("glDepthMask") )
    fprintf ( xglTraceFd, "  glDepthMask ( (GLboolean)%d ) ;\n" , flag ) ;
  if ( xglExecuteIsEnabled("glDepthMask") )
    glDepthMask ( flag ) ;
}

void xglDepthRange ( GLclampd near_val, GLclampd far_val )
{
  if ( xglTraceIsEnabled("glDepthRange") )
    fprintf ( xglTraceFd, "  glDepthRange ( (GLclampd)%f, (GLclampd)%f ) ;\n" , near_val, far_val ) ;
  if ( xglExecuteIsEnabled("glDepthRange") )
    glDepthRange ( near_val, far_val ) ;
}

void xglDisable ( GLenum cap )
{
  if ( xglTraceIsEnabled("glDisable") )
    fprintf ( xglTraceFd, "  glDisable ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) cap ) ) ;
  if ( xglExecuteIsEnabled("glDisable") )
    glDisable ( cap ) ;
}

void xglDrawArraysEXT ( GLenum mode, GLint first, GLsizei count )
{
  if ( xglTraceIsEnabled("glDrawArraysEXT") )
    fprintf ( xglTraceFd, "  glDrawArraysEXT ( (GLenum)%s, (GLint)%d, (GLsizei)%d ) ;\n" , xglExpandGLenum ( (GLenum) mode ), first, count ) ;
#ifdef GL_VERSION_1_1
    glDrawArrays ( mode, first, count ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glDrawArraysEXT") )
    glDrawArraysEXT ( mode, first, count ) ;
#else
  fprintf ( xglTraceFd, "  glDrawArraysEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglDrawBuffer ( GLenum mode )
{
  if ( xglTraceIsEnabled("glDrawBuffer") )
    fprintf ( xglTraceFd, "  glDrawBuffer ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glDrawBuffer") )
    glDrawBuffer ( mode ) ;
}

void xglDrawPixels ( GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels )
{
  if ( xglTraceIsEnabled("glDrawPixels") )
    fprintf ( xglTraceFd, "  glDrawPixels ( (GLsizei)%d, (GLsizei)%d, (GLenum)%s, (GLenum)%s, (GLvoid *)0x%08x ) ;\n" , width, height, xglExpandGLenum ( (GLenum) format ), xglExpandGLenum ( (GLenum) type ), pixels ) ;
  if ( xglExecuteIsEnabled("glDrawPixels") )
    glDrawPixels ( width, height, format, type, pixels ) ;
}

void xglEdgeFlag ( GLboolean flag )
{
  if ( xglTraceIsEnabled("glEdgeFlag") )
    fprintf ( xglTraceFd, "  glEdgeFlag ( (GLboolean)%d ) ;\n" , flag ) ;
  if ( xglExecuteIsEnabled("glEdgeFlag") )
    glEdgeFlag ( flag ) ;
}

void xglEdgeFlagPointerEXT ( GLsizei stride, GLsizei count, GLboolean* ptr )
{
  if ( xglTraceIsEnabled("glEdgeFlagPointerEXT") )
    fprintf ( xglTraceFd, "  glEdgeFlagPointerEXT ( (GLsizei)%d, (GLsizei)%d, (GLboolean *)0x%08x ) ;\n" , stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glEdgeFlagPointer ( stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glEdgeFlagPointerEXT") )
    glEdgeFlagPointerEXT ( stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glEdgeFlagPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglEdgeFlagv ( GLboolean* flag )
{
  if ( xglTraceIsEnabled("glEdgeFlagv") )
    fprintf ( xglTraceFd, "  glEdgeFlagv ( (GLboolean *)0x%08x ) ;\n" , flag ) ;
  if ( xglExecuteIsEnabled("glEdgeFlagv") )
    glEdgeFlagv ( flag ) ;
}

void xglEnable ( GLenum cap )
{
  if ( xglTraceIsEnabled("glEnable") )
    fprintf ( xglTraceFd, "  glEnable ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) cap ) ) ;
  if ( xglExecuteIsEnabled("glEnable") )
    glEnable ( cap ) ;
}

void xglEnd (  )
{
  if ( xglTraceIsEnabled("glEnd") )
    fprintf ( xglTraceFd, "  glEnd (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glEnd") )
    glEnd (  ) ;
}

void xglEndList (  )
{
  if ( xglTraceIsEnabled("glEndList") )
    fprintf ( xglTraceFd, "  glEndList (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glEndList") )
    glEndList (  ) ;
}

void xglEvalCoord1d ( GLdouble u )
{
  if ( xglTraceIsEnabled("glEvalCoord1d") )
    fprintf ( xglTraceFd, "  glEvalCoord1d ( (GLdouble)%f ) ;\n" , u ) ;
  if ( xglExecuteIsEnabled("glEvalCoord1d") )
    glEvalCoord1d ( u ) ;
}

void xglEvalCoord1dv ( GLdouble* u )
{
  if ( xglTraceIsEnabled("glEvalCoord1dv") )
    fprintf ( xglTraceFd, "  glEvalCoord1dv ( xglBuild1dv((GLdouble)%f) ) ;\n" , u[0] ) ;
  if ( xglExecuteIsEnabled("glEvalCoord1dv") )
    glEvalCoord1dv ( u ) ;
}

void xglEvalCoord1f ( GLfloat u )
{
  if ( xglTraceIsEnabled("glEvalCoord1f") )
    fprintf ( xglTraceFd, "  glEvalCoord1f ( (GLfloat)%ff ) ;\n" , u ) ;
  if ( xglExecuteIsEnabled("glEvalCoord1f") )
    glEvalCoord1f ( u ) ;
}

void xglEvalCoord1fv ( GLfloat* u )
{
  if ( xglTraceIsEnabled("glEvalCoord1fv") )
    fprintf ( xglTraceFd, "  glEvalCoord1fv ( xglBuild1fv((GLfloat)%ff) ) ;\n" , u[0] ) ;
  if ( xglExecuteIsEnabled("glEvalCoord1fv") )
    glEvalCoord1fv ( u ) ;
}

void xglEvalCoord2d ( GLdouble u, GLdouble v )
{
  if ( xglTraceIsEnabled("glEvalCoord2d") )
    fprintf ( xglTraceFd, "  glEvalCoord2d ( (GLdouble)%f, (GLdouble)%f ) ;\n" , u, v ) ;
  if ( xglExecuteIsEnabled("glEvalCoord2d") )
    glEvalCoord2d ( u, v ) ;
}

void xglEvalCoord2dv ( GLdouble* u )
{
  if ( xglTraceIsEnabled("glEvalCoord2dv") )
    fprintf ( xglTraceFd, "  glEvalCoord2dv ( xglBuild2dv((GLdouble)%f,(GLdouble)%f) ) ;\n" , u[0], u[1] ) ;
  if ( xglExecuteIsEnabled("glEvalCoord2dv") )
    glEvalCoord2dv ( u ) ;
}

void xglEvalCoord2f ( GLfloat u, GLfloat v )
{
  if ( xglTraceIsEnabled("glEvalCoord2f") )
    fprintf ( xglTraceFd, "  glEvalCoord2f ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , u, v ) ;
  if ( xglExecuteIsEnabled("glEvalCoord2f") )
    glEvalCoord2f ( u, v ) ;
}

void xglEvalCoord2fv ( GLfloat* u )
{
  if ( xglTraceIsEnabled("glEvalCoord2fv") )
    fprintf ( xglTraceFd, "  glEvalCoord2fv ( xglBuild2fv((GLfloat)%ff,(GLfloat)%ff) ) ;\n" , u[0], u[1] ) ;
  if ( xglExecuteIsEnabled("glEvalCoord2fv") )
    glEvalCoord2fv ( u ) ;
}

void xglEvalMesh1 ( GLenum mode, GLint i1, GLint i2 )
{
  if ( xglTraceIsEnabled("glEvalMesh1") )
    fprintf ( xglTraceFd, "  glEvalMesh1 ( (GLenum)%s, (GLint)%d, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) mode ), i1, i2 ) ;
  if ( xglExecuteIsEnabled("glEvalMesh1") )
    glEvalMesh1 ( mode, i1, i2 ) ;
}

void xglEvalMesh2 ( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
  if ( xglTraceIsEnabled("glEvalMesh2") )
    fprintf ( xglTraceFd, "  glEvalMesh2 ( (GLenum)%s, (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) mode ), i1, i2, j1, j2 ) ;
  if ( xglExecuteIsEnabled("glEvalMesh2") )
    glEvalMesh2 ( mode, i1, i2, j1, j2 ) ;
}

void xglEvalPoint1 ( GLint i )
{
  if ( xglTraceIsEnabled("glEvalPoint1") )
    fprintf ( xglTraceFd, "  glEvalPoint1 ( (GLint)%d ) ;\n" , i ) ;
  if ( xglExecuteIsEnabled("glEvalPoint1") )
    glEvalPoint1 ( i ) ;
}

void xglEvalPoint2 ( GLint i, GLint j )
{
  if ( xglTraceIsEnabled("glEvalPoint2") )
    fprintf ( xglTraceFd, "  glEvalPoint2 ( (GLint)%d, (GLint)%d ) ;\n" , i, j ) ;
  if ( xglExecuteIsEnabled("glEvalPoint2") )
    glEvalPoint2 ( i, j ) ;
}

void xglFeedbackBuffer ( GLsizei size, GLenum type, GLfloat* buffer )
{
  if ( xglTraceIsEnabled("glFeedbackBuffer") )
    fprintf ( xglTraceFd, "  glFeedbackBuffer ( (GLsizei)%d, (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , size, xglExpandGLenum ( (GLenum) type ), buffer ) ;
  if ( xglExecuteIsEnabled("glFeedbackBuffer") )
    glFeedbackBuffer ( size, type, buffer ) ;
}

void xglFinish (  )
{
  if ( xglTraceIsEnabled("glFinish") )
    fprintf ( xglTraceFd, "  glFinish (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glFinish") )
    glFinish (  ) ;
}

void xglFlush (  )
{
  if ( xglTraceIsEnabled("glFlush") )
    fprintf ( xglTraceFd, "  glFlush (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glFlush") )
    glFlush (  ) ;
}

void xglFogf ( GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glFogf") )
    fprintf ( xglTraceFd, "  glFogf ( (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glFogf") )
    glFogf ( pname, param ) ;
}

void xglFogfv ( GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glFogfv") )
    fprintf ( xglTraceFd, "  glFogfv ( (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glFogfv") )
    glFogfv ( pname, params ) ;
}

void xglFogi ( GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glFogi") )
    fprintf ( xglTraceFd, "  glFogi ( (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glFogi") )
    glFogi ( pname, param ) ;
}

void xglFogiv ( GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glFogiv") )
    fprintf ( xglTraceFd, "  glFogiv ( (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glFogiv") )
    glFogiv ( pname, params ) ;
}

void xglFrontFace ( GLenum mode )
{
  if ( xglTraceIsEnabled("glFrontFace") )
    fprintf ( xglTraceFd, "  glFrontFace ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glFrontFace") )
    glFrontFace ( mode ) ;
}

void xglFrustum ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val )
{
  if ( xglTraceIsEnabled("glFrustum") )
    fprintf ( xglTraceFd, "  glFrustum ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , left, right, bottom, top, near_val, far_val ) ;
  if ( xglExecuteIsEnabled("glFrustum") )
    glFrustum ( left, right, bottom, top, near_val, far_val ) ;
}

void xglGetBooleanv ( GLenum pname, GLboolean* params )
{
  if ( xglTraceIsEnabled("glGetBooleanv") )
    fprintf ( xglTraceFd, "  /* glGetBooleanv ( (GLenum)%s, (GLboolean *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetBooleanv") )
    glGetBooleanv ( pname, params ) ;
}

void xglGetClipPlane ( GLenum plane, GLdouble* equation )
{
  if ( xglTraceIsEnabled("glGetClipPlane") )
    fprintf ( xglTraceFd, "  /* glGetClipPlane ( (GLenum)%s, (GLdouble *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) plane ), equation ) ;
  if ( xglExecuteIsEnabled("glGetClipPlane") )
    glGetClipPlane ( plane, equation ) ;
}

void xglGetDoublev ( GLenum pname, GLdouble* params )
{
  if ( xglTraceIsEnabled("glGetDoublev") )
    fprintf ( xglTraceFd, "  /* glGetDoublev ( (GLenum)%s, (GLdouble *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetDoublev") )
    glGetDoublev ( pname, params ) ;
}

void xglGetFloatv ( GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetFloatv") )
    fprintf ( xglTraceFd, "  /* glGetFloatv ( (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetFloatv") )
    glGetFloatv ( pname, params ) ;
}

void xglGetIntegerv ( GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetIntegerv") )
    fprintf ( xglTraceFd, "  /* glGetIntegerv ( (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetIntegerv") )
    glGetIntegerv ( pname, params ) ;
}

void xglGetLightfv ( GLenum light, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetLightfv") )
    fprintf ( xglTraceFd, "  /* glGetLightfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetLightfv") )
    glGetLightfv ( light, pname, params ) ;
}

void xglGetLightiv ( GLenum light, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetLightiv") )
    fprintf ( xglTraceFd, "  /* glGetLightiv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetLightiv") )
    glGetLightiv ( light, pname, params ) ;
}

void xglGetMapdv ( GLenum target, GLenum query, GLdouble* v )
{
  if ( xglTraceIsEnabled("glGetMapdv") )
    fprintf ( xglTraceFd, "  /* glGetMapdv ( (GLenum)%s, (GLenum)%s, (GLdouble *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) query ), v ) ;
  if ( xglExecuteIsEnabled("glGetMapdv") )
    glGetMapdv ( target, query, v ) ;
}

void xglGetMapfv ( GLenum target, GLenum query, GLfloat* v )
{
  if ( xglTraceIsEnabled("glGetMapfv") )
    fprintf ( xglTraceFd, "  /* glGetMapfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) query ), v ) ;
  if ( xglExecuteIsEnabled("glGetMapfv") )
    glGetMapfv ( target, query, v ) ;
}

void xglGetMapiv ( GLenum target, GLenum query, GLint* v )
{
  if ( xglTraceIsEnabled("glGetMapiv") )
    fprintf ( xglTraceFd, "  /* glGetMapiv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) query ), v ) ;
  if ( xglExecuteIsEnabled("glGetMapiv") )
    glGetMapiv ( target, query, v ) ;
}

void xglGetMaterialfv ( GLenum face, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetMaterialfv") )
    fprintf ( xglTraceFd, "  /* glGetMaterialfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetMaterialfv") )
    glGetMaterialfv ( face, pname, params ) ;
}

void xglGetMaterialiv ( GLenum face, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetMaterialiv") )
    fprintf ( xglTraceFd, "  /* glGetMaterialiv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetMaterialiv") )
    glGetMaterialiv ( face, pname, params ) ;
}

void xglGetPixelMapfv ( GLenum map, GLfloat* values )
{
  if ( xglTraceIsEnabled("glGetPixelMapfv") )
    fprintf ( xglTraceFd, "  /* glGetPixelMapfv ( (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) map ), values ) ;
  if ( xglExecuteIsEnabled("glGetPixelMapfv") )
    glGetPixelMapfv ( map, values ) ;
}

void xglGetPixelMapuiv ( GLenum map, GLuint* values )
{
  if ( xglTraceIsEnabled("glGetPixelMapuiv") )
    fprintf ( xglTraceFd, "  /* glGetPixelMapuiv ( (GLenum)%s, (GLuint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) map ), values ) ;
  if ( xglExecuteIsEnabled("glGetPixelMapuiv") )
    glGetPixelMapuiv ( map, values ) ;
}

void xglGetPixelMapusv ( GLenum map, GLushort* values )
{
  if ( xglTraceIsEnabled("glGetPixelMapusv") )
    fprintf ( xglTraceFd, "  /* glGetPixelMapusv ( (GLenum)%s, (GLushort *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) map ), values ) ;
  if ( xglExecuteIsEnabled("glGetPixelMapusv") )
    glGetPixelMapusv ( map, values ) ;
}

void xglGetPointervEXT ( GLenum pname, void** params )
{
  if ( xglTraceIsEnabled("glGetPointervEXT") )
    fprintf ( xglTraceFd, "  /* glGetPointervEXT ( (GLenum)%s, (void **)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
#ifdef GL_VERSION_1_1
    glGetPointerv ( pname, params ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glGetPointervEXT") )
    glGetPointervEXT ( pname, params ) ;
#else
  fprintf ( xglTraceFd, "  glGetPointervEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglGetPolygonStipple ( GLubyte* mask )
{
  if ( xglTraceIsEnabled("glGetPolygonStipple") )
    fprintf ( xglTraceFd, "  /* glGetPolygonStipple ( (GLubyte *)0x%08x ) ; */\n" , mask ) ;
  if ( xglExecuteIsEnabled("glGetPolygonStipple") )
    glGetPolygonStipple ( mask ) ;
}

void xglGetTexEnvfv ( GLenum target, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetTexEnvfv") )
    fprintf ( xglTraceFd, "  /* glGetTexEnvfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexEnvfv") )
    glGetTexEnvfv ( target, pname, params ) ;
}

void xglGetTexEnviv ( GLenum target, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetTexEnviv") )
    fprintf ( xglTraceFd, "  /* glGetTexEnviv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexEnviv") )
    glGetTexEnviv ( target, pname, params ) ;
}

void xglGetTexGendv ( GLenum coord, GLenum pname, GLdouble* params )
{
  if ( xglTraceIsEnabled("glGetTexGendv") )
    fprintf ( xglTraceFd, "  /* glGetTexGendv ( (GLenum)%s, (GLenum)%s, (GLdouble *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexGendv") )
    glGetTexGendv ( coord, pname, params ) ;
}

void xglGetTexGenfv ( GLenum coord, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetTexGenfv") )
    fprintf ( xglTraceFd, "  /* glGetTexGenfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexGenfv") )
    glGetTexGenfv ( coord, pname, params ) ;
}

void xglGetTexGeniv ( GLenum coord, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetTexGeniv") )
    fprintf ( xglTraceFd, "  /* glGetTexGeniv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexGeniv") )
    glGetTexGeniv ( coord, pname, params ) ;
}

void xglGetTexImage ( GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels )
{
  if ( xglTraceIsEnabled("glGetTexImage") )
    fprintf ( xglTraceFd, "  /* glGetTexImage ( (GLenum)%s, (GLint)%d, (GLenum)%s, (GLenum)%s, (GLvoid *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), level, xglExpandGLenum ( (GLenum) format ), xglExpandGLenum ( (GLenum) type ), pixels ) ;
  if ( xglExecuteIsEnabled("glGetTexImage") )
    glGetTexImage ( target, level, format, type, pixels ) ;
}

void xglGetTexLevelParameterfv ( GLenum target, GLint level, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetTexLevelParameterfv") )
    fprintf ( xglTraceFd, "  /* glGetTexLevelParameterfv ( (GLenum)%s, (GLint)%d, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), level, xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexLevelParameterfv") )
    glGetTexLevelParameterfv ( target, level, pname, params ) ;
}

void xglGetTexLevelParameteriv ( GLenum target, GLint level, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetTexLevelParameteriv") )
    fprintf ( xglTraceFd, "  /* glGetTexLevelParameteriv ( (GLenum)%s, (GLint)%d, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), level, xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexLevelParameteriv") )
    glGetTexLevelParameteriv ( target, level, pname, params ) ;
}

void xglGetTexParameterfv ( GLenum target, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glGetTexParameterfv") )
    fprintf ( xglTraceFd, "  /* glGetTexParameterfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexParameterfv") )
    glGetTexParameterfv ( target, pname, params ) ;
}

void xglGetTexParameteriv ( GLenum target, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glGetTexParameteriv") )
    fprintf ( xglTraceFd, "  /* glGetTexParameteriv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ; */\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glGetTexParameteriv") )
    glGetTexParameteriv ( target, pname, params ) ;
}

void xglHint ( GLenum target, GLenum mode )
{
  if ( xglTraceIsEnabled("glHint") )
    fprintf ( xglTraceFd, "  glHint ( (GLenum)%s, (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glHint") )
    glHint ( target, mode ) ;
}

void xglIndexMask ( GLuint mask )
{
  if ( xglTraceIsEnabled("glIndexMask") )
    fprintf ( xglTraceFd, "  glIndexMask ( (GLuint)%u ) ;\n" , mask ) ;
  if ( xglExecuteIsEnabled("glIndexMask") )
    glIndexMask ( mask ) ;
}

void xglIndexPointerEXT ( GLenum type, GLsizei stride, GLsizei count, void* ptr )
{
  if ( xglTraceIsEnabled("glIndexPointerEXT") )
    fprintf ( xglTraceFd, "  glIndexPointerEXT ( (GLenum)%s, (GLsizei)%d, (GLsizei)%d, (void *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) type ), stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glIndexPointer ( type, stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glIndexPointerEXT") )
    glIndexPointerEXT ( type, stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glIndexPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglIndexd ( GLdouble c )
{
  if ( xglTraceIsEnabled("glIndexd") )
    fprintf ( xglTraceFd, "  glIndexd ( (GLdouble)%f ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexd") )
    glIndexd ( c ) ;
}

void xglIndexdv ( GLdouble* c )
{
  if ( xglTraceIsEnabled("glIndexdv") )
    fprintf ( xglTraceFd, "  glIndexdv ( (GLdouble *)0x%08x ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexdv") )
    glIndexdv ( c ) ;
}

void xglIndexf ( GLfloat c )
{
  if ( xglTraceIsEnabled("glIndexf") )
    fprintf ( xglTraceFd, "  glIndexf ( (GLfloat)%ff ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexf") )
    glIndexf ( c ) ;
}

void xglIndexfv ( GLfloat* c )
{
  if ( xglTraceIsEnabled("glIndexfv") )
    fprintf ( xglTraceFd, "  glIndexfv ( (GLfloat *)0x%08x ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexfv") )
    glIndexfv ( c ) ;
}

void xglIndexi ( GLint c )
{
  if ( xglTraceIsEnabled("glIndexi") )
    fprintf ( xglTraceFd, "  glIndexi ( (GLint)%d ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexi") )
    glIndexi ( c ) ;
}

void xglIndexiv ( GLint* c )
{
  if ( xglTraceIsEnabled("glIndexiv") )
    fprintf ( xglTraceFd, "  glIndexiv ( (GLint *)0x%08x ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexiv") )
    glIndexiv ( c ) ;
}

void xglIndexs ( GLshort c )
{
  if ( xglTraceIsEnabled("glIndexs") )
    fprintf ( xglTraceFd, "  glIndexs ( (GLshort)%d ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexs") )
    glIndexs ( c ) ;
}

void xglIndexsv ( GLshort* c )
{
  if ( xglTraceIsEnabled("glIndexsv") )
    fprintf ( xglTraceFd, "  glIndexsv ( (GLshort *)0x%08x ) ;\n" , c ) ;
  if ( xglExecuteIsEnabled("glIndexsv") )
    glIndexsv ( c ) ;
}

void xglInitNames (  )
{
  if ( xglTraceIsEnabled("glInitNames") )
    fprintf ( xglTraceFd, "  glInitNames (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glInitNames") )
    glInitNames (  ) ;
}

void xglLightModelf ( GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glLightModelf") )
    fprintf ( xglTraceFd, "  glLightModelf ( (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glLightModelf") )
    glLightModelf ( pname, param ) ;
}

void xglLightModelfv ( GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glLightModelfv") )
    fprintf ( xglTraceFd, "  glLightModelfv ( (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glLightModelfv") )
    glLightModelfv ( pname, params ) ;
}

void xglLightModeli ( GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glLightModeli") )
    fprintf ( xglTraceFd, "  glLightModeli ( (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glLightModeli") )
    glLightModeli ( pname, param ) ;
}

void xglLightModeliv ( GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glLightModeliv") )
    fprintf ( xglTraceFd, "  glLightModeliv ( (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glLightModeliv") )
    glLightModeliv ( pname, params ) ;
}

void xglLightf ( GLenum light, GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glLightf") )
    fprintf ( xglTraceFd, "  glLightf ( (GLenum)%s, (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glLightf") )
    glLightf ( light, pname, param ) ;
}

void xglLightfv ( GLenum light, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glLightfv") )
    fprintf ( xglTraceFd, "  glLightfv ( (GLenum)%s, (GLenum)%s, xglBuild4fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n",
        xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), params[0], params[1], params[2], params[3] ) ;
  if ( xglExecuteIsEnabled("glLightfv") )
    glLightfv ( light, pname, params ) ;
}

void xglLighti ( GLenum light, GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glLighti") )
    fprintf ( xglTraceFd, "  glLighti ( (GLenum)%s, (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glLighti") )
    glLighti ( light, pname, param ) ;
}

void xglLightiv ( GLenum light, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glLightiv") )
    fprintf ( xglTraceFd, "  glLightiv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) light ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glLightiv") )
    glLightiv ( light, pname, params ) ;
}

void xglLineStipple ( GLint factor, GLushort pattern )
{
  if ( xglTraceIsEnabled("glLineStipple") )
    fprintf ( xglTraceFd, "  glLineStipple ( (GLint)%d, (GLushort)%u ) ;\n" , factor, pattern ) ;
  if ( xglExecuteIsEnabled("glLineStipple") )
    glLineStipple ( factor, pattern ) ;
}

void xglLineWidth ( GLfloat width )
{
  if ( xglTraceIsEnabled("glLineWidth") )
    fprintf ( xglTraceFd, "  glLineWidth ( (GLfloat)%ff ) ;\n" , width ) ;
  if ( xglExecuteIsEnabled("glLineWidth") )
    glLineWidth ( width ) ;
}

void xglListBase ( GLuint base )
{
  if ( xglTraceIsEnabled("glListBase") )
    fprintf ( xglTraceFd, "  glListBase ( (GLuint)%u ) ;\n" , base ) ;
  if ( xglExecuteIsEnabled("glListBase") )
    glListBase ( base ) ;
}

void xglLoadIdentity (  )
{
  if ( xglTraceIsEnabled("glLoadIdentity") )
    fprintf ( xglTraceFd, "  glLoadIdentity (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glLoadIdentity") )
    glLoadIdentity (  ) ;
}

void xglLoadMatrixd ( GLdouble* m )
{
  if ( xglTraceIsEnabled("glLoadMatrixd") )
  {
    fprintf ( xglTraceFd, "  glLoadMatrixd ( xglBuildMatrixd(%f,%f,%f,%f,\n"    , m[ 0],m[ 1],m[ 2],m[ 3] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f,\n"    , m[ 4],m[ 5],m[ 6],m[ 7] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f,\n"    , m[ 8],m[ 9],m[10],m[11] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f) ) ;\n", m[12],m[13],m[14],m[15] ) ;
  }

  if ( xglExecuteIsEnabled("glLoadMatrixd") )
    glLoadMatrixd ( m ) ;
}

void xglLoadMatrixf ( GLfloat* m )
{
  if ( xglTraceIsEnabled("glLoadMatrixf") )
  {
    fprintf ( xglTraceFd, "  glLoadMatrixf ( xglBuildMatrixf(%ff,%ff,%ff,%ff,\n"    , m[ 0],m[ 1],m[ 2],m[ 3] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff,\n"    , m[ 4],m[ 5],m[ 6],m[ 7] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff,\n"    , m[ 8],m[ 9],m[10],m[11] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff) ) ;\n", m[12],m[13],m[14],m[15] ) ;
  }

  if ( xglExecuteIsEnabled("glLoadMatrixf") )
    glLoadMatrixf ( m ) ;
}

void xglLoadName ( GLuint name )
{
  if ( xglTraceIsEnabled("glLoadName") )
    fprintf ( xglTraceFd, "  glLoadName ( (GLuint)%u ) ;\n" , name ) ;
  if ( xglExecuteIsEnabled("glLoadName") )
    glLoadName ( name ) ;
}

void xglLogicOp ( GLenum opcode )
{
  if ( xglTraceIsEnabled("glLogicOp") )
    fprintf ( xglTraceFd, "  glLogicOp ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) opcode ) ) ;
  if ( xglExecuteIsEnabled("glLogicOp") )
    glLogicOp ( opcode ) ;
}

void xglMap1d ( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, GLdouble* points )
{
  if ( xglTraceIsEnabled("glMap1d") )
    fprintf ( xglTraceFd, "  glMap1d ( (GLenum)%s, (GLdouble)%f, (GLdouble)%f, (GLint)%d, (GLint)%d, (GLdouble *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), u1, u2, stride, order, points ) ;
  if ( xglExecuteIsEnabled("glMap1d") )
    glMap1d ( target, u1, u2, stride, order, points ) ;
}

void xglMap1f ( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, GLfloat* points )
{
  if ( xglTraceIsEnabled("glMap1f") )
    fprintf ( xglTraceFd, "  glMap1f ( (GLenum)%s, (GLfloat)%ff, (GLfloat)%ff, (GLint)%d, (GLint)%d, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), u1, u2, stride, order, points ) ;
  if ( xglExecuteIsEnabled("glMap1f") )
    glMap1f ( target, u1, u2, stride, order, points ) ;
}

void xglMap2d ( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble* points )
{
  if ( xglTraceIsEnabled("glMap2d") )
    fprintf ( xglTraceFd, "  glMap2d ( (GLenum)%s, (GLdouble)%f, (GLdouble)%f, (GLint)%d, (GLint)%d, (GLdouble)%f, (GLdouble)%f, (GLint)%d, (GLint)%d, (GLdouble *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), u1, u2, ustride, uorder, v1, v2, vstride, vorder, points ) ;
  if ( xglExecuteIsEnabled("glMap2d") )
    glMap2d ( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points ) ;
}

void xglMap2f ( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat* points )
{
  if ( xglTraceIsEnabled("glMap2f") )
    fprintf ( xglTraceFd, "  glMap2f ( (GLenum)%s, (GLfloat)%ff, (GLfloat)%ff, (GLint)%d, (GLint)%d, (GLfloat)%ff, (GLfloat)%ff, (GLint)%d, (GLint)%d, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), u1, u2, ustride, uorder, v1, v2, vstride, vorder, points ) ;
  if ( xglExecuteIsEnabled("glMap2f") )
    glMap2f ( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points ) ;
}

void xglMapGrid1d ( GLint un, GLdouble u1, GLdouble u2 )
{
  if ( xglTraceIsEnabled("glMapGrid1d") )
    fprintf ( xglTraceFd, "  glMapGrid1d ( (GLint)%d, (GLdouble)%f, (GLdouble)%f ) ;\n" , un, u1, u2 ) ;
  if ( xglExecuteIsEnabled("glMapGrid1d") )
    glMapGrid1d ( un, u1, u2 ) ;
}

void xglMapGrid1f ( GLint un, GLfloat u1, GLfloat u2 )
{
  if ( xglTraceIsEnabled("glMapGrid1f") )
    fprintf ( xglTraceFd, "  glMapGrid1f ( (GLint)%d, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , un, u1, u2 ) ;
  if ( xglExecuteIsEnabled("glMapGrid1f") )
    glMapGrid1f ( un, u1, u2 ) ;
}

void xglMapGrid2d ( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 )
{
  if ( xglTraceIsEnabled("glMapGrid2d") )
    fprintf ( xglTraceFd, "  glMapGrid2d ( (GLint)%d, (GLdouble)%f, (GLdouble)%f, (GLint)%d, (GLdouble)%f, (GLdouble)%f ) ;\n" , un, u1, u2, vn, v1, v2 ) ;
  if ( xglExecuteIsEnabled("glMapGrid2d") )
    glMapGrid2d ( un, u1, u2, vn, v1, v2 ) ;
}

void xglMapGrid2f ( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 )
{
  if ( xglTraceIsEnabled("glMapGrid2f") )
    fprintf ( xglTraceFd, "  glMapGrid2f ( (GLint)%d, (GLfloat)%ff, (GLfloat)%ff, (GLint)%d, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , un, u1, u2, vn, v1, v2 ) ;
  if ( xglExecuteIsEnabled("glMapGrid2f") )
    glMapGrid2f ( un, u1, u2, vn, v1, v2 ) ;
}

void xglMaterialf ( GLenum face, GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glMaterialf") )
    fprintf ( xglTraceFd, "  glMaterialf ( (GLenum)%s, (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glMaterialf") )
    glMaterialf ( face, pname, param ) ;
}

void xglMaterialfv ( GLenum face, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glMaterialfv") )
    fprintf ( xglTraceFd, "  glMaterialfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glMaterialfv") )
    glMaterialfv ( face, pname, params ) ;
}

void xglMateriali ( GLenum face, GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glMateriali") )
    fprintf ( xglTraceFd, "  glMateriali ( (GLenum)%s, (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glMateriali") )
    glMateriali ( face, pname, param ) ;
}

void xglMaterialiv ( GLenum face, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glMaterialiv") )
    fprintf ( xglTraceFd, "  glMaterialiv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glMaterialiv") )
    glMaterialiv ( face, pname, params ) ;
}

void xglMatrixMode ( GLenum mode )
{
  if ( xglTraceIsEnabled("glMatrixMode") )
    fprintf ( xglTraceFd, "  glMatrixMode ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glMatrixMode") )
    glMatrixMode ( mode ) ;
}


void xglMultMatrixd ( GLdouble* m )
{
  if ( xglTraceIsEnabled("glMultMatrixd") )
  {
    fprintf ( xglTraceFd, "  glMultMatrixd ( xglBuildMatrixd(%f,%f,%f,%f,\n"    , m[ 0],m[ 1],m[ 2],m[ 3] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f,\n"    , m[ 4],m[ 5],m[ 6],m[ 7] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f,\n"    , m[ 8],m[ 9],m[10],m[11] ) ;
    fprintf ( xglTraceFd, "                                  %f,%f,%f,%f) ) ;\n", m[12],m[13],m[14],m[15] ) ;
  }

  if ( xglExecuteIsEnabled("glMultMatrixd") )
    glMultMatrixd ( m ) ;
}

void xglMultMatrixf ( GLfloat* m )
{
  if ( xglTraceIsEnabled("glMultMatrixf") )
  {
    fprintf ( xglTraceFd, "  glMultMatrixf ( xglBuildMatrixf(%ff,%ff,%ff,%ff,\n"    , m[ 0],m[ 1],m[ 2],m[ 3] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff,\n"    , m[ 4],m[ 5],m[ 6],m[ 7] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff,\n"    , m[ 8],m[ 9],m[10],m[11] ) ;
    fprintf ( xglTraceFd, "                                  %ff,%ff,%ff,%ff) ) ;\n", m[12],m[13],m[14],m[15] ) ;
  }

  if ( xglExecuteIsEnabled("glMultMatrixf") )
    glMultMatrixf ( m ) ;
}

void xglNewList ( GLuint list, GLenum mode )
{
  if ( xglTraceIsEnabled("glNewList") )
    fprintf ( xglTraceFd, "  glNewList ( (GLuint)%u, (GLenum)%s ) ;\n" , list, xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glNewList") )
    glNewList ( list, mode ) ;
}

void xglNormal3b ( GLbyte nx, GLbyte ny, GLbyte nz )
{
  if ( xglTraceIsEnabled("glNormal3b") )
    fprintf ( xglTraceFd, "  glNormal3b ( (GLbyte)%d, (GLbyte)%d, (GLbyte)%d ) ;\n" , nx, ny, nz ) ;
  if ( xglExecuteIsEnabled("glNormal3b") )
    glNormal3b ( nx, ny, nz ) ;
}

void xglNormal3bv ( GLbyte* v )
{
  if ( xglTraceIsEnabled("glNormal3bv") )
    fprintf ( xglTraceFd, "  glNormal3bv ( xglBuild3bv((GLbyte)%d,(GLbyte)%d,(GLbyte)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glNormal3bv") )
    glNormal3bv ( v ) ;
}

void xglNormal3d ( GLdouble nx, GLdouble ny, GLdouble nz )
{
  if ( xglTraceIsEnabled("glNormal3d") )
    fprintf ( xglTraceFd, "  glNormal3d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , nx, ny, nz ) ;
  if ( xglExecuteIsEnabled("glNormal3d") )
    glNormal3d ( nx, ny, nz ) ;
}

void xglNormal3dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glNormal3dv") )
    fprintf ( xglTraceFd, "  glNormal3dv ( xglBuild3dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glNormal3dv") )
    glNormal3dv ( v ) ;
}

void xglNormal3f ( GLfloat nx, GLfloat ny, GLfloat nz )
{
  if ( xglTraceIsEnabled("glNormal3f") )
    fprintf ( xglTraceFd, "  glNormal3f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , nx, ny, nz ) ;
  if ( xglExecuteIsEnabled("glNormal3f") )
    glNormal3f ( nx, ny, nz ) ;
}

void xglNormal3fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glNormal3fv") )
    fprintf ( xglTraceFd, "  glNormal3fv ( xglBuild3fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glNormal3fv") )
    glNormal3fv ( v ) ;
}

void xglNormal3i ( GLint nx, GLint ny, GLint nz )
{
  if ( xglTraceIsEnabled("glNormal3i") )
    fprintf ( xglTraceFd, "  glNormal3i ( (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , nx, ny, nz ) ;
  if ( xglExecuteIsEnabled("glNormal3i") )
    glNormal3i ( nx, ny, nz ) ;
}

void xglNormal3iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glNormal3iv") )
    fprintf ( xglTraceFd, "  glNormal3iv ( xglBuild3iv((GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glNormal3iv") )
    glNormal3iv ( v ) ;
}

void xglNormal3s ( GLshort nx, GLshort ny, GLshort nz )
{
  if ( xglTraceIsEnabled("glNormal3s") )
    fprintf ( xglTraceFd, "  glNormal3s ( (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , nx, ny, nz ) ;
  if ( xglExecuteIsEnabled("glNormal3s") )
    glNormal3s ( nx, ny, nz ) ;
}

void xglNormal3sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glNormal3sv") )
    fprintf ( xglTraceFd, "  glNormal3sv ( xglBuild3sv((GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glNormal3sv") )
    glNormal3sv ( v ) ;
}

void xglNormalPointerEXT ( GLenum type, GLsizei stride, GLsizei count, void* ptr )
{
  if ( xglTraceIsEnabled("glNormalPointerEXT") )
    fprintf ( xglTraceFd, "  glNormalPointerEXT ( (GLenum)%s, (GLsizei)%d, (GLsizei)%d, (void *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) type ), stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glNormalPointer ( type, stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glNormalPointerEXT") )
    glNormalPointerEXT ( type, stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glNormalPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglOrtho ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val )
{
  if ( xglTraceIsEnabled("glOrtho") )
    fprintf ( xglTraceFd, "  glOrtho ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , left, right, bottom, top, near_val, far_val ) ;
  if ( xglExecuteIsEnabled("glOrtho") )
    glOrtho ( left, right, bottom, top, near_val, far_val ) ;
}

void xglPassThrough ( GLfloat token )
{
  if ( xglTraceIsEnabled("glPassThrough") )
    fprintf ( xglTraceFd, "  glPassThrough ( (GLfloat)%ff ) ;\n" , token ) ;
  if ( xglExecuteIsEnabled("glPassThrough") )
    glPassThrough ( token ) ;
}

void xglPixelMapfv ( GLenum map, GLint mapsize, GLfloat* values )
{
  if ( xglTraceIsEnabled("glPixelMapfv") )
    fprintf ( xglTraceFd, "  glPixelMapfv ( (GLenum)%s, (GLint)%d, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) map ), mapsize, values ) ;
  if ( xglExecuteIsEnabled("glPixelMapfv") )
    glPixelMapfv ( map, mapsize, values ) ;
}

void xglPixelMapuiv ( GLenum map, GLint mapsize, GLuint* values )
{
  if ( xglTraceIsEnabled("glPixelMapuiv") )
    fprintf ( xglTraceFd, "  glPixelMapuiv ( (GLenum)%s, (GLint)%d, (GLuint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) map ), mapsize, values ) ;
  if ( xglExecuteIsEnabled("glPixelMapuiv") )
    glPixelMapuiv ( map, mapsize, values ) ;
}

void xglPixelMapusv ( GLenum map, GLint mapsize, GLushort* values )
{
  if ( xglTraceIsEnabled("glPixelMapusv") )
    fprintf ( xglTraceFd, "  glPixelMapusv ( (GLenum)%s, (GLint)%d, (GLushort *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) map ), mapsize, values ) ;
  if ( xglExecuteIsEnabled("glPixelMapusv") )
    glPixelMapusv ( map, mapsize, values ) ;
}

void xglPixelStoref ( GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glPixelStoref") )
    fprintf ( xglTraceFd, "  glPixelStoref ( (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glPixelStoref") )
    glPixelStoref ( pname, param ) ;
}

void xglPixelStorei ( GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glPixelStorei") )
    fprintf ( xglTraceFd, "  glPixelStorei ( (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glPixelStorei") )
    glPixelStorei ( pname, param ) ;
}

void xglPixelTransferf ( GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glPixelTransferf") )
    fprintf ( xglTraceFd, "  glPixelTransferf ( (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glPixelTransferf") )
    glPixelTransferf ( pname, param ) ;
}

void xglPixelTransferi ( GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glPixelTransferi") )
    fprintf ( xglTraceFd, "  glPixelTransferi ( (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glPixelTransferi") )
    glPixelTransferi ( pname, param ) ;
}

void xglPixelZoom ( GLfloat xfactor, GLfloat yfactor )
{
  if ( xglTraceIsEnabled("glPixelZoom") )
    fprintf ( xglTraceFd, "  glPixelZoom ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , xfactor, yfactor ) ;
  if ( xglExecuteIsEnabled("glPixelZoom") )
    glPixelZoom ( xfactor, yfactor ) ;
}

void xglPointSize ( GLfloat size )
{
  if ( xglTraceIsEnabled("glPointSize") )
    fprintf ( xglTraceFd, "  glPointSize ( (GLfloat)%ff ) ;\n" , size ) ;
  if ( xglExecuteIsEnabled("glPointSize") )
    glPointSize ( size ) ;
}

void xglPolygonMode ( GLenum face, GLenum mode )
{
  if ( xglTraceIsEnabled("glPolygonMode") )
    fprintf ( xglTraceFd, "  glPolygonMode ( (GLenum)%s, (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) face ), xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glPolygonMode") )
    glPolygonMode ( face, mode ) ;
}

void xglPolygonOffsetEXT ( GLfloat factor, GLfloat bias )
{
  if ( xglTraceIsEnabled("glPolygonOffsetEXT") )
    fprintf ( xglTraceFd, "  glPolygonOffsetEXT ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , factor, bias ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glPolygonOffsetEXT") )
    glPolygonOffset ( factor, bias ) ;
#else
#ifdef GL_EXT_polygon_offset
  if ( xglExecuteIsEnabled("glPolygonOffsetEXT") )
    glPolygonOffsetEXT ( factor, bias ) ;
#else
  fprintf ( xglTraceFd, "  glPolygonOffsetEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglPolygonOffset ( GLfloat factor, GLfloat bias )
{
  if ( xglTraceIsEnabled("glPolygonOffset") )
    fprintf ( xglTraceFd, "  glPolygonOffset ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , factor, bias ) ;
#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glPolygonOffset") )
    glPolygonOffset ( factor, bias ) ;
#else
#ifdef GL_EXT_polygon_offset
  if ( xglExecuteIsEnabled("glPolygonOffset") )
    glPolygonOffsetEXT ( factor, bias ) ;
#else
  fprintf ( xglTraceFd, "  glPolygonOffsetEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglPolygonStipple ( GLubyte* mask )
{
  if ( xglTraceIsEnabled("glPolygonStipple") )
    fprintf ( xglTraceFd, "  glPolygonStipple ( (GLubyte *)0x%08x ) ;\n" , mask ) ;
  if ( xglExecuteIsEnabled("glPolygonStipple") )
    glPolygonStipple ( mask ) ;
}

void xglPopAttrib (  )
{
  if ( xglTraceIsEnabled("glPopAttrib") )
    fprintf ( xglTraceFd, "  glPopAttrib (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glPopAttrib") )
    glPopAttrib (  ) ;
}

void xglPopMatrix (  )
{
  if ( xglTraceIsEnabled("glPopMatrix") )
    fprintf ( xglTraceFd, "  glPopMatrix (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glPopMatrix") )
    glPopMatrix (  ) ;
}

void xglPopName (  )
{
  if ( xglTraceIsEnabled("glPopName") )
    fprintf ( xglTraceFd, "  glPopName (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glPopName") )
    glPopName (  ) ;
}

void xglPushAttrib ( GLbitfield mask )
{
  if ( xglTraceIsEnabled("glPushAttrib") )
    fprintf ( xglTraceFd, "  glPushAttrib ( (GLbitfield)0x%08x ) ;\n" , mask ) ;
  if ( xglExecuteIsEnabled("glPushAttrib") )
    glPushAttrib ( mask ) ;
}

void xglPushMatrix (  )
{
  if ( xglTraceIsEnabled("glPushMatrix") )
    fprintf ( xglTraceFd, "  glPushMatrix (  ) ;\n"  ) ;
  if ( xglExecuteIsEnabled("glPushMatrix") )
    glPushMatrix (  ) ;
}

void xglPushName ( GLuint name )
{
  if ( xglTraceIsEnabled("glPushName") )
    fprintf ( xglTraceFd, "  glPushName ( (GLuint)%u ) ;\n" , name ) ;
  if ( xglExecuteIsEnabled("glPushName") )
    glPushName ( name ) ;
}

void xglRasterPos2d ( GLdouble x, GLdouble y )
{
  if ( xglTraceIsEnabled("glRasterPos2d") )
    fprintf ( xglTraceFd, "  glRasterPos2d ( (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glRasterPos2d") )
    glRasterPos2d ( x, y ) ;
}

void xglRasterPos2dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glRasterPos2dv") )
    fprintf ( xglTraceFd, "  glRasterPos2dv ( xglBuild2dv((GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glRasterPos2dv") )
    glRasterPos2dv ( v ) ;
}

void xglRasterPos2f ( GLfloat x, GLfloat y )
{
  if ( xglTraceIsEnabled("glRasterPos2f") )
    fprintf ( xglTraceFd, "  glRasterPos2f ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glRasterPos2f") )
    glRasterPos2f ( x, y ) ;
}

void xglRasterPos2fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glRasterPos2fv") )
    fprintf ( xglTraceFd, "  glRasterPos2fv ( xglBuild2fv((GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glRasterPos2fv") )
    glRasterPos2fv ( v ) ;
}

void xglRasterPos2i ( GLint x, GLint y )
{
  if ( xglTraceIsEnabled("glRasterPos2i") )
    fprintf ( xglTraceFd, "  glRasterPos2i ( (GLint)%d, (GLint)%d ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glRasterPos2i") )
    glRasterPos2i ( x, y ) ;
}

void xglRasterPos2iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glRasterPos2iv") )
    fprintf ( xglTraceFd, "  glRasterPos2iv ( xglBuild2iv((GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glRasterPos2iv") )
    glRasterPos2iv ( v ) ;
}

void xglRasterPos2s ( GLshort x, GLshort y )
{
  if ( xglTraceIsEnabled("glRasterPos2s") )
    fprintf ( xglTraceFd, "  glRasterPos2s ( (GLshort)%d, (GLshort)%d ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glRasterPos2s") )
    glRasterPos2s ( x, y ) ;
}

void xglRasterPos2sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glRasterPos2sv") )
    fprintf ( xglTraceFd, "  glRasterPos2sv ( xglBuild2sv((GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glRasterPos2sv") )
    glRasterPos2sv ( v ) ;
}

void xglRasterPos3d ( GLdouble x, GLdouble y, GLdouble z )
{
  if ( xglTraceIsEnabled("glRasterPos3d") )
    fprintf ( xglTraceFd, "  glRasterPos3d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glRasterPos3d") )
    glRasterPos3d ( x, y, z ) ;
}

void xglRasterPos3dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glRasterPos3dv") )
    fprintf ( xglTraceFd, "  glRasterPos3dv ( xglBuild3dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glRasterPos3dv") )
    glRasterPos3dv ( v ) ;
}

void xglRasterPos3f ( GLfloat x, GLfloat y, GLfloat z )
{
  if ( xglTraceIsEnabled("glRasterPos3f") )
    fprintf ( xglTraceFd, "  glRasterPos3f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glRasterPos3f") )
    glRasterPos3f ( x, y, z ) ;
}

void xglRasterPos3fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glRasterPos3fv") )
    fprintf ( xglTraceFd, "  glRasterPos3fv ( xglBuild3fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glRasterPos3fv") )
    glRasterPos3fv ( v ) ;
}

void xglRasterPos3i ( GLint x, GLint y, GLint z )
{
  if ( xglTraceIsEnabled("glRasterPos3i") )
    fprintf ( xglTraceFd, "  glRasterPos3i ( (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glRasterPos3i") )
    glRasterPos3i ( x, y, z ) ;
}

void xglRasterPos3iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glRasterPos3iv") )
    fprintf ( xglTraceFd, "  glRasterPos3iv ( xglBuild3iv((GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glRasterPos3iv") )
    glRasterPos3iv ( v ) ;
}

void xglRasterPos3s ( GLshort x, GLshort y, GLshort z )
{
  if ( xglTraceIsEnabled("glRasterPos3s") )
    fprintf ( xglTraceFd, "  glRasterPos3s ( (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glRasterPos3s") )
    glRasterPos3s ( x, y, z ) ;
}

void xglRasterPos3sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glRasterPos3sv") )
    fprintf ( xglTraceFd, "  glRasterPos3sv ( xglBuild3sv((GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glRasterPos3sv") )
    glRasterPos3sv ( v ) ;
}

void xglRasterPos4d ( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  if ( xglTraceIsEnabled("glRasterPos4d") )
    fprintf ( xglTraceFd, "  glRasterPos4d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glRasterPos4d") )
    glRasterPos4d ( x, y, z, w ) ;
}

void xglRasterPos4dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glRasterPos4dv") )
    fprintf ( xglTraceFd, "  glRasterPos4dv ( xglBuild4dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glRasterPos4dv") )
    glRasterPos4dv ( v ) ;
}

void xglRasterPos4f ( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  if ( xglTraceIsEnabled("glRasterPos4f") )
    fprintf ( xglTraceFd, "  glRasterPos4f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glRasterPos4f") )
    glRasterPos4f ( x, y, z, w ) ;
}

void xglRasterPos4fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glRasterPos4fv") )
    fprintf ( xglTraceFd, "  glRasterPos4fv ( xglBuild4fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glRasterPos4fv") )
    glRasterPos4fv ( v ) ;
}

void xglRasterPos4i ( GLint x, GLint y, GLint z, GLint w )
{
  if ( xglTraceIsEnabled("glRasterPos4i") )
    fprintf ( xglTraceFd, "  glRasterPos4i ( (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glRasterPos4i") )
    glRasterPos4i ( x, y, z, w ) ;
}

void xglRasterPos4iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glRasterPos4iv") )
    fprintf ( xglTraceFd, "  glRasterPos4iv ( xglBuild4iv((GLint)%d,(GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glRasterPos4iv") )
    glRasterPos4iv ( v ) ;
}

void xglRasterPos4s ( GLshort x, GLshort y, GLshort z, GLshort w )
{
  if ( xglTraceIsEnabled("glRasterPos4s") )
    fprintf ( xglTraceFd, "  glRasterPos4s ( (GLshort)%d, (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glRasterPos4s") )
    glRasterPos4s ( x, y, z, w ) ;
}

void xglRasterPos4sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glRasterPos4sv") )
    fprintf ( xglTraceFd, "  glRasterPos4sv ( xglBuild4sv((GLshort)%d,(GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glRasterPos4sv") )
    glRasterPos4sv ( v ) ;
}

void xglReadBuffer ( GLenum mode )
{
  if ( xglTraceIsEnabled("glReadBuffer") )
    fprintf ( xglTraceFd, "  glReadBuffer ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glReadBuffer") )
    glReadBuffer ( mode ) ;
}

void xglReadPixels ( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels )
{
  if ( xglTraceIsEnabled("glReadPixels") )
    fprintf ( xglTraceFd, "  glReadPixels ( (GLint)%d, (GLint)%d, (GLsizei)%d, (GLsizei)%d, (GLenum)%s, (GLenum)%s, (GLvoid *)0x%08x ) ;\n" , x, y, width, height, xglExpandGLenum ( (GLenum) format ), xglExpandGLenum ( (GLenum) type ), pixels ) ;
  if ( xglExecuteIsEnabled("glReadPixels") )
    glReadPixels ( x, y, width, height, format, type, pixels ) ;
}

void xglRectd ( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 )
{
  if ( xglTraceIsEnabled("glRectd") )
    fprintf ( xglTraceFd, "  glRectd ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x1, y1, x2, y2 ) ;
  if ( xglExecuteIsEnabled("glRectd") )
    glRectd ( x1, y1, x2, y2 ) ;
}

void xglRectdv ( GLdouble* v1, GLdouble* v2 )
{
  if ( xglTraceIsEnabled("glRectdv") )
    fprintf ( xglTraceFd, "  glRectdv ( (GLdouble *)0x%08x, (GLdouble *)0x%08x ) ;\n" , v1, v2 ) ;
  if ( xglExecuteIsEnabled("glRectdv") )
    glRectdv ( v1, v2 ) ;
}

void xglRectf ( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
  if ( xglTraceIsEnabled("glRectf") )
    fprintf ( xglTraceFd, "  glRectf ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x1, y1, x2, y2 ) ;
  if ( xglExecuteIsEnabled("glRectf") )
    glRectf ( x1, y1, x2, y2 ) ;
}

void xglRectfv ( GLfloat* v1, GLfloat* v2 )
{
  if ( xglTraceIsEnabled("glRectfv") )
    fprintf ( xglTraceFd, "  glRectfv ( (GLfloat *)0x%08x, (GLfloat *)0x%08x ) ;\n" , v1, v2 ) ;
  if ( xglExecuteIsEnabled("glRectfv") )
    glRectfv ( v1, v2 ) ;
}

void xglRecti ( GLint x1, GLint y1, GLint x2, GLint y2 )
{
  if ( xglTraceIsEnabled("glRecti") )
    fprintf ( xglTraceFd, "  glRecti ( (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , x1, y1, x2, y2 ) ;
  if ( xglExecuteIsEnabled("glRecti") )
    glRecti ( x1, y1, x2, y2 ) ;
}

void xglRectiv ( GLint* v1, GLint* v2 )
{
  if ( xglTraceIsEnabled("glRectiv") )
    fprintf ( xglTraceFd, "  glRectiv ( (GLint *)0x%08x, (GLint *)0x%08x ) ;\n" , v1, v2 ) ;
  if ( xglExecuteIsEnabled("glRectiv") )
    glRectiv ( v1, v2 ) ;
}

void xglRects ( GLshort x1, GLshort y1, GLshort x2, GLshort y2 )
{
  if ( xglTraceIsEnabled("glRects") )
    fprintf ( xglTraceFd, "  glRects ( (GLshort)%d, (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , x1, y1, x2, y2 ) ;
  if ( xglExecuteIsEnabled("glRects") )
    glRects ( x1, y1, x2, y2 ) ;
}

void xglRectsv ( GLshort* v1, GLshort* v2 )
{
  if ( xglTraceIsEnabled("glRectsv") )
    fprintf ( xglTraceFd, "  glRectsv ( (GLshort *)0x%08x, (GLshort *)0x%08x ) ;\n" , v1, v2 ) ;
  if ( xglExecuteIsEnabled("glRectsv") )
    glRectsv ( v1, v2 ) ;
}

void xglRotated ( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
  if ( xglTraceIsEnabled("glRotated") )
    fprintf ( xglTraceFd, "  glRotated ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , angle, x, y, z ) ;
  if ( xglExecuteIsEnabled("glRotated") )
    glRotated ( angle, x, y, z ) ;
}

void xglRotatef ( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
  if ( xglTraceIsEnabled("glRotatef") )
    fprintf ( xglTraceFd, "  glRotatef ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , angle, x, y, z ) ;
  if ( xglExecuteIsEnabled("glRotatef") )
    glRotatef ( angle, x, y, z ) ;
}

void xglScaled ( GLdouble x, GLdouble y, GLdouble z )
{
  if ( xglTraceIsEnabled("glScaled") )
    fprintf ( xglTraceFd, "  glScaled ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glScaled") )
    glScaled ( x, y, z ) ;
}

void xglScalef ( GLfloat x, GLfloat y, GLfloat z )
{
  if ( xglTraceIsEnabled("glScalef") )
    fprintf ( xglTraceFd, "  glScalef ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glScalef") )
    glScalef ( x, y, z ) ;
}

void xglScissor ( GLint x, GLint y, GLsizei width, GLsizei height )
{
  if ( xglTraceIsEnabled("glScissor") )
    fprintf ( xglTraceFd, "  glScissor ( (GLint)%d, (GLint)%d, (GLsizei)%d, (GLsizei)%d ) ;\n" , x, y, width, height ) ;
  if ( xglExecuteIsEnabled("glScissor") )
    glScissor ( x, y, width, height ) ;
}

void xglSelectBuffer ( GLsizei size, GLuint* buffer )
{
  if ( xglTraceIsEnabled("glSelectBuffer") )
    fprintf ( xglTraceFd, "  glSelectBuffer ( (GLsizei)%d, (GLuint *)0x%08x ) ;\n" , size, buffer ) ;
  if ( xglExecuteIsEnabled("glSelectBuffer") )
    glSelectBuffer ( size, buffer ) ;
}

void xglShadeModel ( GLenum mode )
{
  if ( xglTraceIsEnabled("glShadeModel") )
    fprintf ( xglTraceFd, "  glShadeModel ( (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) mode ) ) ;
  if ( xglExecuteIsEnabled("glShadeModel") )
    glShadeModel ( mode ) ;
}

void xglStencilFunc ( GLenum func, GLint ref, GLuint mask )
{
  if ( xglTraceIsEnabled("glStencilFunc") )
    fprintf ( xglTraceFd, "  glStencilFunc ( (GLenum)%s, (GLint)%d, (GLuint)%u ) ;\n" , xglExpandGLenum ( (GLenum) func ), ref, mask ) ;
  if ( xglExecuteIsEnabled("glStencilFunc") )
    glStencilFunc ( func, ref, mask ) ;
}

void xglStencilMask ( GLuint mask )
{
  if ( xglTraceIsEnabled("glStencilMask") )
    fprintf ( xglTraceFd, "  glStencilMask ( (GLuint)%u ) ;\n" , mask ) ;
  if ( xglExecuteIsEnabled("glStencilMask") )
    glStencilMask ( mask ) ;
}

void xglStencilOp ( GLenum fail, GLenum zfail, GLenum zpass )
{
  if ( xglTraceIsEnabled("glStencilOp") )
    fprintf ( xglTraceFd, "  glStencilOp ( (GLenum)%s, (GLenum)%s, (GLenum)%s ) ;\n" , xglExpandGLenum ( (GLenum) fail ), xglExpandGLenum ( (GLenum) zfail ), xglExpandGLenum ( (GLenum) zpass ) ) ;
  if ( xglExecuteIsEnabled("glStencilOp") )
    glStencilOp ( fail, zfail, zpass ) ;
}

void xglTexCoord1d ( GLdouble s )
{
  if ( xglTraceIsEnabled("glTexCoord1d") )
    fprintf ( xglTraceFd, "  glTexCoord1d ( (GLdouble)%f ) ;\n" , s ) ;
  if ( xglExecuteIsEnabled("glTexCoord1d") )
    glTexCoord1d ( s ) ;
}

void xglTexCoord1dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glTexCoord1dv") )
    fprintf ( xglTraceFd, "  glTexCoord1dv ( xglBuild1dv((GLdouble)%f) ) ;\n" , v[0] ) ;
  if ( xglExecuteIsEnabled("glTexCoord1dv") )
    glTexCoord1dv ( v ) ;
}

void xglTexCoord1f ( GLfloat s )
{
  if ( xglTraceIsEnabled("glTexCoord1f") )
    fprintf ( xglTraceFd, "  glTexCoord1f ( (GLfloat)%ff ) ;\n" , s ) ;
  if ( xglExecuteIsEnabled("glTexCoord1f") )
    glTexCoord1f ( s ) ;
}

void xglTexCoord1fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glTexCoord1fv") )
    fprintf ( xglTraceFd, "  glTexCoord1fv ( xglBuild1fv((GLfloat)%ff) ) ;\n" , v[0] ) ;
  if ( xglExecuteIsEnabled("glTexCoord1fv") )
    glTexCoord1fv ( v ) ;
}

void xglTexCoord1i ( GLint s )
{
  if ( xglTraceIsEnabled("glTexCoord1i") )
    fprintf ( xglTraceFd, "  glTexCoord1i ( (GLint)%d ) ;\n" , s ) ;
  if ( xglExecuteIsEnabled("glTexCoord1i") )
    glTexCoord1i ( s ) ;
}

void xglTexCoord1iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glTexCoord1iv") )
    fprintf ( xglTraceFd, "  glTexCoord1iv ( xglBuild1iv((GLint)%d) ) ;\n" , v[0] ) ;
  if ( xglExecuteIsEnabled("glTexCoord1iv") )
    glTexCoord1iv ( v ) ;
}

void xglTexCoord1s ( GLshort s )
{
  if ( xglTraceIsEnabled("glTexCoord1s") )
    fprintf ( xglTraceFd, "  glTexCoord1s ( (GLshort)%d ) ;\n" , s ) ;
  if ( xglExecuteIsEnabled("glTexCoord1s") )
    glTexCoord1s ( s ) ;
}

void xglTexCoord1sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glTexCoord1sv") )
    fprintf ( xglTraceFd, "  glTexCoord1sv ( xglBuild1sv((GLshort)%d) ) ;\n" , v[0] ) ;
  if ( xglExecuteIsEnabled("glTexCoord1sv") )
    glTexCoord1sv ( v ) ;
}

void xglTexCoord2d ( GLdouble s, GLdouble t )
{
  if ( xglTraceIsEnabled("glTexCoord2d") )
    fprintf ( xglTraceFd, "  glTexCoord2d ( (GLdouble)%f, (GLdouble)%f ) ;\n" , s, t ) ;
  if ( xglExecuteIsEnabled("glTexCoord2d") )
    glTexCoord2d ( s, t ) ;
}

void xglTexCoord2dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glTexCoord2dv") )
    fprintf ( xglTraceFd, "  glTexCoord2dv ( xglBuild2dv((GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glTexCoord2dv") )
    glTexCoord2dv ( v ) ;
}

void xglTexCoord2f ( GLfloat s, GLfloat t )
{
  if ( xglTraceIsEnabled("glTexCoord2f") )
    fprintf ( xglTraceFd, "  glTexCoord2f ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , s, t ) ;
  if ( xglExecuteIsEnabled("glTexCoord2f") )
    glTexCoord2f ( s, t ) ;
}

void xglTexCoord2fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glTexCoord2fv") )
    fprintf ( xglTraceFd, "  glTexCoord2fv ( xglBuild2fv((GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glTexCoord2fv") )
    glTexCoord2fv ( v ) ;
}

void xglTexCoord2i ( GLint s, GLint t )
{
  if ( xglTraceIsEnabled("glTexCoord2i") )
    fprintf ( xglTraceFd, "  glTexCoord2i ( (GLint)%d, (GLint)%d ) ;\n" , s, t ) ;
  if ( xglExecuteIsEnabled("glTexCoord2i") )
    glTexCoord2i ( s, t ) ;
}

void xglTexCoord2iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glTexCoord2iv") )
    fprintf ( xglTraceFd, "  glTexCoord2iv ( xglBuild2iv((GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glTexCoord2iv") )
    glTexCoord2iv ( v ) ;
}

void xglTexCoord2s ( GLshort s, GLshort t )
{
  if ( xglTraceIsEnabled("glTexCoord2s") )
    fprintf ( xglTraceFd, "  glTexCoord2s ( (GLshort)%d, (GLshort)%d ) ;\n" , s, t ) ;
  if ( xglExecuteIsEnabled("glTexCoord2s") )
    glTexCoord2s ( s, t ) ;
}

void xglTexCoord2sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glTexCoord2sv") )
    fprintf ( xglTraceFd, "  glTexCoord2sv ( xglBuild2sv((GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glTexCoord2sv") )
    glTexCoord2sv ( v ) ;
}

void xglTexCoord3d ( GLdouble s, GLdouble t, GLdouble r )
{
  if ( xglTraceIsEnabled("glTexCoord3d") )
    fprintf ( xglTraceFd, "  glTexCoord3d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , s, t, r ) ;
  if ( xglExecuteIsEnabled("glTexCoord3d") )
    glTexCoord3d ( s, t, r ) ;
}

void xglTexCoord3dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glTexCoord3dv") )
    fprintf ( xglTraceFd, "  glTexCoord3dv ( xglBuild3dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glTexCoord3dv") )
    glTexCoord3dv ( v ) ;
}

void xglTexCoord3f ( GLfloat s, GLfloat t, GLfloat r )
{
  if ( xglTraceIsEnabled("glTexCoord3f") )
    fprintf ( xglTraceFd, "  glTexCoord3f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , s, t, r ) ;
  if ( xglExecuteIsEnabled("glTexCoord3f") )
    glTexCoord3f ( s, t, r ) ;
}

void xglTexCoord3fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glTexCoord3fv") )
    fprintf ( xglTraceFd, "  glTexCoord3fv ( xglBuild3fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glTexCoord3fv") )
    glTexCoord3fv ( v ) ;
}

void xglTexCoord3i ( GLint s, GLint t, GLint r )
{
  if ( xglTraceIsEnabled("glTexCoord3i") )
    fprintf ( xglTraceFd, "  glTexCoord3i ( (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , s, t, r ) ;
  if ( xglExecuteIsEnabled("glTexCoord3i") )
    glTexCoord3i ( s, t, r ) ;
}

void xglTexCoord3iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glTexCoord3iv") )
    fprintf ( xglTraceFd, "  glTexCoord3iv ( xglBuild3iv((GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glTexCoord3iv") )
    glTexCoord3iv ( v ) ;
}

void xglTexCoord3s ( GLshort s, GLshort t, GLshort r )
{
  if ( xglTraceIsEnabled("glTexCoord3s") )
    fprintf ( xglTraceFd, "  glTexCoord3s ( (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , s, t, r ) ;
  if ( xglExecuteIsEnabled("glTexCoord3s") )
    glTexCoord3s ( s, t, r ) ;
}

void xglTexCoord3sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glTexCoord3sv") )
    fprintf ( xglTraceFd, "  glTexCoord3sv ( xglBuild3sv((GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glTexCoord3sv") )
    glTexCoord3sv ( v ) ;
}

void xglTexCoord4d ( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
  if ( xglTraceIsEnabled("glTexCoord4d") )
    fprintf ( xglTraceFd, "  glTexCoord4d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , s, t, r, q ) ;
  if ( xglExecuteIsEnabled("glTexCoord4d") )
    glTexCoord4d ( s, t, r, q ) ;
}

void xglTexCoord4dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glTexCoord4dv") )
    fprintf ( xglTraceFd, "  glTexCoord4dv ( xglBuild4dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glTexCoord4dv") )
    glTexCoord4dv ( v ) ;
}

void xglTexCoord4f ( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
  if ( xglTraceIsEnabled("glTexCoord4f") )
    fprintf ( xglTraceFd, "  glTexCoord4f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , s, t, r, q ) ;
  if ( xglExecuteIsEnabled("glTexCoord4f") )
    glTexCoord4f ( s, t, r, q ) ;
}

void xglTexCoord4fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glTexCoord4fv") )
    fprintf ( xglTraceFd, "  glTexCoord4fv ( xglBuild4fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glTexCoord4fv") )
    glTexCoord4fv ( v ) ;
}

void xglTexCoord4i ( GLint s, GLint t, GLint r, GLint q )
{
  if ( xglTraceIsEnabled("glTexCoord4i") )
    fprintf ( xglTraceFd, "  glTexCoord4i ( (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , s, t, r, q ) ;
  if ( xglExecuteIsEnabled("glTexCoord4i") )
    glTexCoord4i ( s, t, r, q ) ;
}

void xglTexCoord4iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glTexCoord4iv") )
    fprintf ( xglTraceFd, "  glTexCoord4iv ( xglBuild4iv((GLint)%d,(GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glTexCoord4iv") )
    glTexCoord4iv ( v ) ;
}

void xglTexCoord4s ( GLshort s, GLshort t, GLshort r, GLshort q )
{
  if ( xglTraceIsEnabled("glTexCoord4s") )
    fprintf ( xglTraceFd, "  glTexCoord4s ( (GLshort)%d, (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , s, t, r, q ) ;
  if ( xglExecuteIsEnabled("glTexCoord4s") )
    glTexCoord4s ( s, t, r, q ) ;
}

void xglTexCoord4sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glTexCoord4sv") )
    fprintf ( xglTraceFd, "  glTexCoord4sv ( xglBuild4sv((GLshort)%d,(GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glTexCoord4sv") )
    glTexCoord4sv ( v ) ;
}

void xglTexCoordPointerEXT ( GLint size, GLenum type, GLsizei stride, GLsizei count, void* ptr )
{
  if ( xglTraceIsEnabled("glTexCoordPointerEXT") )
    fprintf ( xglTraceFd, "  glTexCoordPointerEXT ( (GLint)%d, (GLenum)%s, (GLsizei)%d, (GLsizei)%d, (void *)0x%08x ) ;\n" , size, xglExpandGLenum ( (GLenum) type ), stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glTexCoordPointer ( size, type, stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glTexCoordPointerEXT") )
    glTexCoordPointerEXT ( size, type, stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glTexCoordPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglTexEnvf ( GLenum target, GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glTexEnvf") )
    fprintf ( xglTraceFd, "  glTexEnvf ( (GLenum)%s, (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexEnvf") )
    glTexEnvf ( target, pname, param ) ;
}

void xglTexEnvfv ( GLenum target, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glTexEnvfv") )
    fprintf ( xglTraceFd, "  glTexEnvfv ( (GLenum)%s, (GLenum)%s, xglBuild4fv(%ff,%ff,%ff,%ff) ) ;\n",
                          xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params[0], params[1], params[2], params[3] ) ;
  if ( xglExecuteIsEnabled("glTexEnvfv") )
    glTexEnvfv ( target, pname, params ) ;
}

void xglTexEnvi ( GLenum target, GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glTexEnvi") )
    fprintf ( xglTraceFd, "  glTexEnvi ( (GLenum)%s, (GLenum)%s, (GLint)%s ) ;\n",
            xglExpandGLenum ( (GLenum) target ),
            xglExpandGLenum ( (GLenum) pname ),
            xglExpandGLenum ( (GLenum) param ) ) ;

  if ( xglExecuteIsEnabled("glTexEnvi") )
    glTexEnvi ( target, pname, param ) ;
}

void xglTexEnviv ( GLenum target, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glTexEnviv") )
    fprintf ( xglTraceFd, "  glTexEnviv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexEnviv") )
    glTexEnviv ( target, pname, params ) ;
}

void xglTexGend ( GLenum coord, GLenum pname, GLdouble param )
{
  if ( xglTraceIsEnabled("glTexGend") )
    fprintf ( xglTraceFd, "  glTexGend ( (GLenum)%s, (GLenum)%s, (GLdouble)%f ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexGend") )
    glTexGend ( coord, pname, param ) ;
}

void xglTexGendv ( GLenum coord, GLenum pname, GLdouble* params )
{
  if ( xglTraceIsEnabled("glTexGendv") )
    fprintf ( xglTraceFd, "  glTexGendv ( (GLenum)%s, (GLenum)%s, (GLdouble *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexGendv") )
    glTexGendv ( coord, pname, params ) ;
}

void xglTexGenf ( GLenum coord, GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glTexGenf") )
    fprintf ( xglTraceFd, "  glTexGenf ( (GLenum)%s, (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexGenf") )
    glTexGenf ( coord, pname, param ) ;
}

void xglTexGenfv ( GLenum coord, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glTexGenfv") )
    fprintf ( xglTraceFd, "  glTexGenfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexGenfv") )
    glTexGenfv ( coord, pname, params ) ;
}

void xglTexGeni ( GLenum coord, GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glTexGeni") )
    fprintf ( xglTraceFd, "  glTexGeni ( (GLenum)%s, (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexGeni") )
    glTexGeni ( coord, pname, param ) ;
}

void xglTexGeniv ( GLenum coord, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glTexGeniv") )
    fprintf ( xglTraceFd, "  glTexGeniv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) coord ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexGeniv") )
    glTexGeniv ( coord, pname, params ) ;
}

void xglTexImage1D ( GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid* pixels )
{
  if ( xglTraceIsEnabled("glTexImage1D") )
    fprintf ( xglTraceFd, "  glTexImage1D ( (GLenum)%s, (GLint)%d, (GLint)%d, (GLsizei)%d, (GLint)%d, (GLenum)%s, (GLenum)%s, (GLvoid *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), level, components, width, border, xglExpandGLenum ( (GLenum) format ), xglExpandGLenum ( (GLenum) type ), pixels ) ;
  if ( xglExecuteIsEnabled("glTexImage1D") )
    glTexImage1D ( target, level, components, width, border, format, type, pixels ) ;
}

void xglTexImage2D ( GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid* pixels )
{
  if ( xglTraceIsEnabled("glTexImage2D") )
    fprintf ( xglTraceFd, "  glTexImage2D ( (GLenum)%s, (GLint)%d, (GLint)%d, (GLsizei)%d, (GLsizei)%d, (GLint)%d, (GLenum)%s, (GLenum)%s, (GLvoid *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), level, components, width, height, border, xglExpandGLenum ( (GLenum) format ), xglExpandGLenum ( (GLenum) type ), pixels ) ;
  if ( xglExecuteIsEnabled("glTexImage2D") )
    glTexImage2D ( target, level, components, width, height, border, format, type, pixels ) ;
}

void xglTexParameterf ( GLenum target, GLenum pname, GLfloat param )
{
  if ( xglTraceIsEnabled("glTexParameterf") )
    fprintf ( xglTraceFd, "  glTexParameterf ( (GLenum)%s, (GLenum)%s, (GLfloat)%ff ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexParameterf") )
    glTexParameterf ( target, pname, param ) ;
}

void xglTexParameterfv ( GLenum target, GLenum pname, GLfloat* params )
{
  if ( xglTraceIsEnabled("glTexParameterfv") )
    fprintf ( xglTraceFd, "  glTexParameterfv ( (GLenum)%s, (GLenum)%s, (GLfloat *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexParameterfv") )
    glTexParameterfv ( target, pname, params ) ;
}

void xglTexParameteri ( GLenum target, GLenum pname, GLint param )
{
  if ( xglTraceIsEnabled("glTexParameteri") )
    fprintf ( xglTraceFd, "  glTexParameteri ( (GLenum)%s, (GLenum)%s, (GLint)%d ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), param ) ;
  if ( xglExecuteIsEnabled("glTexParameteri") )
    glTexParameteri ( target, pname, param ) ;
}

void xglTexParameteriv ( GLenum target, GLenum pname, GLint* params )
{
  if ( xglTraceIsEnabled("glTexParameteriv") )
    fprintf ( xglTraceFd, "  glTexParameteriv ( (GLenum)%s, (GLenum)%s, (GLint *)0x%08x ) ;\n" , xglExpandGLenum ( (GLenum) target ), xglExpandGLenum ( (GLenum) pname ), params ) ;
  if ( xglExecuteIsEnabled("glTexParameteriv") )
    glTexParameteriv ( target, pname, params ) ;
}

void xglTranslated ( GLdouble x, GLdouble y, GLdouble z )
{
  if ( xglTraceIsEnabled("glTranslated") )
    fprintf ( xglTraceFd, "  glTranslated ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glTranslated") )
    glTranslated ( x, y, z ) ;
}

void xglTranslatef ( GLfloat x, GLfloat y, GLfloat z )
{
  if ( xglTraceIsEnabled("glTranslatef") )
    fprintf ( xglTraceFd, "  glTranslatef ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glTranslatef") )
    glTranslatef ( x, y, z ) ;
}

void xglVertex2d ( GLdouble x, GLdouble y )
{
  if ( xglTraceIsEnabled("glVertex2d") )
    fprintf ( xglTraceFd, "  glVertex2d ( (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glVertex2d") )
    glVertex2d ( x, y ) ;
}

void xglVertex2dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glVertex2dv") )
    fprintf ( xglTraceFd, "  glVertex2dv ( xglBuild2dv((GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glVertex2dv") )
    glVertex2dv ( v ) ;
}

void xglVertex2f ( GLfloat x, GLfloat y )
{
  if ( xglTraceIsEnabled("glVertex2f") )
    fprintf ( xglTraceFd, "  glVertex2f ( (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glVertex2f") )
    glVertex2f ( x, y ) ;
}

void xglVertex2fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glVertex2fv") )
    fprintf ( xglTraceFd, "  glVertex2fv ( xglBuild2fv((GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glVertex2fv") )
    glVertex2fv ( v ) ;
}

void xglVertex2i ( GLint x, GLint y )
{
  if ( xglTraceIsEnabled("glVertex2i") )
    fprintf ( xglTraceFd, "  glVertex2i ( (GLint)%d, (GLint)%d ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glVertex2i") )
    glVertex2i ( x, y ) ;
}

void xglVertex2iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glVertex2iv") )
    fprintf ( xglTraceFd, "  glVertex2iv ( xglBuild2iv((GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glVertex2iv") )
    glVertex2iv ( v ) ;
}

void xglVertex2s ( GLshort x, GLshort y )
{
  if ( xglTraceIsEnabled("glVertex2s") )
    fprintf ( xglTraceFd, "  glVertex2s ( (GLshort)%d, (GLshort)%d ) ;\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glVertex2s") )
    glVertex2s ( x, y ) ;
}

void xglVertex2sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glVertex2sv") )
    fprintf ( xglTraceFd, "  glVertex2sv ( xglBuild2sv((GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1] ) ;
  if ( xglExecuteIsEnabled("glVertex2sv") )
    glVertex2sv ( v ) ;
}

void xglVertex3d ( GLdouble x, GLdouble y, GLdouble z )
{
  if ( xglTraceIsEnabled("glVertex3d") )
    fprintf ( xglTraceFd, "  glVertex3d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glVertex3d") )
    glVertex3d ( x, y, z ) ;
}

void xglVertex3dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glVertex3dv") )
    fprintf ( xglTraceFd, "  glVertex3dv ( xglBuild3dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glVertex3dv") )
    glVertex3dv ( v ) ;
}

void xglVertex3f ( GLfloat x, GLfloat y, GLfloat z )
{
  if ( xglTraceIsEnabled("glVertex3f") )
    fprintf ( xglTraceFd, "  glVertex3f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glVertex3f") )
    glVertex3f ( x, y, z ) ;
}

void xglVertex3fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glVertex3fv") )
    fprintf ( xglTraceFd, "  glVertex3fv ( xglBuild3fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glVertex3fv") )
    glVertex3fv ( v ) ;
}

void xglVertex3i ( GLint x, GLint y, GLint z )
{
  if ( xglTraceIsEnabled("glVertex3i") )
    fprintf ( xglTraceFd, "  glVertex3i ( (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glVertex3i") )
    glVertex3i ( x, y, z ) ;
}

void xglVertex3iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glVertex3iv") )
    fprintf ( xglTraceFd, "  glVertex3iv ( xglBuild3iv((GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glVertex3iv") )
    glVertex3iv ( v ) ;
}

void xglVertex3s ( GLshort x, GLshort y, GLshort z )
{
  if ( xglTraceIsEnabled("glVertex3s") )
    fprintf ( xglTraceFd, "  glVertex3s ( (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , x, y, z ) ;
  if ( xglExecuteIsEnabled("glVertex3s") )
    glVertex3s ( x, y, z ) ;
}

void xglVertex3sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glVertex3sv") )
    fprintf ( xglTraceFd, "  glVertex3sv ( xglBuild3sv((GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2] ) ;
  if ( xglExecuteIsEnabled("glVertex3sv") )
    glVertex3sv ( v ) ;
}

void xglVertex4d ( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
  if ( xglTraceIsEnabled("glVertex4d") )
    fprintf ( xglTraceFd, "  glVertex4d ( (GLdouble)%f, (GLdouble)%f, (GLdouble)%f, (GLdouble)%f ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glVertex4d") )
    glVertex4d ( x, y, z, w ) ;
}

void xglVertex4dv ( GLdouble* v )
{
  if ( xglTraceIsEnabled("glVertex4dv") )
    fprintf ( xglTraceFd, "  glVertex4dv ( xglBuild4dv((GLdouble)%f,(GLdouble)%f,(GLdouble)%f,(GLdouble)%f) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glVertex4dv") )
    glVertex4dv ( v ) ;
}

void xglVertex4f ( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
  if ( xglTraceIsEnabled("glVertex4f") )
    fprintf ( xglTraceFd, "  glVertex4f ( (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff, (GLfloat)%ff ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glVertex4f") )
    glVertex4f ( x, y, z, w ) ;
}

void xglVertex4fv ( GLfloat* v )
{
  if ( xglTraceIsEnabled("glVertex4fv") )
    fprintf ( xglTraceFd, "  glVertex4fv ( xglBuild4fv((GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff,(GLfloat)%ff) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glVertex4fv") )
    glVertex4fv ( v ) ;
}

void xglVertex4i ( GLint x, GLint y, GLint z, GLint w )
{
  if ( xglTraceIsEnabled("glVertex4i") )
    fprintf ( xglTraceFd, "  glVertex4i ( (GLint)%d, (GLint)%d, (GLint)%d, (GLint)%d ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glVertex4i") )
    glVertex4i ( x, y, z, w ) ;
}

void xglVertex4iv ( GLint* v )
{
  if ( xglTraceIsEnabled("glVertex4iv") )
    fprintf ( xglTraceFd, "  glVertex4iv ( xglBuild4iv((GLint)%d,(GLint)%d,(GLint)%d,(GLint)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glVertex4iv") )
    glVertex4iv ( v ) ;
}

void xglVertex4s ( GLshort x, GLshort y, GLshort z, GLshort w )
{
  if ( xglTraceIsEnabled("glVertex4s") )
    fprintf ( xglTraceFd, "  glVertex4s ( (GLshort)%d, (GLshort)%d, (GLshort)%d, (GLshort)%d ) ;\n" , x, y, z, w ) ;
  if ( xglExecuteIsEnabled("glVertex4s") )
    glVertex4s ( x, y, z, w ) ;
}

void xglVertex4sv ( GLshort* v )
{
  if ( xglTraceIsEnabled("glVertex4sv") )
    fprintf ( xglTraceFd, "  glVertex4sv ( xglBuild4sv((GLshort)%d,(GLshort)%d,(GLshort)%d,(GLshort)%d) ) ;\n" , v[0], v[1], v[2], v[3] ) ;
  if ( xglExecuteIsEnabled("glVertex4sv") )
    glVertex4sv ( v ) ;
}

void xglVertexPointerEXT ( GLint size, GLenum type, GLsizei stride, GLsizei count, void* ptr )
{
  if ( xglTraceIsEnabled("glVertexPointerEXT") )
    fprintf ( xglTraceFd, "  glVertexPointerEXT ( (GLint)%d, (GLenum)%s, (GLsizei)%d, (GLsizei)%d, (void *)0x%08x ) ;\n" , size, xglExpandGLenum ( (GLenum) type ), stride, count, ptr ) ;
#ifdef GL_VERSION_1_1
    glVertexPointer ( size, type, stride, ptr ) ;
#else
#ifdef GL_EXT_vertex_array
  if ( xglExecuteIsEnabled("glVertexPointerEXT") )
    glVertexPointerEXT ( size, type, stride, count, ptr ) ;
#else
  fprintf ( xglTraceFd, "  glVertexPointerEXT isn't supported on this OpenGL!\n" ) ;
#endif
#endif
}

void xglViewport ( GLint x, GLint y, GLsizei width, GLsizei height )
{
  if ( xglTraceIsEnabled("glViewport") )
    fprintf ( xglTraceFd, "  glViewport ( (GLint)%d, (GLint)%d, (GLsizei)%d, (GLsizei)%d ) ;\n" , x, y, width, height ) ;
  if ( xglExecuteIsEnabled("glViewport") )
    glViewport ( x, y, width, height ) ;
}

void xglutAddMenuEntry ( char* label, int value )
{
  if ( xglTraceIsEnabled("glutAddMenuEntry") )
    fprintf ( xglTraceFd, "  /* glutAddMenuEntry ( \"%s\", %d ) ; */\n" , label, value ) ;
  if ( xglExecuteIsEnabled("glutAddMenuEntry") )
    glutAddMenuEntry ( label, value ) ;
}

void xglutAttachMenu ( int button )
{
  if ( xglTraceIsEnabled("glutAttachMenu") )
    fprintf ( xglTraceFd, "  /* glutAttachMenu ( %d ) ; */\n" , button ) ;
  if ( xglExecuteIsEnabled("glutAttachMenu") )
    glutAttachMenu ( button ) ;
}

int xglutCreateMenu ( void (*func)(int) )
{
  if ( xglTraceIsEnabled("glutCreateMenu") )
    fprintf ( xglTraceFd, "  /* glutCreateMenu ( 0x%08x ) ; */\n" , func ) ;

  return glutCreateMenu ( func ) ;
}

int xglutCreateWindow ( char* title )
{
  if ( xglTraceIsEnabled("glutCreateWindow") )
    fprintf ( xglTraceFd, "  /* glutCreateWindow ( \"%s\" ) ; */\n" , title ) ;

  return glutCreateWindow ( title ) ;
}

void xglutDisplayFunc ( void (*func)(void) )
{
  if ( xglTraceIsEnabled("glutDisplayFunc") )
    fprintf ( xglTraceFd, "  /* glutDisplayFunc ( 0x%08x ) ; */\n" , func ) ;
  if ( xglExecuteIsEnabled("glutDisplayFunc") )
    glutDisplayFunc ( func ) ;
}

void xglutIdleFunc ( void (*func)(void) )
{
  if ( xglTraceIsEnabled("glutIdleFunc") )
    fprintf ( xglTraceFd, "  /* glutIdleFunc ( 0x%08x ) ; */\n" , func ) ;
  if ( xglExecuteIsEnabled("glutIdleFunc") )
    glutIdleFunc ( func ) ;
}

void xglutInit ( int* argcp, char** argv )
{
  if(!xglTraceFd ) {     // Not defined by any other means, must be here
    xglTraceFd = stdout; // avoid a crash from a NULL ptr.
    }
  if ( xglTraceIsEnabled("glutInit") )
    fprintf ( xglTraceFd,
              "  /* glutInit ( (int *)0x%08x, (char **)0x%08x ) ; */\n" ,
              argcp, argv ) ;
  if ( xglExecuteIsEnabled("glutInit") )
    glutInit ( argcp, argv ) ;
}

void xglutInitDisplayMode ( unsigned int mode )
{
  if ( xglTraceIsEnabled("glutInitDisplayMode") )
    fprintf ( xglTraceFd, "  /* glutInitDisplayMode ( %u ) ; */\n" , mode ) ;
  if ( xglExecuteIsEnabled("glutInitDisplayMode") )
    glutInitDisplayMode ( mode ) ;
}

void xglutInitWindowPosition ( int x, int y )
{
  if ( xglTraceIsEnabled("glutInitWindowPosition") )
    fprintf ( xglTraceFd, "  /* glutInitWindowPosition ( %d, %d ) ; */\n" , x, y ) ;
  if ( xglExecuteIsEnabled("glutInitWindowPosition") )
    glutInitWindowPosition ( x, y ) ;
}

void xglutInitWindowSize ( int width, int height )
{
  if ( xglTraceIsEnabled("glutInitWindowSize") )
    fprintf ( xglTraceFd, "  /* glutInitWindowSize ( %d, %d ) ; */\n" , width, height ) ;
  if ( xglExecuteIsEnabled("glutInitWindowSize") )
    glutInitWindowSize ( width, height ) ;
}

void xglutKeyboardFunc ( void (*func)(unsigned char key, int x, int y) )
{
  if ( xglTraceIsEnabled("glutKeyboardFunc") )
    fprintf ( xglTraceFd, "  /* glutKeyboardFunc ( 0x%08x ) ; */\n" , func ) ;
  if ( xglExecuteIsEnabled("glutKeyboardFunc") )
    glutKeyboardFunc ( func ) ;
}

void xglutMainLoopUpdate (  )
{
  if ( xglTraceIsEnabled("glutMainLoopUpdate") )
    fprintf ( xglTraceFd, "  /* glutMainLoopUpdate (  ) ; */\n"  ) ;
  if ( xglExecuteIsEnabled("glutMainLoopUpdate") )
    /* glutMainLoopUpdate (  ) ; */
    printf("Steves glutMainLoopUpdate() hack not executed!!!!\n");
}

void xglutPostRedisplay (  )
{
  if ( xglTraceIsEnabled("glutPostRedisplay") )
    fprintf ( xglTraceFd, "  /* glutPostRedisplay (  ) ; */\n"  ) ;
  if ( xglExecuteIsEnabled("glutPostRedisplay") )
    glutPostRedisplay (  ) ;
}

void xglutPreMainLoop (  )
{
  if ( xglTraceIsEnabled("glutPreMainLoop") )
    fprintf ( xglTraceFd, "  /* glutPreMainLoop (  ) ; */\n"  ) ;
  if ( xglExecuteIsEnabled("glutPreMainLoop") )
    /* glutPreMainLoop (  ) ; */
    printf("Steves glutPreLoopUpdate() hack not executed!!!!\n");

}

void xglutReshapeFunc ( void (*func)(int width, int height) )
{
  if ( xglTraceIsEnabled("glutReshapeFunc") )
    fprintf ( xglTraceFd, "  /* glutReshapeFunc ( 0x%08x ) ; */\n" , func ) ;
  if ( xglExecuteIsEnabled("glutReshapeFunc") )
    glutReshapeFunc ( func ) ;
}

void xglutSwapBuffers ()
{
  if ( xglTraceIsEnabled("glutSwapBuffers") )
    fprintf ( xglTraceFd, "  /* glutSwapBuffers (  ) ; */\n"  ) ;
  if ( xglExecuteIsEnabled("glutSwapBuffers") )
    glutSwapBuffers () ;
}

GLboolean xglAreTexturesResidentEXT ( GLsizei n, GLuint* textures, GLboolean* residences )
{
  if ( xglTraceIsEnabled("glAreTexturesResidentEXT") )
    fprintf ( xglTraceFd, "  /* glAreTexturesResidentEXT ( (GLsizei)%d, (GLuint *)0x%08x, (GLboolean *)0x%08x ) ; */\n" , n, textures, residences ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glAreTexturesResidentEXT") )
    return glAreTexturesResidentEXT ( n, textures, residences ) ;
#else
  fprintf ( xglTraceFd, "  glAreTexturesResidentEXT isn't supported on this OpenGL!\n" ) ;
#endif

  return TRUE ;
}

GLboolean xglIsTextureEXT ( GLuint texture )
{
  if ( xglTraceIsEnabled("glIsTextureEXT") )
    fprintf ( xglTraceFd, "  /* glIsTextureEXT ( (GLuint)%u ) ; */\n" , texture ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glIsTextureEXT") )
    return glIsTextureEXT ( texture ) ;
#else
  fprintf ( xglTraceFd, "  glIsTextureEXT isn't supported on this OpenGL!\n" ) ;
#endif

  return TRUE ;
}

void xglBindTextureEXT ( GLenum target, GLuint texture )
{
  if ( xglTraceIsEnabled("glBindTextureEXT") )
    fprintf ( xglTraceFd, "  glBindTextureEXT ( (GLenum)%s, (GLuint)%u ) ;\n" , xglExpandGLenum ( (GLenum) target ), texture ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glBindTextureEXT") )
    glBindTextureEXT ( target, texture ) ;
#else
  fprintf ( xglTraceFd, "  glBindTextureEXT isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglDeleteTexturesEXT ( GLsizei n, GLuint* textures )
{
  if ( xglTraceIsEnabled("glDeleteTexturesEXT") )
    fprintf ( xglTraceFd, "  glDeleteTexturesEXT ( (GLsizei)%d, (GLuint *)0x%08x ) ;\n" , n, textures ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glDeleteTexturesEXT") )
    glDeleteTexturesEXT ( n, textures ) ;
#else
  fprintf ( xglTraceFd, "  glDeleteTextures isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglGenTexturesEXT ( GLsizei n, GLuint* textures )
{
  if ( xglTraceIsEnabled("glGenTexturesEXT") )
    fprintf ( xglTraceFd, "  glGenTexturesEXT ( (GLsizei)%d, (GLuint *)0x%08x ) ;\n" , n, textures ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glGenTexturesEXT") )
    glGenTexturesEXT ( n, textures ) ;
#else
  fprintf ( xglTraceFd, "  glDeleteTexturesEXT isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglPrioritizeTexturesEXT ( GLsizei n, GLuint* textures, GLclampf* priorities )
{
  if ( xglTraceIsEnabled("glPrioritizeTexturesEXT") )
    fprintf ( xglTraceFd, "  glPrioritizeTexturesEXT ( (GLsizei)%d, (GLuint *)0x%08x, (GLclampf *)0x%08x ) ;\n" , n, textures, priorities ) ;

#ifdef GL_TEXTURE_2D_BINDING_EXT
  if ( xglExecuteIsEnabled("glPrioritizeTexturesEXT") )
    glPrioritizeTexturesEXT ( n, textures, priorities ) ;
#else
  fprintf ( xglTraceFd, "  glPrioritizeTexturesEXT isn't supported on this OpenGL!\n" ) ;
#endif
}


GLboolean xglAreTexturesResident ( GLsizei n, GLuint* textures, GLboolean* residences )
{
  if ( xglTraceIsEnabled("glAreTexturesResident") )
    fprintf ( xglTraceFd, "  /* glAreTexturesResident ( (GLsizei)%d, (GLuint *)0x%08x, (GLboolean *)0x%08x ) ; */\n" , n, textures, residences ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glAreTexturesResident") )
    return glAreTexturesResident ( n, textures, residences ) ;
#else
  fprintf ( xglTraceFd, "  glAreTexturesResident isn't supported on this OpenGL!\n" ) ;
#endif

  return TRUE ;
}

GLboolean xglIsTexture ( GLuint texture )
{
  if ( xglTraceIsEnabled("glIsTexture") )
    fprintf ( xglTraceFd, "  /* glIsTexture ( (GLuint)%u ) ; */\n" , texture ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glIsTexture") )
    return glIsTexture ( texture ) ;
#else
  fprintf ( xglTraceFd, "  glIsTexture isn't supported on this OpenGL!\n" ) ;
#endif

  return TRUE ;
}

void xglBindTexture ( GLenum target, GLuint texture )
{
  if ( xglTraceIsEnabled("glBindTexture") )
    fprintf ( xglTraceFd, "  glBindTexture ( (GLenum)%s, (GLuint)%u ) ;\n" , xglExpandGLenum ( (GLenum) target ), texture ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glBindTexture") )
    glBindTexture ( target, texture ) ;
#else
  fprintf ( xglTraceFd, "  glBindTexture isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglDeleteTextures ( GLsizei n, GLuint* textures )
{
  if ( xglTraceIsEnabled("glDeleteTextures") )
    fprintf ( xglTraceFd, "  glDeleteTextures ( (GLsizei)%d, (GLuint *)0x%08x ) ;\n" , n, textures ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glDeleteTextures") )
    glDeleteTextures ( n, textures ) ;
#else
  fprintf ( xglTraceFd, "  glDeleteTextures isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglGenTextures ( GLsizei n, GLuint* textures )
{
  if ( xglTraceIsEnabled("glGenTextures") )
    fprintf ( xglTraceFd, "  glGenTextures ( (GLsizei)%d, (GLuint *)0x%08x ) ;\n" , n, textures ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glGenTextures") )
    glGenTextures ( n, textures ) ;
#else
  fprintf ( xglTraceFd, "  glDeleteTextures isn't supported on this OpenGL!\n" ) ;
#endif
}

void xglPrioritizeTextures ( GLsizei n, GLuint* textures, GLclampf* priorities )
{
  if ( xglTraceIsEnabled("glPrioritizeTextures") )
    fprintf ( xglTraceFd, "  glPrioritizeTextures ( (GLsizei)%d, (GLuint *)0x%08x, (GLclampf *)0x%08x ) ;\n" , n, textures, priorities ) ;

#ifdef GL_VERSION_1_1
  if ( xglExecuteIsEnabled("glPrioritizeTextures") )
    glPrioritizeTextures ( n, textures, priorities ) ;
#else
  fprintf ( xglTraceFd, "  glPrioritizeTextures isn't supported on this OpenGL!\n" ) ;
#endif
}

#endif

