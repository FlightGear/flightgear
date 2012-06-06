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

#ifndef __SHPAINT_H
#define __SHPAINT_H

#include "shDefs.h"
#include "shArrays.h"
#include "shImage.h"

typedef struct
{
  float offset;
  SHColor color;
  
} SHStop;

#define _ITEM_T SHStop
#define _ARRAY_T SHStopArray
#define _FUNC_T shStopArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

typedef struct
{
  VGPaintType type;
  SHColor color;
  SHColorArray colors;
  SHStopArray instops;
  SHStopArray stops;
  VGboolean premultiplied;
  VGColorRampSpreadMode spreadMode;
  VGTilingMode tilingMode;
  SHfloat linearGradient[4];
  SHfloat radialGradient[5];
  GLuint texture;
  VGImage pattern;
  
} SHPaint;

#define SH_GRADIENT_TEX_SIZE 1024

void SHPaint_ctor(SHPaint *p);
void SHPaint_dtor(SHPaint *p);

#define _ITEM_T SHPaint*
#define _ARRAY_T SHPaintArray
#define _FUNC_T shPaintArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

void shValidateInputStops(SHPaint *p);
void shSetGradientTexGLState(SHPaint *p);

int shDrawLinearGradientMesh(SHPaint *p, SHVector2 *min, SHVector2 *max,
                             VGPaintMode mode, GLenum texUnit);
  
int shDrawRadialGradientMesh(SHPaint *p, SHVector2 *min, SHVector2 *max,
                             VGPaintMode mode, GLenum texUnit);

int shDrawPatternMesh(SHPaint *p, SHVector2 *min, SHVector2 *max,
                      VGPaintMode mode, GLenum texUnit);
  

#endif /* __SHPAINT_H */
