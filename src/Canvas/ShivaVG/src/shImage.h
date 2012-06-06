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

#ifndef __SHIMAGE_H
#define __SHIMAGE_H

#include "shDefs.h"

/*-----------------------------------------------------------
 * ColorFormat holds the data necessary to pack/unpack color
 * components from a pixel of each supported image format
 *-----------------------------------------------------------*/

typedef struct
{
  VGImageFormat vgformat;
  SHuint8 bytes;
  
  SHuint32 rmask;
  SHuint8 rshift;
  SHuint8 rmax;
  
  SHuint32 gmask;
  SHuint8 gshift;
  SHuint8 gmax;
  
  SHuint32 bmask;
  SHuint8 bshift;
  SHuint8 bmax;
  
  SHuint32 amask;
  SHuint8 ashift;
  SHuint8 amax;

  GLenum glintformat;
  GLenum glformat;
  GLenum gltype;

} SHImageFormatDesc;

typedef struct
{
  SHfloat r,g,b,a;
  
} SHColor;

void SHColor_ctor(SHColor *c);
void SHColor_dtor(SHColor *c);

#define _ITEM_T SHColor
#define _ARRAY_T SHColorArray
#define _FUNC_T shColorArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

typedef struct
{
  SHuint8 *data;
  SHint width;
  SHint height;
  SHImageFormatDesc fd;
  
  SHint texwidth;
  SHint texheight;
  SHfloat texwidthK;
  SHfloat texheightK;
  GLuint texture;
  
} SHImage;

void SHImage_ctor(SHImage *i);
void SHImage_dtor(SHImage *i);

#define _ITEM_T SHImage*
#define _ARRAY_T SHImageArray
#define _FUNC_T shImageArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

/*-------------------------------------------------------
 * Color operators
 *-------------------------------------------------------*/

#define CSET(c, rr,gg,bb,aa) { c.r=rr; c.g=gg; c.b=bb; c.a=aa; }
#define CSETC(c1, c2) { c1.r=c2.r; c1.g=c2.g; c1.b=c2.b; c1.a=c2.a; }

#define CSUB(c1, rr,gg,bb,aa) { c.r-=rr; c.g-=gg; c.b-=bb; c.a-=aa; }
#define CSUBC(c1, c2) { c1.r-=c2.r; c1.g-=c2.g; c1.b-=c2.b; c1.a-=c2.a; }
#define CSUBCTO(c1, c2, c3) { c3.r=c1.r-c2.r; c3.g=c1.g-c2.g;  c3.b=c1.b-c2.b; c3.a=c1.a-c2.a; }

#define CADD(c1, rr,gg,bb,aa) { c.r+=rr; c.g+=gg; c.b+=bb; c.a+=aa; }
#define CADDC(c1, c2) { c1.r+=c2.r; c1.g+=c2.g; c1.b+=c2.b; c1.a+=c2.a; }
#define CADDTO(c1, c2, c3) { c3.r=c1.r+c2.r; c3.g=c1.g+c2.g;  c3.b=c1.b+c2.b; c3.a=c1.a+c2.a; }
#define CADDCK(c1, c2, k) { c1.r+=k*c2.r; c1.g+=k*c2.g; c1.b+=k*c2.b; c1.a+=k*c2.a; }

#define CMUL(c, s) { c.r*=s; c.g*=s; c.b*=s; c.a*=s; }
#define CDIV(c, s) { c.r/=s; c.g/=s; c.b/=s; c.a/=s; }

#define CPREMUL(c) { c.r*=c.a; c.g*=c.a; c.b*=c.a; }
#define CUNPREMUL(c) { c.r/=c.a; c.g/=c.a; c.b/=c.a; }

/*-------------------------------------------------------
 * Color-to-memory functions
 *-------------------------------------------------------*/

#define CSTORE_RGBA1D_8(c, rgba, x)  { \
  rgba[x*4+0] = (SHuint8)SH_FLOOR(c.r * 255); \
  rgba[x*4+1] = (SHuint8)SH_FLOOR(c.g * 255); \
  rgba[x*4+2] = (SHuint8)SH_FLOOR(c.b * 255); \
  rgba[x*4+3] = (SHuint8)SH_FLOOR(c.a * 255); }

#define CSTORE_RGBA1D_F(c, rgba, x)  { \
  rgba[x*4+0] = c.r; \
  rgba[x*4+1] = c.g; \
  rgba[x*4+2] = c.b; \
  rgba[x*4+3] = c.a; }

#define CSTORE_RGBA2D_8(c, rgba, x, y, width) { \
  rgba[y*width*4+x*4+0] = (SHuint8)SH_FLOOR(c.r * 255); \
  rgba[y*width*4+x*4+1] = (SHuint8)SH_FLOOR(c.g * 255); \
  rgba[y*width*4+x*4+2] = (SHuint8)SH_FLOOR(c.b * 255); \
  rgba[y*width*4+x*4+3] = (SHuint8)SH_FLOOR(c.a * 255); }

#define CSTORE_RGBA2D_F(c, rgba, x, y, width) { \
  rgba[y*width*4+x*4+0] = c.r; \
  rgba[y*width*4+x*4+1] = c.g; \
  rgba[y*width*4+x*4+2] = c.b; \
  rgba[y*width*4+x*4+3] = c.a; }

#define INT2COLCOORD(i, max) ( (SHfloat)i / (SHfloat)max  )
#define COL2INTCOORD(c, max) ( (SHuint)SH_FLOOR(c * (SHfloat)max + 0.5f) )

void shStoreColor(SHColor *c, void *data, SHImageFormatDesc *f);
void shLoadColor(SHColor *c, const void *data, SHImageFormatDesc *f);


#endif /* __SHIMAGE_H */
