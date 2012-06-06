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

#ifndef __SHPATH_H
#define __SHPATH_H

#include "shVectors.h"
#include "shArrays.h"

/* Helper structures for subdivision */
typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHVector2 p3;
} SHQuad;

typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHVector2 p3;
  SHVector2 p4;
} SHCubic;

typedef struct {
  SHVector2 p1;
  SHVector2 p2;
  SHfloat a1;
  SHfloat a2;
} SHArc;

/* SHVertex */
typedef struct
{
  SHVector2 point;
  SHVector2 tangent;
  SHfloat length;
  SHuint flags;
  
} SHVertex;

/* Vertex flags for contour definition */
#define SH_VERTEX_FLAG_CLOSE   (1 << 0)
#define SH_VERTEX_FLAG_SEGEND  (1 << 1)
#define SH_SEGMENT_TYPE_COUNT  13

/* Vertex array */
#define _ITEM_T SHVertex
#define _ARRAY_T SHVertexArray
#define _FUNC_T shVertexArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"


/* SHPath */
typedef struct SHPath
{
  /* Properties */
  VGint format;
  SHfloat scale;
  SHfloat bias;
  SHint segHint;
  SHint dataHint;
  VGbitfield caps;
  VGPathDatatype datatype;
  
  /* Raw data */
  SHuint8 *segs;
  void *data;
  SHint segCount;
  SHint dataCount;

  /* Subdivision */
  SHVertexArray vertices;
  SHVector2 min, max;
  
  /* Additional stroke geometry (dash vertices if
     path dashed or triangle vertices if width > 1 */
  SHVector2Array stroke;

  /* Cache */
  VGboolean      cacheDataValid;

  VGboolean      cacheTransformInit;
  SHMatrix3x3    cacheTransform;

  VGboolean      cacheStrokeInit;
  VGboolean      cacheStrokeTessValid;
  SHfloat        cacheStrokeLineWidth;
  VGCapStyle     cacheStrokeCapStyle;
  VGJoinStyle    cacheStrokeJoinStyle;
  SHfloat        cacheStrokeMiterLimit;
  SHfloat        cacheStrokeDashPhase;
  VGboolean      cacheStrokeDashPhaseReset;
  
} SHPath;

void SHPath_ctor(SHPath *p);
void SHPath_dtor(SHPath *p);


/* Processing normalization flags */
#define SH_PROCESS_SIMPLIFY_LINES    (1 << 0)
#define SH_PROCESS_SIMPLIFY_CURVES   (1 << 1)
#define SH_PROCESS_CENTRALIZE_ARCS   (1 << 2)
#define SH_PROCESS_REPAIR_ENDS       (1 << 3)

/* Segment callback function type */
typedef void (*SegmentFunc) (SHPath *p, VGPathSegment segment,
                             VGPathCommand originalCommand,
                             SHfloat *data, void *userData);

/* Processes raw path data into normalized segments */
void shProcessPathData(SHPath *p, int flags,
                       SegmentFunc callback,
                       void *userData);


/* Pointer-to-path array */
#define _ITEM_T SHPath*
#define _ARRAY_T SHPathArray
#define _FUNC_T shPathArray
#define _ARRAY_DECLARE
#include "shArrayBase.h"

#endif /* __SHPATH_H */
