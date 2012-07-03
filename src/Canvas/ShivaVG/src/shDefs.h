/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __SHDEFS_H
#define __SHDEFS_H

/* Standard headers */

#include "shConfig.h"

#ifdef VG_API_WINDOWS
#  include <windows.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#ifndef VG_API_MACOSX
#  include <malloc.h>
#endif

/* Disable VGHandle-pointer conversion warnings since we
   do deal with it by defining VGHandle properly */

#if defined(_MSC_VER)
#  pragma warning(disable:4311)
#  pragma warning(disable:4312)
#endif

/* Type definitions */

#if HAVE_INTTYPES_H

    #include <inttypes.h>

#else

    #define int8_t    char
    #define uint8_t   unsigned char
    #define int16_t   short
    #define uint16_t  unsigned short
    #define int32_t   int
    #define uint32_t  unsigned int
    #define int64_t   long long
    #define uint64_t  unsigned long long

#endif

typedef int8_t      SHint8;
typedef uint8_t     SHuint8;
typedef int16_t     SHint16;
typedef uint16_t    SHuint16;
typedef int32_t     SHint32;
typedef uint32_t    SHuint32;
typedef float       SHfloat32;

#define SHint   SHint32
#define SHuint  SHuint32
#define SHfloat SHfloat32

/* Maximum / minimum values */

#define SH_MAX_INT  (0x7fffffff)
#define SH_MIN_INT (-0x7fffffff-1)

#define SH_MANTISSA_BITS   23
#define SH_EXPONENT_BITS   8

/* all 1s in exponent yields NaN in IEEE 754 so we take
   1 less then maximum representable with exponent bits */
#define SH_MAX_EXPONENT ((1 << SH_EXPONENT_BITS) - 2)
/* maximum representable with mantissa bits */
#define SH_MAX_MANTISSA ((1 << SH_MANTISSA_BITS) - 1)
/* compose into IEEE754 floating point bit value */
#define SH_MAX_FLOAT_BITS (SH_MAX_EXPONENT << SH_MANTISSA_BITS) | SH_MAX_MANTISSA

typedef union {
  float f;
  unsigned int i;
} SHfloatint;

SHfloat getMaxFloat();

/* Portable function definitions */

#define SH_SQRT   (float)sqrt
#define SH_COS    (float)cos
#define SH_SIN    (float)sin
#define SH_ACOS   (float)acos
#define SH_ASIN   (float)asin
#define SH_ATAN   (float)atan
#define SH_FLOOR  (float)floor
#define SH_CEIL   (float)ceil
#define SH_ASSERT assert

#if defined(__isnan) || (defined(__APPLE__) && (__GNUC__ == 3))
#  define SH_ISNAN __isnan
#elif defined(_isnan) || defined(WIN32)
#  define SH_ISNAN _isnan
#else
#  define SH_ISNAN isnan
#endif


/* Helper macros */

#define PI 3.141592654f
#define SH_DEG2RAD(a) (a * PI / 180.0f)
#define SH_RAD2DEG(a) (a * 180.0f / PI)
#define SH_ABS(a) ((a < 0.0f) ? -a : a)
#define SH_MAX(a,b) ((a > b) ? a : b)
#define SH_MIN(a,b) ((a < b) ? a : b)
#define SH_NEARZERO(a) (a >= -0.0001 && a < 0.0001)
#define SH_SWAP(a,b) {SHfloat t=a; a=b; b=t;}
#define SH_CLAMP(a,min,max) {if (a<min) a=min; if (a>max) a=max; }

#define SH_NEWOBJ(type,obj) { obj = (type*)malloc(sizeof(type)); if(obj) type ## _ctor(obj); }
#define SH_INITOBJ(type,obj){ type ## _ctor(&obj); }
#define SH_DEINITOBJ(type,obj) { type ## _dtor(&obj); }
#define SH_DELETEOBJ(type,obj) { if(obj) type ## _dtor(obj); free(obj); }

/* Implementation limits */

#define SH_MAX_SCISSOR_RECTS             1
#define SH_MAX_DASH_COUNT                VG_MAXINT
#define SH_MAX_IMAGE_WIDTH               VG_MAXINT
#define SH_MAX_IMAGE_HEIGHT              VG_MAXINT
#define SH_MAX_IMAGE_PIXELS              VG_MAXINT
#define SH_MAX_IMAGE_BYTES               VG_MAXINT
#define SH_MAX_COLOR_RAMP_STOPS          256

#define SH_MAX_VERTICES 999999999
#define SH_MAX_RECURSE_DEPTH 16

#define SH_GRADIENT_TEX_SIZE       1024
#define SH_GRADIENT_TEX_COORDSIZE   4096 /* 1024 * RGBA */

/* OpenGL headers */

#if defined(VG_API_LINUX)
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glx.h>
#elif defined(VG_API_MACOSX)
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#elif defined(VG_API_WINDOWS)
    #include <GL/gl.h>
    #include <GL/glu.h>
#else
    #define GL_GLEXT_LEGACY /* don't include glext.h */
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/glx.h>
#endif

#include "shExtensions.h"


#endif /* __SHDEFS_H */
