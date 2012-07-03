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

#include <stdio.h>
#include <vg/openvg.h>
#include "shContext.h"

/*----------------------------------------------------
 * Returns true (1) if the specified parameter takes
 * a vector of values (more than 1) or false (0)
 * otherwise.
 *----------------------------------------------------*/

int shIsParamVector(SHint type)
{
  return
    (type == VG_SCISSOR_RECTS ||
     type == VG_STROKE_DASH_PATTERN ||
     type == VG_TILE_FILL_COLOR ||
     type == VG_CLEAR_COLOR ||
     type == VG_PAINT_COLOR ||
     type == VG_PAINT_COLOR_RAMP_STOPS ||
     type == VG_PAINT_LINEAR_GRADIENT ||
     type == VG_PAINT_RADIAL_GRADIENT);
}

/*----------------------------------------------------
 * Returns true (1) if the given integer is a valid
 * enumeration value for the specified parameter or
 * false (0) otherwise.
 *----------------------------------------------------*/
  
int shIsEnumValid(SHint type, VGint val)
{
  switch(type)
  {
  case VG_MATRIX_MODE:
    return (val == VG_MATRIX_PATH_USER_TO_SURFACE ||
            val == VG_MATRIX_IMAGE_USER_TO_SURFACE ||
            val == VG_MATRIX_FILL_PAINT_TO_USER ||
            val == VG_MATRIX_STROKE_PAINT_TO_USER);
    
  case VG_FILL_RULE:
    return (val == VG_EVEN_ODD ||
            val == VG_NON_ZERO)  ? VG_TRUE : VG_FALSE;
    
  case VG_IMAGE_QUALITY:
    return (val == VG_IMAGE_QUALITY_NONANTIALIASED ||
            val == VG_IMAGE_QUALITY_FASTER ||
            val == VG_IMAGE_QUALITY_BETTER);
    
  case VG_RENDERING_QUALITY:
    return (val == VG_RENDERING_QUALITY_NONANTIALIASED ||
            val == VG_RENDERING_QUALITY_FASTER ||
            val == VG_RENDERING_QUALITY_BETTER);
    
  case VG_BLEND_MODE:
    return (val == VG_BLEND_SRC ||
            val == VG_BLEND_SRC_OVER ||
            val == VG_BLEND_DST_OVER ||
            val == VG_BLEND_SRC_IN ||
            val == VG_BLEND_DST_IN ||
            val == VG_BLEND_MULTIPLY ||
            val == VG_BLEND_SCREEN ||
            val == VG_BLEND_DARKEN ||
            val == VG_BLEND_LIGHTEN ||
            val == VG_BLEND_ADDITIVE ||
            val == VG_BLEND_SRC_OUT_SH ||
            val == VG_BLEND_DST_OUT_SH ||
            val == VG_BLEND_SRC_ATOP_SH ||
            val == VG_BLEND_DST_ATOP_SH);
    
  case VG_IMAGE_MODE:
    return (val == VG_DRAW_IMAGE_NORMAL ||
            val == VG_DRAW_IMAGE_MULTIPLY ||
            val == VG_DRAW_IMAGE_STENCIL);
    
  case VG_STROKE_CAP_STYLE:
    return (val == VG_CAP_BUTT ||
            val == VG_CAP_ROUND ||
            val == VG_CAP_SQUARE);
    
  case VG_STROKE_JOIN_STYLE:
    return (val == VG_JOIN_MITER ||
            val == VG_JOIN_ROUND ||
            val == VG_JOIN_BEVEL);
    
  case VG_STROKE_DASH_PHASE_RESET:
  case VG_SCISSORING:
  case VG_MASKING:
    return (val == VG_TRUE ||
            val == VG_FALSE);
    
  case VG_PIXEL_LAYOUT:
    return (val == VG_PIXEL_LAYOUT_UNKNOWN ||
            val == VG_PIXEL_LAYOUT_RGB_VERTICAL ||
            val == VG_PIXEL_LAYOUT_BGR_VERTICAL ||
            val == VG_PIXEL_LAYOUT_RGB_HORIZONTAL ||
            val == VG_PIXEL_LAYOUT_BGR_HORIZONTAL);
    
  case VG_FILTER_FORMAT_LINEAR:
  case VG_FILTER_FORMAT_PREMULTIPLIED:
    return (val == VG_TRUE ||
            val == VG_FALSE);
    
  case VG_PAINT_TYPE:
    return (val == VG_PAINT_TYPE_COLOR ||
            val == VG_PAINT_TYPE_LINEAR_GRADIENT ||
            val == VG_PAINT_TYPE_RADIAL_GRADIENT ||
            val == VG_PAINT_TYPE_PATTERN);
    
  case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
    return (val == VG_COLOR_RAMP_SPREAD_PAD ||
            val == VG_COLOR_RAMP_SPREAD_REPEAT ||
            val == VG_COLOR_RAMP_SPREAD_REFLECT);
    
  case VG_PAINT_PATTERN_TILING_MODE:
    return (VG_TILE_FILL ||
            VG_TILE_PAD ||
            VG_TILE_REPEAT ||
            VG_TILE_REFLECT);
    
  case VG_IMAGE_FORMAT:
    return (val >= VG_sRGBX_8888 &&
            val <= VG_lABGR_8888_PRE);
    
  default:
    return 1;
  }
}

/*--------------------------------------------------------
 * These two functions check for invalid (erroneus) float
 * input and correct it to acceptable ranges.
 *---------------------------------------------------*/

SHfloat getMaxFloat()
{
  SHfloatint fi;
  fi.i = SH_MAX_FLOAT_BITS;
  return fi.f;
}

SHfloat shValidInputFloat(VGfloat f)
{
  SHfloat max = getMaxFloat();
  if (SH_ISNAN(f)) return 0.0f; /* convert NAN to zero */
  SH_CLAMP(f, -max, max); /* clamp to valid range */
  return (SHfloat)f;
}

static SHint shValidInputFloat2Int(VGfloat f)
{
  double v = (double)SH_FLOOR(shValidInputFloat(f));
  SH_CLAMP(v, (double)SH_MIN_INT, (double)SH_MAX_INT);
  return (SHint)v;
}

/*---------------------------------------------------
 * Interpretes the input value vector as an array of
 * integers and returns the value at given index
 *---------------------------------------------------*/

static SHint shParamToInt(const void *values, SHint floats, SHint index)
{
  if (floats)
    return shValidInputFloat2Int(((const VGfloat*)values)[index]);
  else
    return (SHint)((const VGint*)values)[index];
}

/*---------------------------------------------------
 * Interpretes the input value vector as an array of
 * floats and returns the value at given index
 *---------------------------------------------------*/

static VGfloat shParamToFloat(const void *values, SHint floats, SHint index)
{
  if (floats)
    return shValidInputFloat(((const VGfloat*)values)[index]);
  else
    return (SHfloat)((const VGint*)values)[index];
}

/*---------------------------------------------------
 * Interpretes the output value vector as an array of
 * integers and sets the value at given index
 *---------------------------------------------------*/

static void shIntToParam(SHint i, SHint count, void *output, SHint floats, SHint index)
{
  if (index >= count)
    return;
  if (floats)
    ((VGfloat*)output)[index] = (VGfloat)i;
  else
    ((VGint*)output)[index] = (VGint)i;
}

/*----------------------------------------------------
 * Interpretes the output value vector as an array of
 * floats and sets the value at given index
 *----------------------------------------------------*/

static void shFloatToParam(SHfloat f, SHint count, void *output, SHint floats, SHint index)
{
  if (index >= count)
    return;
  if (floats)
    ((VGfloat*)output)[index] = (VGfloat)f;
  else
    ((VGint*)output)[index] = (VGint)shValidInputFloat2Int(f);
}

/*---------------------------------------------------------
 * Sets a parameter by interpreting the input value vector
 * according to the parameter type and input type.
 *---------------------------------------------------------*/

static void shSet(VGContext *context, VGParamType type, SHint count,
                  const void* values, SHint floats)
{
  SHfloat fvalue = 0.0f;
  SHint ivalue = 0;
  VGboolean bvalue = VG_FALSE;
  int i = 0;
  
  /* Check for negative count */
  SH_RETURN_ERR_IF(count<0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  /* Check for empty vector */
  SH_RETURN_ERR_IF(!values && count!=0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  /* Pre-convert first value for non-vector params */
  if (count == 1) {
    fvalue = shParamToFloat(values, floats, 0);
    ivalue = shParamToInt(values, floats, 0);
    bvalue = (ivalue ? VG_TRUE : VG_FALSE);
  }
  
  switch (type)
  {
  case VG_MATRIX_MODE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->matrixMode = (VGMatrixMode)ivalue;
    break;
    
  case VG_FILL_RULE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->fillRule = (VGFillRule)ivalue;
    break;
    
  case VG_IMAGE_QUALITY:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->imageQuality = (VGImageQuality)ivalue;
    break;
    
  case VG_RENDERING_QUALITY:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->renderingQuality = (VGRenderingQuality)ivalue;
    break;
    
  case VG_BLEND_MODE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->blendMode = (VGBlendMode)ivalue;
    break;
    
  case VG_IMAGE_MODE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->imageMode = (VGImageMode)ivalue;
    break;
    
  case VG_STROKE_CAP_STYLE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeCapStyle = (VGCapStyle)ivalue;
    break;
    
  case VG_STROKE_JOIN_STYLE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeJoinStyle = (VGJoinStyle)ivalue;
    break;
    
  case VG_PIXEL_LAYOUT:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    SH_RETURN_ERR_IF(!shIsEnumValid(type,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->pixelLayout = (VGPixelLayout)ivalue;
    break;
    
  case VG_FILTER_CHANNEL_MASK:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->filterChannelMask = (VGbitfield)ivalue;
    break;
    
  case VG_FILTER_FORMAT_LINEAR:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->filterFormatLinear = bvalue;
    break;
    
  case VG_FILTER_FORMAT_PREMULTIPLIED:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->filterFormatPremultiplied = bvalue;
    break;
    
  case VG_STROKE_DASH_PHASE_RESET:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeDashPhaseReset = bvalue;
    break;
    
  case VG_MASKING:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->masking = bvalue;
    break;
    
  case VG_SCISSORING:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->scissoring = bvalue;
    break;
    
  case VG_STROKE_LINE_WIDTH:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeLineWidth = fvalue;
    break;
    
  case VG_STROKE_MITER_LIMIT:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeMiterLimit = SH_MAX(fvalue, 1.0f);
    break;
    
  case VG_STROKE_DASH_PHASE:
    SH_RETURN_ERR_IF(count!=1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    context->strokeDashPhase = fvalue;
    break;
    
  case VG_STROKE_DASH_PATTERN:
    
    /* TODO: limit by the VG_MAX_DASH_COUNT value */
    shFloatArrayClear(&context->strokeDashPattern);
    for (i=0; i<count; ++i)
      shFloatArrayPushBack(&context->strokeDashPattern,
                           shParamToFloat(values, floats, i));
    break;
  case VG_TILE_FILL_COLOR:
    
    SH_RETURN_ERR_IF(count!=4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    CSET(context->tileFillColor,
         shParamToFloat(values, floats, 0),
         shParamToFloat(values, floats, 1),
         shParamToFloat(values, floats, 2),
         shParamToFloat(values, floats, 3));
    
    break;
  case VG_CLEAR_COLOR:
    
    SH_RETURN_ERR_IF(count!=4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    CSET(context->clearColor,
         shParamToFloat(values, floats, 0),
         shParamToFloat(values, floats, 1),
         shParamToFloat(values, floats, 2),
         shParamToFloat(values, floats, 3));
    
    break;
  case VG_SCISSOR_RECTS:
    
    SH_RETURN_ERR_IF(count % 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shRectArrayClear(&context->scissor);
    for (i=0; i<count && i<SH_MAX_SCISSOR_RECTS; i+=4) {
      SHRectangle r;
      r.x = shParamToFloat(values, floats, i+0);
      r.y = shParamToFloat(values, floats, i+1);
      r.w = shParamToFloat(values, floats, i+2);
      r.h = shParamToFloat(values, floats, i+3);
      shRectArrayPushBackP(&context->scissor, &r);
    }
    
    break;
    
  case VG_MAX_SCISSOR_RECTS:
  case VG_MAX_DASH_COUNT:
  case VG_MAX_KERNEL_SIZE:
  case VG_MAX_SEPARABLE_KERNEL_SIZE:
  case VG_MAX_COLOR_RAMP_STOPS:
  case VG_MAX_IMAGE_WIDTH:
  case VG_MAX_IMAGE_HEIGHT:
  case VG_MAX_IMAGE_PIXELS:
  case VG_MAX_IMAGE_BYTES:
  case VG_MAX_FLOAT:
  case VG_MAX_GAUSSIAN_STD_DEVIATION:
    /* Read-only */ break;
    
  default:
    /* Invalid VGParamType */
    SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  }
  
  SH_RETURN(SH_NO_RETVAL);
}

/*--------------------------------------------------
 * Sets a parameter of a single integer value
 *--------------------------------------------------*/

VG_API_CALL void vgSetf (VGParamType type, VGfloat value)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Check if target vector */
  VG_RETURN_ERR_IF(shIsParamVector(type),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   VG_NO_RETVAL);
  
  /* Error code will be set by shSet */
  shSet(context, type, 1, &value, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*--------------------------------------------------
 * Sets a parameter of a single float value
 *--------------------------------------------------*/

VG_API_CALL void vgSeti (VGParamType type, VGint value)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Check if target vector */
  VG_RETURN_ERR_IF(shIsParamVector(type),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   VG_NO_RETVAL);
  
  /* Error code will be set by shSet */
  shSet(context, type, 1, &value, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*-------------------------------------------------------
 * Sets a parameter which takes a vector of float values
 *-------------------------------------------------------*/

VG_API_CALL void vgSetfv(VGParamType type, VGint count,
                         const VGfloat * values)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* TODO: check input array alignment */
  
  /* Error code will be set by shSet */
  shSet(context, type, count, values, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Sets a parameter which takes a vector of integer values
 *---------------------------------------------------------*/

VG_API_CALL void vgSetiv(VGParamType type, VGint count,
                         const VGint * values)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* TODO: check input array alignment */
  
  /* Error code wil be set by shSet */
  shSet(context, type, count, values, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Outputs a parameter by interpreting the output value
 * vector according to the parameter type and input type.
 *---------------------------------------------------------*/

static void shGet(VGContext *context, VGParamType type, SHint count, void *values, SHint floats)
{
  int i;
  
  /* Check for invalid array / count */
  SH_RETURN_ERR_IF(!values || count<=0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  switch (type)
  {
  case VG_MATRIX_MODE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->matrixMode, count, values, floats, 0);
    break;
    
  case VG_FILL_RULE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->fillRule, count, values, floats, 0);
    break;
    
  case VG_IMAGE_QUALITY:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->imageQuality, count, values, floats, 0);
    break;
    
  case VG_RENDERING_QUALITY:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->renderingQuality, count, values, floats, 0);
    break;
    
  case VG_BLEND_MODE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->blendMode, count, values, floats, 0);
    break;
    
  case VG_IMAGE_MODE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->imageMode, count, values, floats, 0);
    break;
    
  case VG_STROKE_CAP_STYLE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->strokeCapStyle, count, values, floats, 0);
    break;
    
  case VG_STROKE_JOIN_STYLE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->strokeJoinStyle, count, values, floats, 0);
    break;
    
  case VG_PIXEL_LAYOUT:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->pixelLayout, count, values, floats, 0);
    break;
    
  case VG_FILTER_CHANNEL_MASK:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->filterChannelMask, count, values, floats, 0);
    break;
    
  case VG_FILTER_FORMAT_LINEAR:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->filterFormatLinear, count, values, floats, 0);
    break;
    
  case VG_FILTER_FORMAT_PREMULTIPLIED:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->filterFormatPremultiplied, count, values, floats, 0);
    break;
    
  case VG_STROKE_DASH_PHASE_RESET:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->strokeDashPhaseReset, count, values, floats, 0);
    break;
    
  case VG_MASKING:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->masking, count, values, floats, 0);
    break;
    
  case VG_SCISSORING:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam((SHint)context->scissoring, count, values, floats, 0);
    break;
    
  case VG_STROKE_LINE_WIDTH:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(context->strokeLineWidth, count, values, floats, 0);
    break;
    
  case VG_STROKE_MITER_LIMIT:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(context->strokeMiterLimit, count, values, floats, 0);
    break;
    
  case VG_STROKE_DASH_PHASE:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(context->strokeDashPhase, count, values, floats, 0);
    break;
    
  case VG_STROKE_DASH_PATTERN:
    
    SH_RETURN_ERR_IF(count > context->strokeDashPattern.size,
                     VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    
    for (i=0; i<context->strokeDashPattern.size; ++i)
      shFloatToParam(context->strokeDashPattern.items[i],
                     count, values, floats, i);
    
    break;
  case VG_TILE_FILL_COLOR:
    
    SH_RETURN_ERR_IF(count > 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(context->tileFillColor.r, count, values, floats, 0);
    shFloatToParam(context->tileFillColor.g, count, values, floats, 1);
    shFloatToParam(context->tileFillColor.b, count, values, floats, 2);
    shFloatToParam(context->tileFillColor.a, count, values, floats, 3);
    
    break;
  case VG_CLEAR_COLOR:
    
    SH_RETURN_ERR_IF(count > 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(context->clearColor.r, count, values, floats, 0);
    shFloatToParam(context->clearColor.g, count, values, floats, 1);
    shFloatToParam(context->clearColor.b, count, values, floats, 2);
    shFloatToParam(context->clearColor.a, count, values, floats, 3);
    
    break;
  case VG_SCISSOR_RECTS:
    
    SH_RETURN_ERR_IF(count > context->scissor.size * 4,
                     VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    
    for (i=0; i<context->scissor.size; ++i) {
      shIntToParam((SHint)context->scissor.items[i].x, count, values, floats, i*4+0);
      shIntToParam((SHint)context->scissor.items[i].y, count, values, floats, i*4+1);
      shIntToParam((SHint)context->scissor.items[i].w, count, values, floats, i*4+2);
      shIntToParam((SHint)context->scissor.items[i].h, count, values, floats, i*4+3);
    }
    
    break;
    
  case VG_MAX_SCISSOR_RECTS:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_SCISSOR_RECTS, count, values, floats, 0);
    break;
    
  case VG_MAX_DASH_COUNT:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_DASH_COUNT, count, values, floats, 0);
    break;
    
  case VG_MAX_COLOR_RAMP_STOPS:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_COLOR_RAMP_STOPS, count, values, floats, 0);
    break;
    
  case VG_MAX_IMAGE_WIDTH:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_IMAGE_WIDTH, count, values, floats, 0);
    break;
    
  case VG_MAX_IMAGE_HEIGHT:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_IMAGE_WIDTH, count, values, floats, 0);
    break;
    
  case VG_MAX_IMAGE_PIXELS:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_IMAGE_HEIGHT, count, values, floats, 0);
    break;
    
  case VG_MAX_IMAGE_BYTES:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(SH_MAX_IMAGE_BYTES, count, values, floats, 0);
    break;
    
  case VG_MAX_FLOAT:
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(getMaxFloat(), count, values, floats, 0);
    break;
    
  case VG_MAX_KERNEL_SIZE: /* TODO: depends on convolution implementation */
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(0, count, values, floats, 0);
    break;
    
  case VG_MAX_SEPARABLE_KERNEL_SIZE: /* TODO: depends on convolution implementation */
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shIntToParam(0, count, values, floats, 0);
    break;
    
  case VG_MAX_GAUSSIAN_STD_DEVIATION: /* TODO: depends on gaussian blur implementation */
    SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    shFloatToParam(0.0f, count, values, floats, 0);
    break;
    
  default:
    /* Invalid VGParamType */
    SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  }
  
  SH_RETURN(SH_NO_RETVAL);
}

/*---------------------------------------------------------
 * Returns a parameter of a single float value
 *---------------------------------------------------------*/

VG_API_CALL VGfloat vgGetf(VGParamType type)
{
  VGfloat retval = 0.0f;
  VG_GETCONTEXT(retval);
  
  /* Check if target vector */
  VG_RETURN_ERR_IF(shIsParamVector(type),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   retval);
  
  /* Error code will be set by shGet */
  shGet(context, type, 1, &retval, 1);
  VG_RETURN(retval);
}

/*---------------------------------------------------------
 * Returns a parameter of a single integer value
 *---------------------------------------------------------*/

VG_API_CALL VGint vgGeti(VGParamType type)
{
  VGint retval = 0;
  VG_GETCONTEXT(retval);
  
  /* Check if target vector */
  VG_RETURN_ERR_IF(shIsParamVector(type),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   retval);
  
  /* Error code will be set by shGet */
  shGet(context, type, 1, &retval, 0);
  VG_RETURN(retval);
}

/*---------------------------------------------------------
 * Outputs a parameter of a float vector value
 *---------------------------------------------------------*/

VG_API_CALL void vgGetfv(VGParamType type, VGint count, VGfloat * values)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* TODO: check output array alignment */
  
  /* Error code will be set by shGet */
  shGet(context, type, count, values, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Outputs a parameter of an integer vector value
 *---------------------------------------------------------*/

VG_API_CALL void vgGetiv(VGParamType type, VGint count, VGint * values)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* TODO: check output array alignment */
  
  /* Error code will be set by shGet */
  shGet(context, type, count, values, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Returns the size of the output array required to
 * receive the whole vector of parameter values
 *---------------------------------------------------------*/

VG_API_CALL VGint vgGetVectorSize(VGParamType type)
{
  int retval = 0;
  VG_GETCONTEXT(retval);
  
  switch(type)
  {
  case VG_MATRIX_MODE:
  case VG_FILL_RULE:
  case VG_IMAGE_QUALITY:
  case VG_RENDERING_QUALITY:
  case VG_BLEND_MODE:
  case VG_IMAGE_MODE:
  case VG_STROKE_CAP_STYLE:
  case VG_STROKE_JOIN_STYLE:
  case VG_PIXEL_LAYOUT:
  case VG_FILTER_CHANNEL_MASK:
  case VG_FILTER_FORMAT_LINEAR:
  case VG_FILTER_FORMAT_PREMULTIPLIED:
  case VG_STROKE_DASH_PHASE_RESET:
  case VG_MASKING:
  case VG_SCISSORING:
  case VG_STROKE_LINE_WIDTH:
  case VG_STROKE_MITER_LIMIT:
  case VG_STROKE_DASH_PHASE:
  case VG_MAX_SCISSOR_RECTS:
  case VG_MAX_DASH_COUNT:
  case VG_MAX_KERNEL_SIZE:
  case VG_MAX_SEPARABLE_KERNEL_SIZE:
  case VG_MAX_COLOR_RAMP_STOPS:
  case VG_MAX_IMAGE_WIDTH:
  case VG_MAX_IMAGE_HEIGHT:
  case VG_MAX_IMAGE_PIXELS:
  case VG_MAX_IMAGE_BYTES:
  case VG_MAX_FLOAT:
  case VG_MAX_GAUSSIAN_STD_DEVIATION:
    retval = 1;
    break;
    
  case VG_TILE_FILL_COLOR:
  case VG_CLEAR_COLOR:
    retval = 4;
    break;
    
  case VG_STROKE_DASH_PATTERN:
    retval = context->strokeDashPattern.size;
    break;
    
  case VG_SCISSOR_RECTS:
    retval = context->scissor.size * 4;
    break;
    
  default:
    /* Invalid VGParamType */
    VG_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, retval);
  }
  
  VG_RETURN(retval);
}

/*-----------------------------------------------------------
 * Sets a resource parameter by interpreting the input value
 * vector according to the parameter type and input type.
 *-----------------------------------------------------------*/

static void shSetParameter(VGContext *context, VGHandle object,
                           SHResourceType rtype, VGint ptype,
                           SHint count, const void *values, SHint floats)
{
//  SHfloat fvalue = 0.0f;
  SHint ivalue = 0;
//  VGboolean bvalue = VG_FALSE;
  int i;
  
  /* Check for negative count */
  SH_RETURN_ERR_IF(count<0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  /* Check for empty vector */
  SH_RETURN_ERR_IF(!values && count!=0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  /* Pre-convert first value for non-vector params */
  if (count == 1) {
//    fvalue = shParamToFloat(values, floats, 0);
    ivalue = shParamToInt(values, floats, 0);
//    bvalue = (ivalue ? VG_TRUE : VG_FALSE);
  }
  
  switch (rtype)
  {
  case SH_RESOURCE_PATH: switch (ptype) { /* Path parameters */
      
    case VG_PATH_FORMAT:
    case VG_PATH_DATATYPE:
    case VG_PATH_SCALE:
    case VG_PATH_BIAS:
    case VG_PATH_NUM_SEGMENTS:
    case VG_PATH_NUM_COORDS:
      /* Read-only */ break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      
    } break;
  case SH_RESOURCE_PAINT: switch (ptype) { /* Paint parameters */
      
    case VG_PAINT_TYPE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      SH_RETURN_ERR_IF(!shIsEnumValid(ptype,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      ((SHPaint*)object)->type = (VGPaintType)ivalue;
      break;
      
    case VG_PAINT_COLOR:
      SH_RETURN_ERR_IF(count != 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      ((SHPaint*)object)->color.r = shParamToFloat(values, floats, 0);
      ((SHPaint*)object)->color.g = shParamToFloat(values, floats, 1);
      ((SHPaint*)object)->color.b = shParamToFloat(values, floats, 2);
      ((SHPaint*)object)->color.a = shParamToFloat(values, floats, 3);
      break;
      
    case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      SH_RETURN_ERR_IF(!shIsEnumValid(ptype,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      ((SHPaint*)object)->spreadMode = (VGColorRampSpreadMode)ivalue;
      break;
      
    case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      SH_RETURN_ERR_IF(!shIsEnumValid(ptype,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      ((SHPaint*)object)->premultiplied = (VGboolean)ivalue;
      break;
      
    case VG_PAINT_COLOR_RAMP_STOPS: {
        
        int max; SHPaint *paint; SHStop stop;
        SH_RETURN_ERR_IF(count % 5, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
        max  = SH_MIN(count, SH_MAX_COLOR_RAMP_STOPS * 5);
        paint = (SHPaint*)object;
        shStopArrayClear(&paint->instops);
        
        for (i=0; i<max; i+=5) {
          stop.offset = shParamToFloat(values, floats, i+0);
          CSET(stop.color,
               shParamToFloat(values, floats, i+1),
               shParamToFloat(values, floats, i+2),
               shParamToFloat(values, floats, i+3),
               shParamToFloat(values, floats, i+4));
          shStopArrayPushBackP(&paint->instops, &stop); }
        
        shValidateInputStops(paint);
        break;}
      
    case VG_PAINT_LINEAR_GRADIENT:
      SH_RETURN_ERR_IF(count != 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      for (i=0; i<4; ++i)
        ((SHPaint*)object)->linearGradient[i] = shParamToFloat(values, floats, i);
      break;
      
    case VG_PAINT_RADIAL_GRADIENT:
      SH_RETURN_ERR_IF(count != 5, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      for (i=0; i<5; ++i)
        ((SHPaint*)object)->radialGradient[i] = shParamToFloat(values, floats, i);
      break;
      
    case VG_PAINT_PATTERN_TILING_MODE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      SH_RETURN_ERR_IF(!shIsEnumValid(ptype,ivalue), VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      ((SHPaint*)object)->tilingMode = (VGTilingMode)ivalue;
      break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
    
    } break;
  case SH_RESOURCE_IMAGE: switch (ptype) {/* Image parameters */
      
    case VG_IMAGE_FORMAT:
    case VG_IMAGE_WIDTH:
    case VG_IMAGE_HEIGHT:
      /* Read-only */ break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      
    } break;
    
  default:
    /* Invalid resource handle */
    SH_ASSERT(rtype!=SH_RESOURCE_INVALID);
    break;
  }
  
  SH_RETURN(SH_NO_RETVAL);
}

/*------------------------------------------------------------
 * Sets a resource parameter which takes a single float value
 *------------------------------------------------------------*/

VG_API_CALL void vgSetParameterf(VGHandle object, VGint paramType, VGfloat value)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* Check if param vector */
  VG_RETURN_ERR_IF(shIsParamVector(paramType),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   VG_NO_RETVAL);
  
  /* Error code will be set by shSetParam() */
  shSetParameter(context, object, resType, paramType, 1, &value, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*--------------------------------------------------------------
 * Sets a resource parameter which takes a single integer value
 *--------------------------------------------------------------*/

VG_API_CALL void vgSetParameteri(VGHandle object, VGint paramType, VGint value)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* Check if param vector */
  VG_RETURN_ERR_IF(shIsParamVector(paramType),
                   VG_ILLEGAL_ARGUMENT_ERROR,
                   VG_NO_RETVAL);
  
  /* Error code will be set by shSetParam() */
  shSetParameter(context, object, resType, paramType, 1, &value, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------------
 * Sets a resource parameter which takes a vector of float values
 *----------------------------------------------------------------*/

VG_API_CALL void vgSetParameterfv(VGHandle object, VGint paramType,
                                  VGint count, const VGfloat * values)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: Check for input array alignment */
  
  /* Error code will be set by shSetParam() */
  shSetParameter(context, object, resType, paramType, count, values, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------------
 * Sets a resource parameter which takes a vector of integer values
 *------------------------------------------------------------------*/

VG_API_CALL void vgSetParameteriv(VGHandle object, VGint paramType,
                                  VGint count, const VGint * values)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: Check for input array alignment */
  
  /* Error code will be set by shSetParam() */
  shSetParameter(context, object, resType, paramType, count, values, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------------
 * Outputs a resource parameter by interpreting the output value
 * vector according to the parameter type and input type.
 *---------------------------------------------------------------*/

static void shGetParameter(VGContext *context, VGHandle object,
                           SHResourceType rtype, VGint ptype,
                           SHint count, void *values, SHint floats)
{
  int i;
  
  /* Check for invalid array / count */
  SH_RETURN_ERR_IF(!values || count<=0, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
  
  switch (rtype)
  {
  case SH_RESOURCE_PATH: switch (ptype) { /* Path parameters */
      
    case VG_PATH_FORMAT:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPath*)object)->format, count, values, floats, 0);
      break;
      
    case VG_PATH_DATATYPE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPath*)object)->datatype, count, values, floats, 0);
      break;
      
    case VG_PATH_SCALE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shFloatToParam(((SHPath*)object)->scale, count, values, floats, 0);
      break;
      
    case VG_PATH_BIAS:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shFloatToParam(((SHPath*)object)->bias, count, values, floats, 0);
      break;
      
    case VG_PATH_NUM_SEGMENTS:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPath*)object)->segCount, count, values, floats, 0);
      break;
      
    case VG_PATH_NUM_COORDS:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPath*)object)->dataCount, count, values, floats, 0);
      break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      
    } break;
  case SH_RESOURCE_PAINT: switch (ptype) { /* Paint parameters */
      
    case VG_PAINT_TYPE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPaint*)object)->type, count, values, floats, 0);
      break;
      
    case VG_PAINT_COLOR:
      SH_RETURN_ERR_IF(count > 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shFloatToParam(((SHPaint*)object)->color.r, count, values, floats, 0);
      shFloatToParam(((SHPaint*)object)->color.g, count, values, floats, 1);
      shFloatToParam(((SHPaint*)object)->color.b, count, values, floats, 2);
      shFloatToParam(((SHPaint*)object)->color.a, count, values, floats, 3);
      break;
      
    case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPaint*)object)->spreadMode, count, values, floats, 0);
      break;
      
    case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPaint*)object)->spreadMode, count, values, floats, 0);
      break;
      
    case VG_PAINT_COLOR_RAMP_STOPS:{
        
        int i; SHPaint* paint = (SHPaint*)object; SHStop *stop;
        SH_RETURN_ERR_IF(count > paint->stops.size * 5,
                         VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
        
        for (i=0; i<paint->stops.size; ++i) {
          stop = &paint->stops.items[i];
          shFloatToParam(stop->offset, count, values, floats, i*5+0);
          shFloatToParam(stop->color.r, count, values, floats, i*5+1);
          shFloatToParam(stop->color.g, count, values, floats, i*5+2);
          shFloatToParam(stop->color.b, count, values, floats, i*5+3);
          shFloatToParam(stop->color.a, count, values, floats, i*5+4);
        }
        
        break;}
      
    case VG_PAINT_LINEAR_GRADIENT:
      SH_RETURN_ERR_IF(count > 4, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      for (i=0; i<4; ++i)
        shFloatToParam(((SHPaint*)object)->linearGradient[i],
                        count, values, floats, i);
      break;
      
    case VG_PAINT_RADIAL_GRADIENT:
      SH_RETURN_ERR_IF(count > 5, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      for (i=0; i<5; ++i)
        shFloatToParam(((SHPaint*)object)->radialGradient[i],
                       count, values, floats, i);
      break;
      
    case VG_PAINT_PATTERN_TILING_MODE:
      SH_RETURN_ERR_IF(count != 1, VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      shIntToParam(((SHPaint*)object)->tilingMode, count, values, floats, 0);
      break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      
    } break;
  case SH_RESOURCE_IMAGE: switch (ptype) { /* Image parameters */
      
      /* TODO: output image parameters when image implemented */
    case VG_IMAGE_FORMAT:
      break;
      
    case VG_IMAGE_WIDTH:
      break;
      
    case VG_IMAGE_HEIGHT:
      break;
      
    default:
      /* Invalid VGParamType */
      SH_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, SH_NO_RETVAL);
      
    } break;
    
  default:
    /* Invalid resource handle */
    SH_ASSERT(rtype!=SH_RESOURCE_INVALID);
    break;
  }
  
  SH_RETURN(SH_NO_RETVAL);
}

/*----------------------------------------------------------
 * Returns a resource parameter of a single float value
 *----------------------------------------------------------*/

VG_API_CALL VGfloat vgGetParameterf(VGHandle object, VGint paramType)
{
  VGfloat retval = 0.0f;
  SHResourceType resType;
  VG_GETCONTEXT(retval);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, retval);
  
  /* Check if param vector */
  VG_RETURN_ERR_IF(shIsParamVector(paramType),
                   VG_ILLEGAL_ARGUMENT_ERROR, retval);
  
  /* Error code will be set by shGetParameter() */
  shGetParameter(context, object, resType, paramType, 1, &retval, 1);
  VG_RETURN(retval);
}

/*----------------------------------------------------------
 * Returns a resource parameter of a single integer value
 *----------------------------------------------------------*/

VG_API_CALL VGint vgGetParameteri(VGHandle object, VGint paramType)
{
  VGint retval = 0;
  SHResourceType resType;
  VG_GETCONTEXT(retval);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, retval);
  
  /* Check if param vector */
  VG_RETURN_ERR_IF(shIsParamVector(paramType),
                   VG_ILLEGAL_ARGUMENT_ERROR, retval);
  
  /* Error code will be set by shGetParameter() */
  shGetParameter(context, object, resType, paramType, 1, &retval, 0);
  VG_RETURN(retval);
}

/*----------------------------------------------------------
 * Outputs a resource parameter of a float vector value
 *----------------------------------------------------------*/

VG_API_CALL void vgGetParameterfv(VGHandle object, VGint paramType,
                                  VGint count, VGfloat * values)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: Check output array alignment */
  
  /* Error code will be set by shGetParameter() */
  shGetParameter(context, object, resType, paramType, count, values, 1);
  VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------
 * Outputs a resource parameter of an integer vector value
 *----------------------------------------------------------*/

VG_API_CALL void vgGetParameteriv(VGHandle object, VGint paramType,
                                  VGint count, VGint * values)
{
  SHResourceType resType;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Validate object */
  resType = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(resType == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* TODO: Check output array alignment */
  
  /* Error code will be set by shGetParameter() */
  shGetParameter(context, object, resType, paramType, count, values, 0);
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Returns the size of the output array required to
 * receive the whole vector of resource parameter values
 *---------------------------------------------------------*/
  
VG_API_CALL VGint vgGetParameterVectorSize(VGHandle object, VGint ptype)
{
  int retval = 0;
  SHResourceType rtype;
  VG_GETCONTEXT(retval);
  
  /* Validate object */
  rtype = shGetResourceType(context, object);
  VG_RETURN_ERR_IF(rtype == SH_RESOURCE_INVALID,
                   VG_BAD_HANDLE_ERROR, retval);
  
  switch (rtype)
  {
  case SH_RESOURCE_PATH: switch (ptype) { /* Path parameters */
      
    case VG_PATH_FORMAT:
    case VG_PATH_DATATYPE:
    case VG_PATH_SCALE:
    case VG_PATH_BIAS:
    case VG_PATH_NUM_SEGMENTS:
    case VG_PATH_NUM_COORDS:
      retval = 1; break;
      
    default:
      /* Invalid VGParamType */
      VG_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, retval);
      
    } break;
  case SH_RESOURCE_PAINT: switch (ptype) { /* Paint parameters */
      
    case VG_PAINT_TYPE:
      retval = 1; break;
      
    case VG_PAINT_COLOR:
      retval = 4; break;
      
    case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
      retval = 1; break;
      
    case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
      retval = 1; break;
      
    case VG_PAINT_COLOR_RAMP_STOPS:
      retval = ((SHPaint*)object)->stops.size*5; break;
      
    case VG_PAINT_LINEAR_GRADIENT:
      retval = 4; break;
      
    case VG_PAINT_RADIAL_GRADIENT:
      retval = 5; break;
      
    case VG_PAINT_PATTERN_TILING_MODE:
      retval = 1; break;
      
    default:
      /* Invalid VGParamType */
      VG_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, retval);
      
    } break;
  case SH_RESOURCE_IMAGE: switch (ptype) { /* Image parameters */
      
    case VG_IMAGE_FORMAT:
    case VG_IMAGE_WIDTH:
    case VG_IMAGE_HEIGHT:
      retval = 1; break;
      
    default:
      /* Invalid VGParamType */
      VG_RETURN_ERR(VG_ILLEGAL_ARGUMENT_ERROR, retval);
      
    } break;
    
  default:
    /* Invalid resource handle */
    SH_ASSERT(rtype!=SH_RESOURCE_INVALID);
    break;
  }
  
  VG_RETURN(retval);
}

VG_API_CALL const VGubyte * vgGetString(VGStringID name)
{
  VG_GETCONTEXT(NULL);
  
  switch(name) {
  case VG_VENDOR:
    VG_RETURN((const VGubyte*)context->vendor);
  case VG_RENDERER:
    VG_RETURN((const VGubyte*)context->renderer);
  case VG_VERSION:
    VG_RETURN((const VGubyte*)context->version);
  case VG_EXTENSIONS:
    VG_RETURN((const VGubyte*)context->extensions);
  default:
    VG_RETURN(NULL);
  }
}
