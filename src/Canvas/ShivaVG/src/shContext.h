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

#ifndef __SHCONTEXT_H
#define __SHCONTEXT_H

#include "shDefs.h"
#include "shVectors.h"
#include "shArrays.h"
#include "shPath.h"
#include "shPaint.h"
#include "shImage.h"

/*------------------------------------------------
 * VGContext object
 *------------------------------------------------*/

typedef enum
{
  SH_RESOURCE_INVALID   = 0,
  SH_RESOURCE_PATH      = 1,
  SH_RESOURCE_PAINT     = 2,
  SH_RESOURCE_IMAGE     = 3
} SHResourceType;

typedef struct
{
  /* Surface info (since no EGL yet) */
  SHint surfaceWidth;
  SHint surfaceHeight;
  
  /* GetString info */
  char vendor[256];
  char renderer[256];
  char version[256];
  char extensions[256];
  
  /* Mode settings */
  VGMatrixMode        matrixMode;
	VGFillRule          fillRule;
	VGImageQuality      imageQuality;
	VGRenderingQuality  renderingQuality;
	VGBlendMode         blendMode;
	VGImageMode         imageMode;
  
	/* Scissor rectangles */
	SHRectArray        scissor;
  VGboolean          scissoring;
  VGboolean          masking;
  
	/* Stroke parameters */
  SHfloat           strokeLineWidth;
  VGCapStyle        strokeCapStyle;
  VGJoinStyle       strokeJoinStyle;
  SHfloat           strokeMiterLimit;
  SHFloatArray      strokeDashPattern;
  SHfloat           strokeDashPhase;
  VGboolean         strokeDashPhaseReset;
  
  /* Edge fill color for vgConvolve and pattern paint */
  SHColor           tileFillColor;
  
  /* Color for vgClear */
  SHColor           clearColor;
  
  /* Color components layout inside pixel */
  VGPixelLayout     pixelLayout;
  
  /* Source format for image filters */
  VGboolean         filterFormatLinear;
  VGboolean         filterFormatPremultiplied;
  VGbitfield        filterChannelMask;
  
  /* Matrices */
  SHMatrix3x3       pathTransform;
  SHMatrix3x3       imageTransform;
  SHMatrix3x3       fillTransform;
  SHMatrix3x3       strokeTransform;
  
  /* Paints */
  SHPaint*          fillPaint;
  SHPaint*          strokePaint;
  SHPaint           defaultPaint;
  
  VGErrorCode       error;
  
  /* Resources */
  SHPathArray       paths;
  SHPaintArray      paints;
  SHImageArray      images;

  /* Pointers to extensions */
  SHint isGLAvailable_ClampToEdge;
  SHint isGLAvailable_MirroredRepeat;
  SHint isGLAvailable_Multitexture;
  SHint isGLAvailable_TextureNonPowerOfTwo;
  SH_PGLACTIVETEXTURE pglActiveTexture;
  SH_PGLMULTITEXCOORD1F pglMultiTexCoord1f;
  SH_PGLMULTITEXCOORD2F pglMultiTexCoord2f;
  
} VGContext;

void VGContext_ctor(VGContext *c);
void VGContext_dtor(VGContext *c);
void shSetError(VGContext *c, VGErrorCode e);
SHint shIsValidPath(VGContext *c, VGHandle h);
SHint shIsValidPaint(VGContext *c, VGHandle h);
SHint shIsValidImage(VGContext *c, VGHandle h);
SHResourceType shGetResourceType(VGContext *c, VGHandle h);
VGContext* shGetContext();

/*----------------------------------------------------
 * TODO: Add mutex locking/unlocking to these macros
 * to assure sequentiallity in multithreading.
 *----------------------------------------------------*/

#define VG_NO_RETVAL

#define VG_GETCONTEXT(RETVAL) \
  VGContext *context = shGetContext(); \
  if (!context) return RETVAL;
  
#define VG_RETURN(RETVAL) \
  { return RETVAL; }

#define VG_RETURN_ERR(ERRORCODE, RETVAL) \
  { shSetError(context,ERRORCODE); return RETVAL; }

#define VG_RETURN_ERR_IF(COND, ERRORCODE, RETVAL) \
  { if (COND) {shSetError(context,ERRORCODE); return RETVAL;} }

/*-----------------------------------------------------------
 * Same macros but no mutex handling - used by sub-functions
 *-----------------------------------------------------------*/

#define SH_NO_RETVAL

#define SH_GETCONTEXT(RETVAL) \
  VGContext *context = shGetContext(); \
  if (!context) return RETVAL;
  
#define SH_RETURN(RETVAL) \
  { return RETVAL; }

#define SH_RETURN_ERR(ERRORCODE, RETVAL) \
  { shSetError(context,ERRORCODE); return RETVAL; }

#define SH_RETURN_ERR_IF(COND, ERRORCODE, RETVAL) \
  { if (COND) {shSetError(context,ERRORCODE); return RETVAL;} }


#endif /* __SHCONTEXT_H */
