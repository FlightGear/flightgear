
/* Copyright (c) Mark J. Kilgard, 1997.  */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */


#ifndef _TEXLOAD_H
#define _TEXLOAD_H


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include GLUT_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


extern GLubyte *read_alpha_texture(const char *name, int *width, int *height);
extern GLubyte *read_rgb_texture(const char *name, int *width, int *height);
extern GLubyte *read_raw_texture(const char *name, int *width, int *height);
extern GLubyte *read_r8_texture(const char *name, int *width, int *height);


#ifdef __cplusplus
}
#endif


#endif /* _TEXLOAD_H */
