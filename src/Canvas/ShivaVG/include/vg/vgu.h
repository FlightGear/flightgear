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
 */

#ifndef _VGU_H
#define _VGU_H

#include "openvg.h"

#define VGU_VERSION_1_0		1
#define VGU_VERSION_1_0_1	1

#ifndef VGU_API_CALL
#define VGU_API_CALL VG_API_CALL
#endif

typedef enum {
  VGU_NO_ERROR                                 = 0,
  VGU_BAD_HANDLE_ERROR                         = 0xF000,
  VGU_ILLEGAL_ARGUMENT_ERROR                   = 0xF001,
  VGU_OUT_OF_MEMORY_ERROR                      = 0xF002,
  VGU_PATH_CAPABILITY_ERROR                    = 0xF003,
  VGU_BAD_WARP_ERROR                           = 0xF004
} VGUErrorCode;

typedef enum {
  VGU_ARC_OPEN                                 = 0xF100,
  VGU_ARC_CHORD                                = 0xF101,
  VGU_ARC_PIE                                  = 0xF102
} VGUArcType;

#ifdef __cplusplus 
extern "C" { 
#endif

VGU_API_CALL VGUErrorCode vguLine(VGPath path,
				  VGfloat x0, VGfloat y0,
				  VGfloat x1, VGfloat y1);

VGU_API_CALL VGUErrorCode vguPolygon(VGPath path,
				    const VGfloat * points, VGint count,
				    VGboolean closed);

VGU_API_CALL VGUErrorCode vguRect(VGPath path,
				 VGfloat x, VGfloat y,
				 VGfloat width, VGfloat height);

VGU_API_CALL VGUErrorCode vguRoundRect(VGPath path,
				      VGfloat x, VGfloat y,
				      VGfloat width, VGfloat height,
				      VGfloat arcWidth, VGfloat arcHeight);
  
VGU_API_CALL VGUErrorCode vguEllipse(VGPath path,
				    VGfloat cx, VGfloat cy,
				    VGfloat width, VGfloat height);

VGU_API_CALL VGUErrorCode vguArc(VGPath path,
				VGfloat x, VGfloat y,
				VGfloat width, VGfloat height,
				VGfloat startAngle, VGfloat angleExtent,
				VGUArcType arcType);

VGU_API_CALL VGUErrorCode vguComputeWarpQuadToSquare(VGfloat sx0, VGfloat sy0,
                                                    VGfloat sx1, VGfloat sy1,
                                                    VGfloat sx2, VGfloat sy2,
                                                    VGfloat sx3, VGfloat sy3,
                                                    VGfloat * matrix);

VGU_API_CALL VGUErrorCode vguComputeWarpSquareToQuad(VGfloat dx0, VGfloat dy0,
                                                    VGfloat dx1, VGfloat dy1,
                                                    VGfloat dx2, VGfloat dy2,
                                                    VGfloat dx3, VGfloat dy3,
                                                    VGfloat * matrix);

VGU_API_CALL VGUErrorCode vguComputeWarpQuadToQuad(VGfloat dx0, VGfloat dy0,
                                                  VGfloat dx1, VGfloat dy1,
                                                  VGfloat dx2, VGfloat dy2,
                                                  VGfloat dx3, VGfloat dy3,
												                          VGfloat sx0, VGfloat sy0,
                                                  VGfloat sx1, VGfloat sy1,
                                                  VGfloat sx2, VGfloat sy2,
                                                  VGfloat sx3, VGfloat sy3,
                                                  VGfloat * matrix);

#ifdef __cplusplus 
} /* extern "C" */
#endif

#endif /* #ifndef _VGU_H */
