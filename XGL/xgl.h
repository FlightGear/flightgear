#ifndef _XGL_H
#define _XGL_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#include <GL/gl.h>
#include <GL/glu.h>
#include <stdio.h>

/* xgl Utilities */

extern FILE *xglTraceFd ;

int       xglTraceIsEnabled   ( char *gl_function_name ) ;
int       xglExecuteIsEnabled ( char *gl_function_name ) ;
char     *xglExpandGLenum     ( GLenum x ) ;

GLdouble *xglBuild1dv  ( GLdouble v ) ;
GLfloat  *xglBuild1fv  ( GLfloat  v ) ;
GLbyte   *xglBuild1bv  ( GLbyte   v ) ;
GLint    *xglBuild1iv  ( GLint    v ) ;
GLshort  *xglBuild1sv  ( GLshort  v ) ;
GLubyte  *xglBuild1ubv ( GLubyte  v ) ;
GLuint   *xglBuild1uiv ( GLuint   v ) ;
GLushort *xglBuild1usv ( GLushort v ) ;

GLdouble *xglBuild2dv  ( GLdouble v0, GLdouble v1 ) ;
GLfloat  *xglBuild2fv  ( GLfloat  v0, GLfloat  v1 ) ;
GLbyte   *xglBuild2bv  ( GLbyte   v0, GLbyte   v1 ) ;
GLint    *xglBuild2iv  ( GLint    v0, GLint    v1 ) ;
GLshort  *xglBuild2sv  ( GLshort  v0, GLshort  v1 ) ;
GLubyte  *xglBuild2ubv ( GLubyte  v0, GLubyte  v1 ) ;
GLuint   *xglBuild2uiv ( GLuint   v0, GLuint   v1 ) ;
GLushort *xglBuild2usv ( GLushort v0, GLushort v1 ) ;

GLdouble *xglBuild3dv  ( GLdouble v0, GLdouble v1, GLdouble v2 ) ;
GLfloat  *xglBuild3fv  ( GLfloat  v0, GLfloat  v1, GLfloat  v2 ) ;
GLbyte   *xglBuild3bv  ( GLbyte   v0, GLbyte   v1, GLbyte   v2 ) ;
GLint    *xglBuild3iv  ( GLint    v0, GLint    v1, GLint    v2 ) ;
GLshort  *xglBuild3sv  ( GLshort  v0, GLshort  v1, GLshort  v2 ) ;
GLubyte  *xglBuild3ubv ( GLubyte  v0, GLubyte  v1, GLubyte  v2 ) ;
GLuint   *xglBuild3uiv ( GLuint   v0, GLuint   v1, GLuint   v2 ) ;
GLushort *xglBuild3usv ( GLushort v0, GLushort v1, GLushort v2 ) ;

GLdouble *xglBuild4dv  ( GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 ) ;
GLfloat  *xglBuild4fv  ( GLfloat  v0, GLfloat  v1, GLfloat  v2, GLfloat  v3 ) ;
GLbyte   *xglBuild4bv  ( GLbyte   v0, GLbyte   v1, GLbyte   v2, GLbyte   v3 ) ;
GLint    *xglBuild4iv  ( GLint    v0, GLint    v1, GLint    v2, GLint    v3 ) ;
GLshort  *xglBuild4sv  ( GLshort  v0, GLshort  v1, GLshort  v2, GLshort  v3 ) ;
GLubyte  *xglBuild4ubv ( GLubyte  v0, GLubyte  v1, GLubyte  v2, GLubyte  v3 ) ;
GLuint   *xglBuild4uiv ( GLuint   v0, GLuint   v1, GLuint   v2, GLuint   v3 ) ;
GLushort *xglBuild4usv ( GLushort v0, GLushort v1, GLushort v2, GLushort v3 ) ;

GLfloat  *xglBuildMatrixf ( GLfloat m0 , GLfloat m1 , GLfloat m2 , GLfloat m3 ,
                            GLfloat m4 , GLfloat m5 , GLfloat m6 , GLfloat m7 ,
                            GLfloat m8 , GLfloat m9 , GLfloat m10, GLfloat m11,
                            GLfloat m12, GLfloat m13, GLfloat m14, GLfloat m15 ) ;

GLdouble *xglBuildMatrixd ( GLdouble m0 , GLdouble m1 , GLdouble m2 , GLdouble m3 ,
                            GLdouble m4 , GLdouble m5 , GLdouble m6 , GLdouble m7 ,
                            GLdouble m8 , GLdouble m9 , GLdouble m10, GLdouble m11,
                            GLdouble m12, GLdouble m13, GLdouble m14, GLdouble m15 ) ;

/*
  Conditionally compile all 'xgl' calls into standard 'gl' calls...

  OR

  Declare all possible 'xgl' calls as 'extern'.
*/

#ifndef XGL_TRACE

#define xglAccum		glAccum	
#define xglAlphaFunc		glAlphaFunc	
#ifdef GL_EXT_vertex_array
#define xglArrayElementEXT	glArrayElementEXT	
#endif
#define xglBegin		glBegin	
#define xglBitmap		glBitmap	
#ifdef GL_EXT_blend_color
#define xglBlendColorEXT	glBlendColorEXT	
#endif
#ifdef GL_EXT_blend_minmax
#define xglBlendEquationEXT	glBlendEquationEXT	
#endif
#define xglBlendFunc		glBlendFunc	
#define xglCallList		glCallList	
#define xglCallLists		glCallLists	
#define xglClear		glClear	
#define xglClearAccum		glClearAccum	
#define xglClearColor		glClearColor	
#define xglClearDepth		glClearDepth	
#define xglClearIndex		glClearIndex	
#define xglClearStencil		glClearStencil	
#define xglClipPlane		glClipPlane	
#define xglColor3b		glColor3b	
#define xglColor3bv		glColor3bv	
#define xglColor3d		glColor3d	
#define xglColor3dv		glColor3dv	
#define xglColor3f		glColor3f	
#define xglColor3fv		glColor3fv	
#define xglColor3i		glColor3i	
#define xglColor3iv		glColor3iv	
#define xglColor3s		glColor3s	
#define xglColor3sv		glColor3sv	
#define xglColor3ub		glColor3ub	
#define xglColor3ubv		glColor3ubv	
#define xglColor3ui		glColor3ui	
#define xglColor3uiv		glColor3uiv	
#define xglColor3us		glColor3us	
#define xglColor3usv		glColor3usv	
#define xglColor4b		glColor4b	
#define xglColor4bv		glColor4bv	
#define xglColor4d		glColor4d	
#define xglColor4dv		glColor4dv	
#define xglColor4f		glColor4f	
#define xglColor4fv		glColor4fv	
#define xglColor4i		glColor4i	
#define xglColor4iv		glColor4iv	
#define xglColor4s		glColor4s	
#define xglColor4sv		glColor4sv	
#define xglColor4ub		glColor4ub	
#define xglColor4ubv		glColor4ubv	
#define xglColor4ui		glColor4ui	
#define xglColor4uiv		glColor4uiv	
#define xglColor4us		glColor4us	
#define xglColor4usv		glColor4usv	
#define xglColorMask		glColorMask	
#define xglColorMaterial	glColorMaterial	
#ifdef GL_EXT_vertex_array
#define xglColorPointerEXT	glColorPointerEXT	
#endif
#define xglCopyPixels		glCopyPixels	
#define xglCullFace		glCullFace	
#define xglDeleteLists		glDeleteLists	
#define xglDepthFunc		glDepthFunc	
#define xglDepthMask		glDepthMask	
#define xglDepthRange		glDepthRange	
#define xglDisable		glDisable	
#ifdef GL_EXT_vertex_array
#define xglDrawArraysEXT	glDrawArraysEXT	
#endif
#define xglDrawBuffer		glDrawBuffer	
#define xglDrawPixels		glDrawPixels	
#define xglEdgeFlag		glEdgeFlag	
#ifdef GL_EXT_vertex_array
#define xglEdgeFlagPointerEXT	glEdgeFlagPointerEXT	
#endif
#define xglEdgeFlagv		glEdgeFlagv	
#define xglEnable		glEnable	
#define xglEnd			glEnd	
#define xglEndList		glEndList	
#define xglEvalCoord1d		glEvalCoord1d	
#define xglEvalCoord1dv		glEvalCoord1dv	
#define xglEvalCoord1f		glEvalCoord1f	
#define xglEvalCoord1fv		glEvalCoord1fv	
#define xglEvalCoord2d		glEvalCoord2d	
#define xglEvalCoord2dv		glEvalCoord2dv	
#define xglEvalCoord2f		glEvalCoord2f	
#define xglEvalCoord2fv		glEvalCoord2fv	
#define xglEvalMesh1		glEvalMesh1	
#define xglEvalMesh2		glEvalMesh2	
#define xglEvalPoint1		glEvalPoint1	
#define xglEvalPoint2		glEvalPoint2	
#define xglFeedbackBuffer	glFeedbackBuffer	
#define xglFinish		glFinish	
#define xglFlush		glFlush	
#define xglFogf			glFogf	
#define xglFogfv		glFogfv	
#define xglFogi			glFogi	
#define xglFogiv		glFogiv	
#define xglFrontFace		glFrontFace	
#define xglFrustum		glFrustum	
#define xglGenLists		glGenLists	
#define xglGetBooleanv		glGetBooleanv	
#define xglGetClipPlane		glGetClipPlane	
#define xglGetDoublev		glGetDoublev	
#define xglGetError		glGetError	
#define xglGetFloatv		glGetFloatv	
#define xglGetIntegerv		glGetIntegerv	
#define xglGetLightfv		glGetLightfv	
#define xglGetLightiv		glGetLightiv	
#define xglGetMapdv		glGetMapdv	
#define xglGetMapfv		glGetMapfv	
#define xglGetMapiv		glGetMapiv	
#define xglGetMaterialfv	glGetMaterialfv	
#define xglGetMaterialiv	glGetMaterialiv	
#define xglGetPixelMapfv	glGetPixelMapfv	
#define xglGetPixelMapuiv	glGetPixelMapuiv	
#define xglGetPixelMapusv	glGetPixelMapusv	
#ifdef GL_EXT_vertex_array
#define xglGetPointervEXT	glGetPointervEXT	
#endif
#define xglGetPolygonStipple	glGetPolygonStipple	
#define xglGetString		glGetString	
#define xglGetTexEnvfv		glGetTexEnvfv	
#define xglGetTexEnviv		glGetTexEnviv	
#define xglGetTexGendv		glGetTexGendv	
#define xglGetTexGenfv		glGetTexGenfv	
#define xglGetTexGeniv		glGetTexGeniv	
#define xglGetTexImage		glGetTexImage	
#define xglGetTexLevelParameterfv glGetTexLevelParameterfv	
#define xglGetTexLevelParameteriv glGetTexLevelParameteriv	
#define xglGetTexParameterfv	glGetTexParameterfv	
#define xglGetTexParameteriv	glGetTexParameteriv	
#define xglHint			glHint	
#define xglIndexMask		glIndexMask	
#ifdef GL_EXT_vertex_array
#define xglIndexPointerEXT	glIndexPointerEXT	
#endif
#define xglIndexd		glIndexd	
#define xglIndexdv		glIndexdv	
#define xglIndexf		glIndexf	
#define xglIndexfv		glIndexfv	
#define xglIndexi		glIndexi	
#define xglIndexiv		glIndexiv	
#define xglIndexs		glIndexs	
#define xglIndexsv		glIndexsv	
#define xglInitNames		glInitNames	
#define xglIsEnabled		glIsEnabled	
#define xglIsList		glIsList	
#define xglLightModelf		glLightModelf	
#define xglLightModelfv		glLightModelfv	
#define xglLightModeli		glLightModeli	
#define xglLightModeliv		glLightModeliv	
#define xglLightf		glLightf	
#define xglLightfv		glLightfv	
#define xglLighti		glLighti	
#define xglLightiv		glLightiv	
#define xglLineStipple		glLineStipple	
#define xglLineWidth		glLineWidth	
#define xglListBase		glListBase	
#define xglLoadIdentity		glLoadIdentity	
#define xglLoadMatrixd		glLoadMatrixd	
#define xglLoadMatrixf		glLoadMatrixf	
#define xglLoadName		glLoadName	
#define xglLogicOp		glLogicOp	
#define xglMap1d		glMap1d	
#define xglMap1f		glMap1f	
#define xglMap2d		glMap2d	
#define xglMap2f		glMap2f	
#define xglMapGrid1d		glMapGrid1d	
#define xglMapGrid1f		glMapGrid1f	
#define xglMapGrid2d		glMapGrid2d	
#define xglMapGrid2f		glMapGrid2f	
#define xglMaterialf		glMaterialf	
#define xglMaterialfv		glMaterialfv	
#define xglMateriali		glMateriali	
#define xglMaterialiv		glMaterialiv	
#define xglMatrixMode		glMatrixMode	
#define xglMultMatrixd		glMultMatrixd	
#define xglMultMatrixf		glMultMatrixf	
#define xglNewList		glNewList	
#define xglNormal3b		glNormal3b	
#define xglNormal3bv		glNormal3bv	
#define xglNormal3d		glNormal3d	
#define xglNormal3dv		glNormal3dv	
#define xglNormal3f		glNormal3f	
#ifdef DEBUGGING_NORMALS
#define xglNormal3fv(f)		{\
float ff = (f)[0]*(f)[0]+(f)[1]*(f)[1]+(f)[2]*(f)[2];\
if ( ff < 0.9 || ff > 1.1 )\
{\
fprintf(stderr,"glNormal3fv Overflow: %f, %f, %f -> %f [%s,%s,%s]\n",\
(f)[0],(f)[1],(f)[2],ff,str1,str2,str3);\
normal_bombed = 1 ;\
}\
glNormal3fv(f);\
}
#else
#define xglNormal3fv   		glNormal3fv
#endif
#define xglNormal3i		glNormal3i	
#define xglNormal3iv		glNormal3iv	
#define xglNormal3s		glNormal3s	
#define xglNormal3sv		glNormal3sv	
#ifdef GL_EXT_vertex_array
#define xglNormalPointerEXT	glNormalPointerEXT	
#endif
#define xglOrtho		glOrtho	
#define xglPassThrough		glPassThrough	
#define xglPixelMapfv		glPixelMapfv	
#define xglPixelMapuiv		glPixelMapuiv	
#define xglPixelMapusv		glPixelMapusv	
#define xglPixelStoref		glPixelStoref	
#define xglPixelStorei		glPixelStorei	
#define xglPixelTransferf	glPixelTransferf	
#define xglPixelTransferi	glPixelTransferi	
#define xglPixelZoom		glPixelZoom	
#define xglPointSize		glPointSize	
#define xglPolygonMode		glPolygonMode	
#ifdef GL_EXT_polygon_offset
#define xglPolygonOffsetEXT	glPolygonOffsetEXT	
#endif
#define xglPolygonOffset	glPolygonOffset	
#define xglPolygonStipple	glPolygonStipple	
#define xglPopAttrib		glPopAttrib	
#define xglPopMatrix		glPopMatrix	
#define xglPopName		glPopName	
#define xglPushAttrib		glPushAttrib	
#define xglPushMatrix		glPushMatrix	
#define xglPushName		glPushName	
#define xglRasterPos2d		glRasterPos2d	
#define xglRasterPos2dv		glRasterPos2dv	
#define xglRasterPos2f		glRasterPos2f	
#define xglRasterPos2fv		glRasterPos2fv	
#define xglRasterPos2i		glRasterPos2i	
#define xglRasterPos2iv		glRasterPos2iv	
#define xglRasterPos2s		glRasterPos2s	
#define xglRasterPos2sv		glRasterPos2sv	
#define xglRasterPos3d		glRasterPos3d	
#define xglRasterPos3dv		glRasterPos3dv	
#define xglRasterPos3f		glRasterPos3f	
#define xglRasterPos3fv		glRasterPos3fv	
#define xglRasterPos3i		glRasterPos3i	
#define xglRasterPos3iv		glRasterPos3iv	
#define xglRasterPos3s		glRasterPos3s	
#define xglRasterPos3sv		glRasterPos3sv	
#define xglRasterPos4d		glRasterPos4d	
#define xglRasterPos4dv		glRasterPos4dv	
#define xglRasterPos4f		glRasterPos4f	
#define xglRasterPos4fv		glRasterPos4fv	
#define xglRasterPos4i		glRasterPos4i	
#define xglRasterPos4iv		glRasterPos4iv	
#define xglRasterPos4s		glRasterPos4s	
#define xglRasterPos4sv		glRasterPos4sv	
#define xglReadBuffer		glReadBuffer	
#define xglReadPixels		glReadPixels	
#define xglRectd		glRectd	
#define xglRectdv		glRectdv	
#define xglRectf		glRectf	
#define xglRectfv		glRectfv	
#define xglRecti		glRecti	
#define xglRectiv		glRectiv	
#define xglRects		glRects	
#define xglRectsv		glRectsv	
#define xglRenderMode		glRenderMode	
#define xglRotated		glRotated	
#define xglRotatef		glRotatef	
#define xglScaled		glScaled	
#define xglScalef		glScalef	
#define xglScissor		glScissor	
#define xglSelectBuffer		glSelectBuffer	
#define xglShadeModel		glShadeModel	
#define xglStencilFunc		glStencilFunc	
#define xglStencilMask		glStencilMask	
#define xglStencilOp		glStencilOp	
#define xglTexCoord1d		glTexCoord1d	
#define xglTexCoord1dv		glTexCoord1dv	
#define xglTexCoord1f		glTexCoord1f	
#define xglTexCoord1fv		glTexCoord1fv	
#define xglTexCoord1i		glTexCoord1i	
#define xglTexCoord1iv		glTexCoord1iv	
#define xglTexCoord1s		glTexCoord1s	
#define xglTexCoord1sv		glTexCoord1sv	
#define xglTexCoord2d		glTexCoord2d	
#define xglTexCoord2dv		glTexCoord2dv	
#define xglTexCoord2f		glTexCoord2f	
#define xglTexCoord2fv		glTexCoord2fv	
#define xglTexCoord2i		glTexCoord2i	
#define xglTexCoord2iv		glTexCoord2iv	
#define xglTexCoord2s		glTexCoord2s	
#define xglTexCoord2sv		glTexCoord2sv	
#define xglTexCoord3d		glTexCoord3d	
#define xglTexCoord3dv		glTexCoord3dv	
#define xglTexCoord3f		glTexCoord3f	
#define xglTexCoord3fv		glTexCoord3fv	
#define xglTexCoord3i		glTexCoord3i	
#define xglTexCoord3iv		glTexCoord3iv	
#define xglTexCoord3s		glTexCoord3s	
#define xglTexCoord3sv		glTexCoord3sv	
#define xglTexCoord4d		glTexCoord4d	
#define xglTexCoord4dv		glTexCoord4dv	
#define xglTexCoord4f		glTexCoord4f	
#define xglTexCoord4fv		glTexCoord4fv	
#define xglTexCoord4i		glTexCoord4i	
#define xglTexCoord4iv		glTexCoord4iv	
#define xglTexCoord4s		glTexCoord4s	
#define xglTexCoord4sv		glTexCoord4sv	
#ifdef GL_EXT_vertex_array
#define xglTexCoordPointerEXT	glTexCoordPointerEXT	
#endif
#define xglTexEnvf		glTexEnvf	
#define xglTexEnvfv		glTexEnvfv	
#define xglTexEnvi		glTexEnvi	
#define xglTexEnviv		glTexEnviv	
#define xglTexGend		glTexGend	
#define xglTexGendv		glTexGendv	
#define xglTexGenf		glTexGenf	
#define xglTexGenfv		glTexGenfv	
#define xglTexGeni		glTexGeni	
#define xglTexGeniv		glTexGeniv	
#define xglTexImage1D		glTexImage1D	
#define xglTexImage2D		glTexImage2D	
#define xglTexParameterf	glTexParameterf	
#define xglTexParameterfv	glTexParameterfv	
#define xglTexParameteri	glTexParameteri	
#define xglTexParameteriv	glTexParameteriv	
#define xglTranslated		glTranslated	
#define xglTranslatef		glTranslatef	
#define xglVertex2d		glVertex2d	
#define xglVertex2dv		glVertex2dv	
#define xglVertex2f		glVertex2f	
#define xglVertex2fv		glVertex2fv	
#define xglVertex2i		glVertex2i	
#define xglVertex2iv		glVertex2iv	
#define xglVertex2s		glVertex2s	
#define xglVertex2sv		glVertex2sv	
#define xglVertex3d		glVertex3d	
#define xglVertex3dv		glVertex3dv	
#define xglVertex3f		glVertex3f	
#define xglVertex3fv		glVertex3fv	
#define xglVertex3i		glVertex3i	
#define xglVertex3iv		glVertex3iv	
#define xglVertex3s		glVertex3s	
#define xglVertex3sv		glVertex3sv	
#define xglVertex4d		glVertex4d	
#define xglVertex4dv		glVertex4dv	
#define xglVertex4f		glVertex4f	
#define xglVertex4fv		glVertex4fv	
#define xglVertex4i		glVertex4i	
#define xglVertex4iv		glVertex4iv	
#define xglVertex4s		glVertex4s	
#define xglVertex4sv		glVertex4sv	
#ifdef GL_EXT_vertex_array
#define xglVertexPointerEXT	glVertexPointerEXT	
#endif
#define xglViewport		glViewport	

#ifdef GL_VERSION_1_1
#define xglAreTexturesResident	  glAreTexturesResident
#define xglIsTexture		  glIsTexture
#define xglBindTexture		  glBindTexture
#define xglDeleteTextures	  glDeleteTextures
#define xglGenTextures		  glGenTextures
#define xglPrioritizeTextures	  glPrioritizeTextures
#endif

#ifdef GL_EXT_texture_object
#define xglAreTexturesResidentEXT glAreTexturesResidentEXT
#define xglIsTextureEXT		  glIsTextureEXT
#define xglBindTextureEXT	  glBindTextureEXT
#define xglDeleteTexturesEXT	  glDeleteTexturesEXT
#define xglGenTexturesEXT	  glGenTexturesEXT
#define xglPrioritizeTexturesEXT  glPrioritizeTexturesEXT
#endif

#define xglutAddMenuEntry       glutAddMenuEntry
#define xglutAttachMenu         glutAttachMenu
#define xglutCreateMenu         glutCreateMenu
#define xglutCreateWindow       glutCreateWindow
#define xglutDisplayFunc        glutDisplayFunc
#define xglutIdleFunc           glutIdleFunc
#define xglutInit               glutInit
#define xglutInitDisplayMode    glutInitDisplayMode
#define xglutInitWindowPosition glutInitWindowPosition
#define xglutInitWindowSize     glutInitWindowSize
#define xglutKeyboardFunc       glutKeyboardFunc
#define xglutMainLoopUpdate     glutMainLoopUpdate
#define xglutPostRedisplay      glutPostRedisplay
#define xglutPreMainLoop        glutPreMainLoop
#define xglutReshapeFunc        glutReshapeFunc
#define xglutSwapBuffers        glutSwapBuffers

#else

#ifdef __cplusplus
extern "C" {
#endif

GLboolean xglIsEnabled  ( GLenum cap ) ; 
GLboolean xglIsList     ( GLuint list ) ; 
GLenum    xglGetError   () ; 
GLint     xglRenderMode ( GLenum mode ) ; 
GLuint    xglGenLists   ( GLsizei range ) ; 
const GLubyte  *xglGetString  ( GLenum name ) ; 

void xglAccum		( GLenum op, GLfloat value ) ;
void xglAlphaFunc	( GLenum func, GLclampf ref ) ;
void xglArrayElementEXT	( GLint i ) ;
void xglBegin		( GLenum mode ) ;
void xglBitmap		( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, GLubyte *bitmap ) ;
void xglBlendColorEXT	( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) ;
void xglBlendEquationEXT( GLenum mode ) ;
void xglBlendFunc	( GLenum sfactor, GLenum dfactor ) ;
void xglCallList	( GLuint list ) ;
void xglCallLists	( GLsizei n, GLenum type, GLvoid *lists ) ;
void xglClear		( GLbitfield mask ) ;
void xglClearAccum	( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) ;
void xglClearColor	( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) ;
void xglClearDepth	( GLclampd depth ) ;
void xglClearIndex	( GLfloat c ) ;
void xglClearStencil	( GLint s ) ;
void xglClipPlane	( GLenum plane, GLdouble *equation ) ;
void xglColor3b		( GLbyte red, GLbyte green, GLbyte blue ) ;
void xglColor3bv	( GLbyte *v ) ;
void xglColor3d		( GLdouble red, GLdouble green, GLdouble blue ) ;
void xglColor3dv	( GLdouble *v ) ;
void xglColor3f		( GLfloat red, GLfloat green, GLfloat blue ) ;
void xglColor3fv	( GLfloat *v ) ;
void xglColor3i		( GLint red, GLint green, GLint blue ) ;
void xglColor3iv	( GLint *v ) ;
void xglColor3s		( GLshort red, GLshort green, GLshort blue ) ;
void xglColor3sv	( GLshort *v ) ;
void xglColor3ub	( GLubyte red, GLubyte green, GLubyte blue ) ;
void xglColor3ubv	( GLubyte *v ) ;
void xglColor3ui	( GLuint red, GLuint green, GLuint blue ) ;
void xglColor3uiv	( GLuint *v ) ;
void xglColor3us	( GLushort red, GLushort green, GLushort blue ) ;
void xglColor3usv	( GLushort *v ) ;
void xglColor4b		( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha ) ;
void xglColor4bv	( GLbyte *v ) ;
void xglColor4d		( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha ) ;
void xglColor4dv	( GLdouble *v ) ;
void xglColor4f		( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) ;
void xglColor4fv	( GLfloat *v ) ;
void xglColor4i		( GLint red, GLint green, GLint blue, GLint alpha ) ;
void xglColor4iv	( GLint *v ) ;
void xglColor4s		( GLshort red, GLshort green, GLshort blue, GLshort alpha ) ;
void xglColor4sv	( GLshort *v ) ;
void xglColor4ub	( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha ) ;
void xglColor4ubv	( GLubyte *v ) ;
void xglColor4ui	( GLuint red, GLuint green, GLuint blue, GLuint alpha ) ;
void xglColor4uiv	( GLuint *v ) ;
void xglColor4us	( GLushort red, GLushort green, GLushort blue, GLushort alpha ) ;
void xglColor4usv	( GLushort *v ) ;
void xglColorMask	( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) ;
void xglColorMaterial	( GLenum face, GLenum mode ) ;
void xglColorPointerEXT	( GLint size, GLenum type, GLsizei stride, GLsizei count, void *ptr ) ;
void xglCopyPixels	( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type ) ;
void xglCullFace	( GLenum mode ) ;
void xglDeleteLists	( GLuint list, GLsizei range ) ;
void xglDepthFunc	( GLenum func ) ;
void xglDepthMask	( GLboolean flag ) ;
void xglDepthRange	( GLclampd near_val, GLclampd far_val ) ;
void xglDisable		( GLenum cap ) ;
void xglDrawArraysEXT	( GLenum mode, GLint first, GLsizei count ) ;
void xglDrawBuffer	( GLenum mode ) ;
void xglDrawPixels	( GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) ;
void xglEdgeFlag	( GLboolean flag ) ;
void xglEdgeFlagPointerEXT( GLsizei stride, GLsizei count, GLboolean *ptr ) ;
void xglEdgeFlagv	( GLboolean *flag ) ;
void xglEnable		( GLenum cap ) ;
void xglEnd		() ;
void xglEndList		() ;
void xglEvalCoord1d	( GLdouble u ) ;
void xglEvalCoord1dv	( GLdouble *u ) ;
void xglEvalCoord1f	( GLfloat u ) ;
void xglEvalCoord1fv	( GLfloat *u ) ;
void xglEvalCoord2d	( GLdouble u, GLdouble v ) ;
void xglEvalCoord2dv	( GLdouble *u ) ;
void xglEvalCoord2f	( GLfloat u, GLfloat v ) ;
void xglEvalCoord2fv	( GLfloat *u ) ;
void xglEvalMesh1	( GLenum mode, GLint i1, GLint i2 ) ;
void xglEvalMesh2	( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) ;
void xglEvalPoint1	( GLint i ) ;
void xglEvalPoint2	( GLint i, GLint j ) ;
void xglFeedbackBuffer	( GLsizei size, GLenum type, GLfloat *buffer ) ;
void xglFinish		() ;
void xglFlush		() ;
void xglFogf		( GLenum pname, GLfloat param ) ;
void xglFogfv		( GLenum pname, GLfloat *params ) ;
void xglFogi		( GLenum pname, GLint param ) ;
void xglFogiv		( GLenum pname, GLint *params ) ;
void xglFrontFace	( GLenum mode ) ;
void xglFrustum		( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) ;
void xglGetBooleanv	( GLenum pname, GLboolean *params ) ;
void xglGetClipPlane	( GLenum plane, GLdouble *equation ) ;
void xglGetDoublev	( GLenum pname, GLdouble *params ) ;
void xglGetFloatv	( GLenum pname, GLfloat *params ) ;
void xglGetIntegerv	( GLenum pname, GLint *params ) ;
void xglGetLightfv	( GLenum light, GLenum pname, GLfloat *params ) ;
void xglGetLightiv	( GLenum light, GLenum pname, GLint *params ) ;
void xglGetMapdv	( GLenum target, GLenum query, GLdouble *v ) ;
void xglGetMapfv	( GLenum target, GLenum query, GLfloat *v ) ;
void xglGetMapiv	( GLenum target, GLenum query, GLint *v ) ;
void xglGetMaterialfv	( GLenum face, GLenum pname, GLfloat *params ) ;
void xglGetMaterialiv	( GLenum face, GLenum pname, GLint *params ) ;
void xglGetPixelMapfv	( GLenum map, GLfloat *values ) ;
void xglGetPixelMapuiv	( GLenum map, GLuint *values ) ;
void xglGetPixelMapusv	( GLenum map, GLushort *values ) ;
void xglGetPointervEXT	( GLenum pname, void **params ) ;
void xglGetPolygonStipple( GLubyte *mask ) ;
void xglGetTexEnvfv	( GLenum target, GLenum pname, GLfloat *params ) ;
void xglGetTexEnviv	( GLenum target, GLenum pname, GLint *params ) ;
void xglGetTexGendv	( GLenum coord, GLenum pname, GLdouble *params ) ;
void xglGetTexGenfv	( GLenum coord, GLenum pname, GLfloat *params ) ;
void xglGetTexGeniv	( GLenum coord, GLenum pname, GLint *params ) ;
void xglGetTexImage	( GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels ) ;
void xglGetTexLevelParameterfv	( GLenum target, GLint level, GLenum pname, GLfloat *params ) ;
void xglGetTexLevelParameteriv	( GLenum target, GLint level, GLenum pname, GLint *params ) ;
void xglGetTexParameterfv	( GLenum target, GLenum pname, GLfloat *params) ;
void xglGetTexParameteriv	( GLenum target, GLenum pname, GLint *params ) ;
void xglHint		( GLenum target, GLenum mode ) ;
void xglIndexMask	( GLuint mask ) ;
void xglIndexPointerEXT	( GLenum type, GLsizei stride, GLsizei count, void *ptr ) ;
void xglIndexd		( GLdouble c ) ;
void xglIndexdv		( GLdouble *c ) ;
void xglIndexf		( GLfloat c ) ;
void xglIndexfv		( GLfloat *c ) ;
void xglIndexi		( GLint c ) ;
void xglIndexiv		( GLint *c ) ;
void xglIndexs		( GLshort c ) ;
void xglIndexsv		( GLshort *c ) ;
void xglInitNames	() ;
void xglLightModelf	( GLenum pname, GLfloat param ) ;
void xglLightModelfv	( GLenum pname, GLfloat *params ) ;
void xglLightModeli	( GLenum pname, GLint param ) ;
void xglLightModeliv	( GLenum pname, GLint *params ) ;
void xglLightf		( GLenum light, GLenum pname, GLfloat param ) ;
void xglLightfv		( GLenum light, GLenum pname, GLfloat *params ) ;
void xglLighti		( GLenum light, GLenum pname, GLint param ) ;
void xglLightiv		( GLenum light, GLenum pname, GLint *params ) ;
void xglLineStipple	( GLint factor, GLushort pattern ) ;
void xglLineWidth	( GLfloat width ) ;
void xglListBase	( GLuint base ) ;
void xglLoadIdentity	() ;
void xglLoadMatrixd	( GLdouble *m ) ;
void xglLoadMatrixf	( GLfloat *m ) ;
void xglLoadName	( GLuint name ) ;
void xglLogicOp		( GLenum opcode ) ;
void xglMap1d		( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, GLdouble *points ) ;
void xglMap1f		( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, GLfloat *points ) ;
void xglMap2d		( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble *points ) ;
void xglMap2f		( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat *points ) ;
void xglMapGrid1d	( GLint un, GLdouble u1, GLdouble u2 ) ;
void xglMapGrid1f	( GLint un, GLfloat u1, GLfloat u2 ) ;
void xglMapGrid2d	( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 ) ;
void xglMapGrid2f	( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 ) ;
void xglMaterialf	( GLenum face, GLenum pname, GLfloat param ) ;
void xglMaterialfv	( GLenum face, GLenum pname, GLfloat *params ) ;
void xglMateriali	( GLenum face, GLenum pname, GLint param ) ;
void xglMaterialiv	( GLenum face, GLenum pname, GLint *params ) ;
void xglMatrixMode	( GLenum mode ) ;
void xglMultMatrixd	( GLdouble *m ) ;
void xglMultMatrixf	( GLfloat *m ) ;
void xglNewList		( GLuint list, GLenum mode ) ;
void xglNormal3b	( GLbyte nx, GLbyte ny, GLbyte nz ) ;
void xglNormal3bv	( GLbyte *v ) ;
void xglNormal3d	( GLdouble nx, GLdouble ny, GLdouble nz ) ;
void xglNormal3dv	( GLdouble *v ) ;
void xglNormal3f	( GLfloat nx, GLfloat ny, GLfloat nz ) ;
void xglNormal3fv	( GLfloat *v ) ;
void xglNormal3i	( GLint nx, GLint ny, GLint nz ) ;
void xglNormal3iv	( GLint *v ) ;
void xglNormal3s	( GLshort nx, GLshort ny, GLshort nz ) ;
void xglNormal3sv	( GLshort *v ) ;
void xglNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count, void *ptr ) ;
void xglOrtho		( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val ) ;
void xglPassThrough	( GLfloat token ) ;
void xglPixelMapfv	( GLenum map, GLint mapsize, GLfloat *values ) ;
void xglPixelMapuiv	( GLenum map, GLint mapsize, GLuint *values ) ;
void xglPixelMapusv	( GLenum map, GLint mapsize, GLushort *values ) ;
void xglPixelStoref	( GLenum pname, GLfloat param ) ;
void xglPixelStorei	( GLenum pname, GLint param ) ;
void xglPixelTransferf	( GLenum pname, GLfloat param ) ;
void xglPixelTransferi	( GLenum pname, GLint param ) ;
void xglPixelZoom	( GLfloat xfactor, GLfloat yfactor ) ;
void xglPointSize	( GLfloat size ) ;
void xglPolygonMode	( GLenum face, GLenum mode ) ;
void xglPolygonOffsetEXT( GLfloat factor, GLfloat bias ) ;
void xglPolygonOffset   ( GLfloat factor, GLfloat bias ) ;
void xglPolygonStipple	( GLubyte *mask ) ;
void xglPopAttrib	() ;
void xglPopMatrix	() ;
void xglPopName		() ;
void xglPushAttrib	( GLbitfield mask ) ;
void xglPushMatrix	() ;
void xglPushName	( GLuint name ) ;
void xglRasterPos2d	( GLdouble x, GLdouble y ) ;
void xglRasterPos2dv	( GLdouble *v ) ;
void xglRasterPos2f	( GLfloat x, GLfloat y ) ;
void xglRasterPos2fv	( GLfloat *v ) ;
void xglRasterPos2i	( GLint x, GLint y ) ;
void xglRasterPos2iv	( GLint *v ) ;
void xglRasterPos2s	( GLshort x, GLshort y ) ;
void xglRasterPos2sv	( GLshort *v ) ;
void xglRasterPos3d	( GLdouble x, GLdouble y, GLdouble z ) ;
void xglRasterPos3dv	( GLdouble *v ) ;
void xglRasterPos3f	( GLfloat x, GLfloat y, GLfloat z ) ;
void xglRasterPos3fv	( GLfloat *v ) ;
void xglRasterPos3i	( GLint x, GLint y, GLint z ) ;
void xglRasterPos3iv	( GLint *v ) ;
void xglRasterPos3s	( GLshort x, GLshort y, GLshort z ) ;
void xglRasterPos3sv	( GLshort *v ) ;
void xglRasterPos4d	( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) ;
void xglRasterPos4dv	( GLdouble *v ) ;
void xglRasterPos4f	( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) ;
void xglRasterPos4fv	( GLfloat *v ) ;
void xglRasterPos4i	( GLint x, GLint y, GLint z, GLint w ) ;
void xglRasterPos4iv	( GLint *v ) ;
void xglRasterPos4s	( GLshort x, GLshort y, GLshort z, GLshort w ) ;
void xglRasterPos4sv	( GLshort *v ) ;
void xglReadBuffer	( GLenum mode ) ;
void xglReadPixels	( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels ) ;
void xglRectd		( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) ;
void xglRectdv		( GLdouble *v1, GLdouble *v2 ) ;
void xglRectf		( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) ;
void xglRectfv		( GLfloat *v1, GLfloat *v2 ) ;
void xglRecti		( GLint x1, GLint y1, GLint x2, GLint y2 ) ;
void xglRectiv		( GLint *v1, GLint *v2 ) ;
void xglRects		( GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) ;
void xglRectsv		( GLshort *v1, GLshort *v2 ) ;
void xglRotated		( GLdouble angle, GLdouble x, GLdouble y, GLdouble z ) ;
void xglRotatef		( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) ;
void xglScaled		( GLdouble x, GLdouble y, GLdouble z ) ;
void xglScalef		( GLfloat x, GLfloat y, GLfloat z ) ;
void xglScissor		( GLint x, GLint y, GLsizei width, GLsizei height) ;
void xglSelectBuffer	( GLsizei size, GLuint *buffer ) ;
void xglShadeModel	( GLenum mode ) ;
void xglStencilFunc	( GLenum func, GLint ref, GLuint mask ) ;
void xglStencilMask	( GLuint mask ) ;
void xglStencilOp	( GLenum fail, GLenum zfail, GLenum zpass ) ;
void xglTexCoord1d	( GLdouble s ) ;
void xglTexCoord1dv	( GLdouble *v ) ;
void xglTexCoord1f	( GLfloat s ) ;
void xglTexCoord1fv	( GLfloat *v ) ;
void xglTexCoord1i	( GLint s ) ;
void xglTexCoord1iv	( GLint *v ) ;
void xglTexCoord1s	( GLshort s ) ;
void xglTexCoord1sv	( GLshort *v ) ;
void xglTexCoord2d	( GLdouble s, GLdouble t ) ;
void xglTexCoord2dv	( GLdouble *v ) ;
void xglTexCoord2f	( GLfloat s, GLfloat t ) ;
void xglTexCoord2fv	( GLfloat *v ) ;
void xglTexCoord2i	( GLint s, GLint t ) ;
void xglTexCoord2iv	( GLint *v ) ;
void xglTexCoord2s	( GLshort s, GLshort t ) ;
void xglTexCoord2sv	( GLshort *v ) ;
void xglTexCoord3d	( GLdouble s, GLdouble t, GLdouble r ) ;
void xglTexCoord3dv	( GLdouble *v ) ;
void xglTexCoord3f	( GLfloat s, GLfloat t, GLfloat r ) ;
void xglTexCoord3fv	( GLfloat *v ) ;
void xglTexCoord3i	( GLint s, GLint t, GLint r ) ;
void xglTexCoord3iv	( GLint *v ) ;
void xglTexCoord3s	( GLshort s, GLshort t, GLshort r ) ;
void xglTexCoord3sv	( GLshort *v ) ;
void xglTexCoord4d	( GLdouble s, GLdouble t, GLdouble r, GLdouble q ) ;
void xglTexCoord4dv	( GLdouble *v ) ;
void xglTexCoord4f	( GLfloat s, GLfloat t, GLfloat r, GLfloat q ) ;
void xglTexCoord4fv	( GLfloat *v ) ;
void xglTexCoord4i	( GLint s, GLint t, GLint r, GLint q ) ;
void xglTexCoord4iv	( GLint *v ) ;
void xglTexCoord4s	( GLshort s, GLshort t, GLshort r, GLshort q ) ;
void xglTexCoord4sv	( GLshort *v ) ;
void xglTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, void *ptr ) ;
void xglTexEnvf		( GLenum target, GLenum pname, GLfloat param ) ;
void xglTexEnvfv	( GLenum target, GLenum pname, GLfloat *params ) ;
void xglTexEnvi		( GLenum target, GLenum pname, GLint param ) ;
void xglTexEnviv	( GLenum target, GLenum pname, GLint *params ) ;
void xglTexGend		( GLenum coord, GLenum pname, GLdouble param ) ;
void xglTexGendv	( GLenum coord, GLenum pname, GLdouble *params ) ;
void xglTexGenf		( GLenum coord, GLenum pname, GLfloat param ) ;
void xglTexGenfv	( GLenum coord, GLenum pname, GLfloat *params ) ;
void xglTexGeni		( GLenum coord, GLenum pname, GLint param ) ;
void xglTexGeniv	( GLenum coord, GLenum pname, GLint *params ) ;
void xglTexImage1D	( GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, GLvoid *pixels ) ;
void xglTexImage2D	( GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, GLvoid *pixels ) ;
void xglTexParameterf	( GLenum target, GLenum pname, GLfloat param ) ;
void xglTexParameterfv	( GLenum target, GLenum pname, GLfloat *params ) ;
void xglTexParameteri	( GLenum target, GLenum pname, GLint param ) ;
void xglTexParameteriv	( GLenum target, GLenum pname, GLint *params ) ;
void xglTranslated	( GLdouble x, GLdouble y, GLdouble z ) ;
void xglTranslatef	( GLfloat x, GLfloat y, GLfloat z ) ;
void xglVertex2d	( GLdouble x, GLdouble y ) ;
void xglVertex2dv	( GLdouble *v ) ;
void xglVertex2f	( GLfloat x, GLfloat y ) ;
void xglVertex2fv	( GLfloat *v ) ;
void xglVertex2i	( GLint x, GLint y ) ;
void xglVertex2iv	( GLint *v ) ;
void xglVertex2s	( GLshort x, GLshort y ) ;
void xglVertex2sv	( GLshort *v ) ;
void xglVertex3d	( GLdouble x, GLdouble y, GLdouble z ) ;
void xglVertex3dv	( GLdouble *v ) ;
void xglVertex3f	( GLfloat x, GLfloat y, GLfloat z ) ;
void xglVertex3fv	( GLfloat *v ) ;
void xglVertex3i	( GLint x, GLint y, GLint z ) ;
void xglVertex3iv	( GLint *v ) ;
void xglVertex3s	( GLshort x, GLshort y, GLshort z ) ;
void xglVertex3sv	( GLshort *v ) ;
void xglVertex4d	( GLdouble x, GLdouble y, GLdouble z, GLdouble w ) ;
void xglVertex4dv	( GLdouble *v ) ;
void xglVertex4f	( GLfloat x, GLfloat y, GLfloat z, GLfloat w ) ;
void xglVertex4fv	( GLfloat *v ) ;
void xglVertex4i	( GLint x, GLint y, GLint z, GLint w ) ;
void xglVertex4iv	( GLint *v ) ;
void xglVertex4s	( GLshort x, GLshort y, GLshort z, GLshort w ) ;
void xglVertex4sv	( GLshort *v ) ;
void xglVertexPointerEXT( GLint size, GLenum type, GLsizei stride, GLsizei count, void *ptr ) ;
void xglViewport	( GLint x, GLint y, GLsizei width, GLsizei height ) ;

void xglutAddMenuEntry		( char *label, int value ) ;
void xglutAttachMenu		( int button ) ;
int  xglutCreateMenu		( void (*)(int) ) ;
int  xglutCreateWindow		( char *title ) ;
void xglutDisplayFunc		( void (*)(void) ) ;
void xglutIdleFunc		( void (*)(void) ) ;
void xglutInit			( int *argcp, char **argv ) ;
void xglutInitDisplayMode	( unsigned int mode ) ;
void xglutInitWindowPosition	( int x, int y ) ;
void xglutInitWindowSize	( int width, int height ) ;
void xglutKeyboardFunc		( void (*)(unsigned char key, int x, int y) ) ;
void xglutMainLoopUpdate	() ;
void xglutPostRedisplay		() ;
void xglutPreMainLoop		() ;
void xglutReshapeFunc		( void (*)(int width, int height) ) ;
void xglutSwapBuffers		() ;

GLboolean xglAreTexturesResident( GLsizei n, GLuint *textures, GLboolean *residences ) ;
GLboolean xglIsTexture          ( GLuint texture ) ;
void xglBindTexture             ( GLenum target, GLuint texture ) ;
void xglDeleteTextures          ( GLsizei n, GLuint *textures ) ;
void xglGenTextures             ( GLsizei n, GLuint *textures ) ;
void xglPrioritizeTextures      ( GLsizei n, GLuint *textures, GLclampf *priorities ) ;

GLboolean xglAreTexturesResidentEXT ( GLsizei n, GLuint *textures, GLboolean *residences ) ;
GLboolean xglIsTextureEXT           ( GLuint texture ) ;
void xglBindTextureEXT              ( GLenum target, GLuint texture ) ;
void xglDeleteTexturesEXT           ( GLsizei n, GLuint *textures ) ;
void xglGenTexturesEXT              ( GLsizei n, GLuint *textures ) ;
void xglPrioritizeTexturesEXT       ( GLsizei n, GLuint *textures, GLclampf *priorities ) ;

#ifdef __cplusplus
}; 
#endif

#endif


#ifdef __cplusplus
}
#endif


#endif /* _XGL_H */
