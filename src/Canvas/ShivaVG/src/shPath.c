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

#include <vg/openvg.h>
#include "shContext.h"
#include "shPath.h"
#include <string.h>
#include <stdio.h>

#define _ITEM_T SHVertex
#define _ARRAY_T SHVertexArray
#define _FUNC_T shVertexArray
#define _COMPARE_T(v1,v2) 0
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T SHPath*
#define _ARRAY_T SHPathArray
#define _FUNC_T shPathArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"


static const SHint shCoordsPerCommand[] = {
  0, /* VG_CLOSE_PATH */
  2, /* VG_MOTE_TO */
  2, /* VG_LINE_TO */
  1, /* VG_HLINE_TO */
  1, /* VG_VLINE_TO */
  4, /* VG_QUAD_TO */
  6, /* VG_CUBIC_TO */
  2, /* VG_SQUAD_TO */
  4, /* VG_SCUBIC_TO */
  5, /* VG_SCCWARC_TO */
  5, /* VG_SCWARC_TO */
  5, /* VG_LCCWARC_TO */
  5  /* VG_LCWARC_TO */
};

#define SH_PATH_MAX_COORDS 6
#define SH_PATH_MAX_COORDS_PROCESSED 12

static const SHint shBytesPerDatatype[] = {
  1, /* VG_PATH_DATATYPE_S_8 */
  2, /* VG_PATH_DATATYPE_S_16 */
  4, /* VG_PATH_DATATYPE_S_32 */
  4  /* VG_PATH_DATATYPE_F */
};

#define SH_PATH_MAX_BYTES  4

/*-----------------------------------------------------
 * Path constructor
 *-----------------------------------------------------*/

void shClearSegCallbacks(SHPath *p);
void SHPath_ctor(SHPath *p)
{
  p->format = 0;
  p->scale = 0.0f;
  p->bias = 0.0f;
  p->caps = 0;
  p->datatype = VG_PATH_DATATYPE_F;
  
  p->segs = NULL;
  p->data = NULL;
  p->segCount = 0;
  p->dataCount = 0;
  
  SH_INITOBJ(SHVertexArray, p->vertices);
  SH_INITOBJ(SHVector2Array, p->stroke);
}

/*-----------------------------------------------------
 * Path destructor
 *-----------------------------------------------------*/

void SHPath_dtor(SHPath *p)
{
  if (p->segs) free(p->segs);
  if (p->data) free(p->data);
  
  SH_DEINITOBJ(SHVertexArray, p->vertices);
  SH_DEINITOBJ(SHVector2Array, p->stroke);
}

/*-----------------------------------------------------
 * Returns true (1) if given path data type is valid
 *-----------------------------------------------------*/

static SHint shIsValidDatatype(VGPathDatatype t)
{
  return (t == VG_PATH_DATATYPE_S_8 ||
          t == VG_PATH_DATATYPE_S_16 ||
          t == VG_PATH_DATATYPE_S_32 ||
          t == VG_PATH_DATATYPE_F);
}

static SHint shIsValidCommand(SHint c)
{
  return (c >= (VG_CLOSE_PATH >> 1) &&
          c <= (VG_LCWARC_TO >> 1));
}

static SHint shIsArcSegment(SHint s)
{
  return (s == VG_SCWARC_TO || s == VG_SCCWARC_TO ||
          s == VG_LCWARC_TO || s == VG_LCCWARC_TO);
}

/*-------------------------------------------------------
 * Allocates a path resource in the current context and
 * sets its capabilities.
 *-------------------------------------------------------*/

VG_API_CALL VGPath vgCreatePath(VGint pathFormat,
                                VGPathDatatype datatype,
                                VGfloat scale, VGfloat bias,
                                VGint segmentCapacityHint,
                                VGint coordCapacityHint,
                                VGbitfield capabilities)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_INVALID_HANDLE);
  
  /* Only standard format supported */
  VG_RETURN_ERR_IF(pathFormat != VG_PATH_FORMAT_STANDARD,
                   VG_UNSUPPORTED_PATH_FORMAT_ERROR,
                   VG_INVALID_HANDLE);
  
  /* Dataype must be valid, scale not zero */
  VG_RETURN_ERR_IF(!shIsValidDatatype(datatype) || scale == 0.0f,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
  
  /* Allocate new resource */
  SH_NEWOBJ(SHPath, p);
  VG_RETURN_ERR_IF(!p, VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);
  shPathArrayPushBack(&context->paths, p);
  
  /* Set parameters */
  p->format = pathFormat;
  p->scale = scale;
  p->bias = bias;
  p->segHint = segmentCapacityHint;
  p->dataHint = coordCapacityHint;
  p->datatype = datatype;
  p->caps = capabilities & VG_PATH_CAPABILITY_ALL;

  /* Init cache flags */
  p->cacheDataValid = VG_TRUE;
  p->cacheTransformInit = VG_FALSE;
  p->cacheStrokeInit = VG_FALSE;
  
  VG_RETURN((VGPath)p);
}

/*-----------------------------------------------------
 * Clears the specified path of all data and sets new
 * capabilities to it.
 *-----------------------------------------------------*/

VG_API_CALL void vgClearPath(VGPath path, VGbitfield capabilities)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* Clear raw data */
  p = (SHPath*)path;
  free(p->segs);
  free(p->data);
  p->segs = NULL;
  p->data = NULL;
  p->segCount = 0;
  p->dataCount = 0;

  /* Mark change */
  p->cacheDataValid = VG_FALSE;
  
  /* Downsize arrays to save memory */
  shVertexArrayRealloc(&p->vertices, 1);
  shVector2ArrayRealloc(&p->stroke, 1);
  
  /* Re-set capabilities */
  p->caps = capabilities & VG_PATH_CAPABILITY_ALL;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Disposes specified path resource in the current context
 *---------------------------------------------------------*/

VG_API_CALL void vgDestroyPath(VGPath path)
{
  int index;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  /* Check if handle valid */
  index = shPathArrayFind(&context->paths, (SHPath*)path);
  VG_RETURN_ERR_IF(index == -1, VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  /* Delete object and remove resource */
  SH_DELETEOBJ(SHPath, (SHPath*)path);
  shPathArrayRemoveAt(&context->paths, index);
  
  VG_RETURN_ERR(VG_NO_ERROR, VG_NO_RETVAL);
}

/*-----------------------------------------------------
 * Removes capabilities defined in the given bitfield
 * from the specified path.
 *-----------------------------------------------------*/

VG_API_CALL void vgRemovePathCapabilities(VGPath path, VGbitfield capabilities)
{
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  capabilities &= VG_PATH_CAPABILITY_ALL;
  ((SHPath*)path)->caps &= ~capabilities;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------
 * Returns capabilities of a path resource
 *-----------------------------------------------------*/

VG_API_CALL VGbitfield vgGetPathCapabilities(VGPath path)
{
  VG_GETCONTEXT(0x0);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, 0x0);
  
  VG_RETURN( ((SHPath*)path)->caps );
}

/*-----------------------------------------------------
 * Walks the given path segments and returns the total
 * number of coordinates.
 *-----------------------------------------------------*/

static SHint shCoordCountForData(VGint segcount, const SHuint8 *segs)
{
  int s;
  int command;
  int count = 0;
  
  for (s=0; s<segcount; ++s) {
    command = ((segs[s] & 0x1E) >> 1);
    if (!shIsValidCommand(command)) return -1;
    count += shCoordsPerCommand[command];
  }
  
  return count;
}

/*-------------------------------------------------------
 * Interpretes the path data array according to the
 * path data type and returns the value at given index
 * in final interpretation (including scale and bias)
 *-------------------------------------------------------*/

static SHfloat shRealCoordFromData(VGPathDatatype type, SHfloat scale, SHfloat bias,
                                   void *data, SHint index)
{
  switch (type) {
  case VG_PATH_DATATYPE_S_8:
    return ( (SHfloat) ((SHint8*)data)[index] ) * scale + bias;
  case VG_PATH_DATATYPE_S_16:
    return ( (SHfloat) ((SHint16*)data)[index] ) * scale + bias;
  case VG_PATH_DATATYPE_S_32:
    return ( (SHfloat) ((SHint32*)data)[index] ) * scale + bias;;
  default:
    return ( (SHfloat) ((SHfloat32*)data)[index] ) * scale + bias;
  }
}

/*-------------------------------------------------------
 * Interpretes the path data array according to the
 * path data type and sets the value at given index
 * from final interpretation (including scale and bias)
 *-------------------------------------------------------*/

static void shRealCoordToData(VGPathDatatype type, SHfloat scale, SHfloat bias,
                              void *data,  SHint index, SHfloat c)
{
  c -= bias;
  c /= scale;
  
  switch (type) {
  case VG_PATH_DATATYPE_S_8:
    ((SHint8*)data) [index] = (SHint8)SH_FLOOR(c + 0.5f); break;
  case VG_PATH_DATATYPE_S_16:
    ((SHint16*)data) [index] = (SHint16)SH_FLOOR(c + 0.5f); break;
  case VG_PATH_DATATYPE_S_32:
    ((SHint32*)data) [index] = (SHint32)SH_FLOOR(c + 0.5f); break;
  default:
    ((SHfloat32*)data) [index] = (SHfloat32)c; break;
  }
}

/*-------------------------------------------------
 * Resizes storage for segment and coordinate data
 * of the specified path by given number of items
 *-------------------------------------------------*/

static int shResizePathData(SHPath *p, SHint newSegCount, SHint newDataCount,
                            SHuint8 **newSegs, SHuint8 **newData)
{
  SHint oldDataSize = 0;
  SHint newDataSize = 0;
  
  /* Allocate memory for new segments */
  (*newSegs) = (SHuint8*)malloc(p->segCount + newSegCount);
  if ((*newSegs) == NULL) return 0;
  
  /* Allocate memory for new data */
  oldDataSize = p->dataCount * shBytesPerDatatype[p->datatype];
  newDataSize = newDataCount * shBytesPerDatatype[p->datatype];
  (*newData) = (SHuint8*)malloc(oldDataSize + newDataSize);
  if ((*newData) == NULL) {free(*newSegs); return 0;}
  
  /* Copy old segments */
  memcpy(*newSegs, p->segs, p->segCount);
  
  /* Copy old data */
  memcpy(*newData, p->data, oldDataSize);
  
  return 1;
}

/*-------------------------------------------------------------
 * Appends path data from source to destination path resource
 *-------------------------------------------------------------*/

VG_API_CALL void vgAppendPath(VGPath dstPath, VGPath srcPath)
{
  int i;
  SHPath *src, *dst;
  SHuint8 *newSegs = NULL;
  SHuint8 *newData = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, srcPath) ||
                   !shIsValidPath(context, dstPath),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  src = (SHPath*)srcPath; dst = (SHPath*)dstPath;
  VG_RETURN_ERR_IF(!(src->caps & VG_PATH_CAPABILITY_APPEND_FROM) ||
                   !(dst->caps & VG_PATH_CAPABILITY_APPEND_TO),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);
  
  /* Resize path storage */
  shResizePathData(dst, src->segCount, src->dataCount, &newSegs, &newData);
  VG_RETURN_ERR_IF(!newData, VG_OUT_OF_MEMORY_ERROR, VG_NO_RETVAL);
  
  /* Copy new segments */
  memcpy(newSegs+dst->segCount, src->segs, src->segCount);
  
  /* Copy new coordinates */
  for (i=0; i<src->dataCount; ++i) {
    
    SHfloat coord = shRealCoordFromData(src->datatype, src->scale,
                                        src->bias, src->data, i);
    
    shRealCoordToData(dst->datatype, dst->scale, dst->bias,
                      newData, dst->dataCount+i, coord);
  }
  
  /* Free old arrays */
  free(dst->segs);
  free(dst->data);
  
  /* Adjust new properties */
  dst->segs = newSegs;
  dst->data = newData;
  dst->segCount += src->segCount;
  dst->dataCount += src->dataCount;

  /* Mark change */
  dst->cacheDataValid = VG_FALSE;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------
 * Appends data to destination path resource
 *-----------------------------------------------------*/

SHfloat shValidInputFloat(VGfloat f);
VG_API_CALL void vgAppendPathData(VGPath dstPath, VGint newSegCount,
                                  const VGubyte *segs, const void *data)
{
  int i;
  SHPath *dst = NULL;
  SHint newDataCount = 0;
  SHint oldDataSize = 0;
  SHint newDataSize = 0;
  SHuint8 *newSegs = NULL;
  SHuint8 *newData = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, dstPath),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  dst = (SHPath*)dstPath;
  VG_RETURN_ERR_IF(!(dst->caps & VG_PATH_CAPABILITY_APPEND_TO),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!segs || !data || newSegCount <= 0,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* TODO: check segment + data array alignment */
  
  /* Count number of coordinatess in appended data */
  newDataCount = shCoordCountForData(newSegCount, segs);
  newDataSize = newDataCount * shBytesPerDatatype[dst->datatype];
  oldDataSize = dst->dataCount * shBytesPerDatatype[dst->datatype];
  VG_RETURN_ERR_IF(newDataCount == -1, VG_ILLEGAL_ARGUMENT_ERROR,
                   VG_NO_RETVAL);
  
  /* Resize path storage */
  shResizePathData(dst, newSegCount, newDataCount, &newSegs, &newData);
  VG_RETURN_ERR_IF(!newData, VG_OUT_OF_MEMORY_ERROR, VG_NO_RETVAL);
  
  /* Copy new segments */
  memcpy(newSegs+dst->segCount, segs, newSegCount);
  
  /* Copy new coordinates */
  if (dst->datatype == VG_PATH_DATATYPE_F) {
    for (i=0; i<newDataCount; ++i)
      ((SHfloat32*)newData) [dst->dataCount+i] =
        shValidInputFloat( ((VGfloat*)data) [i] );
  }else{
    memcpy(newData+oldDataSize, data, newDataSize);
  }
  
  /* Free old arrays */
  free(dst->segs);
  free(dst->data);
  
  /* Adjust new properties */
  dst->segs = newSegs;
  dst->data = newData;
  dst->segCount += newSegCount;
  dst->dataCount += newDataCount;

  /* Mark change */
  dst->cacheDataValid = VG_FALSE;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*--------------------------------------------------------
 * Modifies the coordinates of the existing path segments
 *--------------------------------------------------------*/

VG_API_CALL void vgModifyPathCoords(VGPath dstPath, VGint startIndex,
                                    VGint numSegments,  const void * data)
{
  int i;
  SHPath *p;
  SHint newDataCount;
  SHint newDataSize;
  SHint dataStartCount;
  SHint dataStartSize;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, dstPath),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  p = (SHPath*)dstPath;
  VG_RETURN_ERR_IF(!(p->caps & VG_PATH_CAPABILITY_MODIFY),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!data || startIndex<0 || numSegments <=0 ||
                   startIndex + numSegments > p->segCount,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
  
  /* TODO: check data array alignment */
  
  /* Find start of the coordinates to be changed */
  dataStartCount = shCoordCountForData(startIndex, p->segs);
  dataStartSize = dataStartCount * shBytesPerDatatype[p->datatype];
  
  /* Find byte size of coordinates to be changed */
  newDataCount = shCoordCountForData(numSegments, &p->segs[startIndex]);
  newDataSize = newDataCount * shBytesPerDatatype[p->datatype];
  
  /* Copy new coordinates */
  if (p->datatype == VG_PATH_DATATYPE_F) {
    for (i=0; i<newDataCount; ++i)
      ((SHfloat32*)p->data) [dataStartCount+i] =
        shValidInputFloat( ((VGfloat*)data) [i] );
  }else{
    memcpy( ((SHuint8*)p->data) + dataStartSize, data, newDataSize);
  }

  /* Mark change */
  p->cacheDataValid = VG_FALSE;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------
 * Converts standart endpoint arc parametrization into center
 * arc parametrization for further internal processing
 *------------------------------------------------------------*/

SHint shCentralizeArc(SHuint command, SHfloat *data)
{
  SHfloat rx,ry,r,a;
  SHVector2 p1, p2, pp1,pp2;
  SHVector2 d, dt;
  SHfloat dist, halfdist, q;
  SHVector2 c1, c2, *c;
  SHVector2 pc1, pc2;
  SHfloat a1, ad, a2;
  SHVector2 ux, uy;
  SHMatrix3x3 user2arc;
  SHMatrix3x3 arc2user;
  
  rx = data[2];
  ry = data[3];
  a = SH_DEG2RAD(data[4]);
  SET2(p1, data[0], data[1]);
  SET2(p2, data[5], data[6]);
  
  /* Check for invalid radius and return line */
  if (SH_NEARZERO(rx) || SH_NEARZERO(ry)) {
    data[2] = p2.x; data[3] = p2.y;
    return 0;
  }
  
  /* Rotate to arc coordinates.
     Scale to correct eccentricity. */
  IDMAT(user2arc);
  ROTATEMATL(user2arc, -a);
  SCALEMATL(user2arc, 1, rx/ry);
  TRANSFORM2TO(p1, user2arc, pp1);
  TRANSFORM2TO(p2, user2arc, pp2);
  
  /* Distance between points and
     perpendicular vector */
  SET2V(d, pp2); SUB2V(d, pp1);
  dist = NORM2(d);
  halfdist = dist * 0.5f;
  SET2(dt, -d.y, d.x);
  DIV2(dt, dist);
  
  /* Check if too close and return line */
  if (halfdist == 0.0f) {
    data[2] = p2.x; data[3] = p2.y;
    return 0;
  }
  
  /* Scale radius if too far away */
  r = rx;
  if (halfdist > rx)
    r = halfdist;
  
  /* Intersections of circles centered to start and
     end point. (i.e. centers of two possible arcs) */
  q = SH_SIN(SH_ACOS(halfdist/r)) * r;
  if (SH_ISNAN(q)) q = 0.0f;
  c1.x = pp1.x + d.x/2 + dt.x * q;
  c1.y = pp1.y + d.y/2 + dt.y * q;
  c2.x = pp1.x + d.x/2 - dt.x * q;
  c2.y = pp1.y + d.y/2 - dt.y * q;
  
  /* Pick the right arc center */
  switch (command & 0x1E) {
  case VG_SCWARC_TO: case VG_LCCWARC_TO:
    c = &c2; break;
  case VG_LCWARC_TO: case VG_SCCWARC_TO:
    c = &c1; break;
  default:
    c = &c1;
  }
  
  /* Find angles of p1,p2 towards the chosen center */
  SET2V(pc1, pp1); SUB2V(pc1, (*c));
  SET2V(pc2, pp2); SUB2V(pc2, (*c));
  a1 = shVectorOrientation(&pc1);
  
  /* Correct precision error when ry very small
     (q gets very large and points very far appart) */
  if (SH_ISNAN(a1)) a1 = 0.0f;
  
  /* Small or large one? */
  switch (command & 0x1E) {
  case VG_SCWARC_TO: case VG_SCCWARC_TO:
    ad = ANGLE2(pc1,pc2); break;
  case VG_LCWARC_TO: case VG_LCCWARC_TO:
    ad = 2*PI - ANGLE2(pc1,pc2); break;
  default:
    ad = 0.0f;
  }
  
  /* Correct precision error when solution is single
     (pc1 and pc2 are nearly collinear but not really) */
  if (SH_ISNAN(ad)) ad = PI;
  
  /* CW or CCW? */
  switch (command & 0x1E) {
  case VG_SCWARC_TO: case VG_LCWARC_TO:
    a2 = a1 - ad; break;
  case VG_SCCWARC_TO: case VG_LCCWARC_TO:
    a2 = a1 + ad; break;
  default:
    a2 = a1;
  }
  
  /* Arc unit vectors */
  SET2(ux, r, 0);
  SET2(uy, 0, r);
  
  /* Transform back to user coordinates */
  IDMAT(arc2user);
  SCALEMATL(arc2user, 1, ry/rx);
  ROTATEMATL(arc2user, a);
  TRANSFORM2((*c), arc2user);
  TRANSFORM2(ux, arc2user);
  TRANSFORM2(uy, arc2user);
  
  /* Write out arc coords */
  data[2]  = c->x;  data[3]  = c->y;
  data[4]  = ux.x;  data[5]  = ux.y;
  data[6]  = uy.x;  data[7]  = uy.y;
  data[8]  = a1;    data[9]  = a2;
  data[10] = p2.x;  data[11] = p2.y;
  return 1;
}

/*-------------------------------------------------------
 * Walks the raw (standard) path data and simplifies the
 * complex and implicit segments according to given
 * simplificatin request flags. Instead of storing the
 * processed data into another array, the given callback
 * function is called, so we don't need to walk the
 * raw data twice just to find the neccessary memory
 * size for processed data.
 *-------------------------------------------------------*/

#define SH_PROCESS_SIMPLIFY_LINES    (1 << 0)
#define SH_PROCESS_SIMPLIFY_CURVES   (1 << 1)
#define SH_PROCESS_CENTRALIZE_ARCS   (1 << 2)
#define SH_PROCESS_REPAIR_ENDS       (1 << 3)

void shProcessPathData(SHPath *p,
                       int flags,
                       SegmentFunc callback,
                       void *userData)
{
  SHint i=0, s=0, d=0, c=0;
  SHuint command;
  SHuint segment;
  SHint segindex;
  VGPathAbsRel absrel;
  SHint numcoords;
  SHfloat data[SH_PATH_MAX_COORDS_PROCESSED];
  SHVector2 start; /* start of the current contour */
  SHVector2 pen; /* current pen position */
  SHVector2 tan; /* backward tangent for smoothing */
  SHint open = 0; /* contour-open flag */
  
  /* Reset points */
  SET2(start, 0,0);
  SET2(pen, 0,0);
  SET2(tan, 0,0);
  
  for (s=0; s<p->segCount; ++s, d+=numcoords) {
    
    /* Extract command */
    command = (p->segs[s]);
    absrel = (command & 1);
    segment = (command & 0x1E);
    segindex = (segment >> 1);
    numcoords = shCoordsPerCommand[segindex];
    
    /* Repair segment start / end */
    if (flags & SH_PROCESS_REPAIR_ENDS) {
      
      /* Prevent double CLOSE_PATH */
      if (!open && segment == VG_CLOSE_PATH)
        continue;
      
      /* Implicit MOVE_TO if segment starts without */
      if (!open && segment != VG_MOVE_TO) {
        data[0] = pen.x; data[1] = pen.y;
        data[2] = pen.x; data[3] = pen.y;
        (*callback)(p,VG_MOVE_TO,command,data,userData);
        open = 1;
      }
      
      /* Avoid a MOVE_TO at the end of data */
      if (segment == VG_MOVE_TO) {
        if (s == p->segCount-1) break;
        else {
          /* Avoid a lone MOVE_TO  */
          SHuint nextsegment = (p->segs[s+1] & 0x1E);
          if (nextsegment == VG_MOVE_TO)
            {open = 0; continue;}
        }}
    }
    
    /* Place pen into first two coords */
    data[0] = pen.x;
    data[1] = pen.y;
    c = 2;
    
    /* Unpack coordinates from path data */
    for (i=0; i<numcoords; ++i)
      data[c++] = shRealCoordFromData(p->datatype, p->scale, p->bias,
                                      p->data, d+i);
    
    /* Simplify complex segments */
    switch (segment)
    {
    case VG_CLOSE_PATH:
      
      data[2] = start.x;
      data[3] = start.y;
      
      SET2V(pen, start);
      SET2V(tan, start);
      open = 0;
      
      (*callback)(p,VG_CLOSE_PATH,command,data,userData);
      
      break;
    case VG_MOVE_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
      }
      
      SET2(pen, data[2], data[3]);
      SET2V(start, pen);
      SET2V(tan, pen);
      open = 1;
      
      (*callback)(p,VG_MOVE_TO,command,data,userData);
      
      break;
    case VG_LINE_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
      }
      
      SET2(pen, data[2], data[3]);
      SET2V(tan, pen);
      
      (*callback)(p,VG_LINE_TO,command,data,userData);
      
      break;
    case VG_HLINE_TO:
      
      if (absrel == VG_RELATIVE)
        data[2] += pen.x;
      
      SET2(pen, data[2], pen.y);
      SET2V(tan, pen);
      
      if (flags & SH_PROCESS_SIMPLIFY_LINES) {
        data[3] = pen.y;
        (*callback)(p,VG_LINE_TO,command,data,userData);
        break;
      }
      
      (*callback)(p,VG_HLINE_TO,command,data,userData);
      
      break;
    case VG_VLINE_TO:
      
      if (absrel == VG_RELATIVE)
        data[2] += pen.y;
      
      SET2(pen, pen.x, data[2]);
      SET2V(tan, pen);
      
      if (flags & SH_PROCESS_SIMPLIFY_LINES) {
        data[2] = pen.x; data[3] = pen.y;
        (*callback)(p,VG_LINE_TO,command,data,userData);
        break;
      }
       
      (*callback)(p,VG_VLINE_TO,command,data,userData);
      
      break;
    case VG_QUAD_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
        data[4] += pen.x; data[5] += pen.y;
      }
      
      SET2(tan, data[2], data[3]);
      SET2(pen, data[4], data[5]);
      
      (*callback)(p,VG_QUAD_TO,command,data,userData);
      
      break;
    case VG_CUBIC_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
        data[4] += pen.x; data[5] += pen.y;
        data[6] += pen.x; data[7] += pen.y;
      }
      
      SET2(tan, data[4], data[5]);
      SET2(pen, data[6], data[7]);
      
      (*callback)(p,VG_CUBIC_TO,command,data,userData);
      
      break;
    case VG_SQUAD_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
      }
      
      SET2(tan, 2*pen.x - tan.x, 2*pen.y - tan.y);
      SET2(pen, data[2], data[3]);
      
      if (flags & SH_PROCESS_SIMPLIFY_CURVES) {
        data[2] = tan.x; data[3] = tan.y;
        data[4] = pen.x; data[5] = pen.y;
        (*callback)(p,VG_QUAD_TO,command,data,userData);
        break;
      }
      
      (*callback)(p,VG_SQUAD_TO,command,data,userData);
      
      break;
    case VG_SCUBIC_TO:
      
      if (absrel == VG_RELATIVE) {
        data[2] += pen.x; data[3] += pen.y;
        data[4] += pen.x; data[5] += pen.y;
      }
      
      SET2(tan, data[2], data[3]);
      SET2(pen, data[4], data[5]);
      
      if (flags & SH_PROCESS_SIMPLIFY_CURVES) {
        data[2] = 2*pen.x - tan.x;
        data[3] = 2*pen.y - tan.y;
        data[4] = tan.x; data[5] = tan.y;
        data[6] = pen.x; data[7] = pen.y;
        (*callback)(p,VG_CUBIC_TO,command,data,userData);
        break;
      }
      
      (*callback)(p,VG_SCUBIC_TO,command,data,userData);
      
      break;
    case VG_SCWARC_TO: case VG_SCCWARC_TO:
    case VG_LCWARC_TO: case VG_LCCWARC_TO:
      
      if (absrel == VG_RELATIVE) {
        data[5] += pen.x; data[6] += pen.y;
      }
      
      data[2] = SH_ABS(data[2]);
      data[3] = SH_ABS(data[3]);
      
      SET2(tan, data[5], data[6]);
      SET2V(pen, tan);
      
      if (flags & SH_PROCESS_CENTRALIZE_ARCS) {
        if (shCentralizeArc(command, data))
          (*callback)(p,segment,command,data,userData);
        else
          (*callback)(p,VG_LINE_TO,command,data,userData);
        break;
      }
      
      (*callback)(p,segment,command,data,userData);
      break;
      
    } /* switch (command) */
  } /* for each segment */
}

/*-------------------------------------------------------
 * Walks raw path data and counts the resulting number
 * of segments and coordinates if the simplifications
 * specified in the given processing flags were applied.
 *-------------------------------------------------------*/

static void shProcessedDataCount(SHPath *p, SHint flags,
                                 SHint *segCount, SHint *dataCount)
{
  SHint s, segment, segindex;
  *segCount = 0; *dataCount = 0;
  
  for (s=0; s<p->segCount; ++s) {
    
    segment = (p->segs[s] & 0x1E);
    segindex = (segment >> 1);
    
    switch (segment) {
    case VG_HLINE_TO: case VG_VLINE_TO:
      
      if (flags & SH_PROCESS_SIMPLIFY_LINES)
        *dataCount += shCoordsPerCommand[VG_LINE_TO >> 1];
      else *dataCount += shCoordsPerCommand[segindex];
      break;
      
    case VG_SQUAD_TO:
      
      if (flags & SH_PROCESS_SIMPLIFY_CURVES)
        *dataCount += shCoordsPerCommand[VG_QUAD_TO >> 1];
      else *dataCount += shCoordsPerCommand[segindex];
      break;
      
    case VG_SCUBIC_TO:
      
      if (flags & SH_PROCESS_SIMPLIFY_CURVES)
        *dataCount += shCoordsPerCommand[VG_CUBIC_TO >> 1];
      else *dataCount += shCoordsPerCommand[segindex];
      break;
      
    case VG_SCWARC_TO: case VG_SCCWARC_TO:
    case VG_LCWARC_TO: case VG_LCCWARC_TO:
      
      if (flags & SH_PROCESS_CENTRALIZE_ARCS)
        *dataCount += 10;
      else *dataCount += shCoordsPerCommand[segindex];
      break;
      
    default:
      *dataCount += shCoordsPerCommand[segindex];
    }
    
    *segCount += 1;
  }
}

static void shTransformSegment(SHPath *p, VGPathSegment segment,
                               VGPathCommand originalCommand,
                               SHfloat *data, void *userData)
{
  int i, numPoints; SHVector2 point;
  SHuint8* newSegs   = (SHuint8*) ((void**)userData)[0];
  SHint*   segCount  = (SHint*)   ((void**)userData)[1];
  SHuint8* newData   = (SHuint8*) ((void**)userData)[2];
  SHint*   dataCount = (SHint*)   ((void**)userData)[3];
  SHPath*  dst       = (SHPath*)  ((void**)userData)[4];
  SHMatrix3x3 *ctm;
  
  /* Get current transform matrix */
  VG_GETCONTEXT(VG_NO_RETVAL);
  ctm = &context->pathTransform;
  
  switch (segment) {
  case VG_CLOSE_PATH:
      
    /* No cordinates for this segment */
    
    break;
  case VG_MOVE_TO:
  case VG_LINE_TO:
  case VG_QUAD_TO:
  case VG_CUBIC_TO:
  case VG_SQUAD_TO:
  case VG_SCUBIC_TO:
    
    /* Walk over control points */
    numPoints = shCoordsPerCommand[segment >> 1] / 2;
    for (i=0; i<numPoints; ++i) {
      
      /* Transform point by user to surface matrix */
      SET2(point, data[2 + i*2], data[2 + i*2 + 1]);
      TRANSFORM2(point, (*ctm));
      
      /* Write coordinates back to path data */
      shRealCoordToData(dst->datatype, dst->scale, dst->bias,
                        newData, (*dataCount)++, point.x);
      
      shRealCoordToData(dst->datatype, dst->scale, dst->bias,
                        newData, (*dataCount)++, point.y);
    }
    
    break;
  default:{
      
      SHMatrix3x3 u, u2;
      SHfloat a, cosa, sina, rx, ry;
      SHfloat A,B,C,AC,K,A2,C2;
      SHint invertible;
      SHVector2 p;
      SHfloat out[5];
      SHint i;
      
      SH_ASSERT(segment==VG_SCWARC_TO || segment==VG_SCCWARC_TO ||
                segment==VG_LCWARC_TO || segment==VG_LCCWARC_TO);
      
      rx = data[2]; ry = data[3];
      a = SH_DEG2RAD(data[4]);
      cosa = SH_COS(a); sina = SH_SIN(a);
      
      /* Center parametrization of the elliptical arc. */
      SETMAT(u, rx * cosa, -ry * sina, 0,
             rx * sina,  ry * cosa,    0,
             0,          0,            1);
      
      /* Add current transformation */
      MULMATMAT((*ctm), u, u2);
      
      /* Inverse (transforms ellipse back to unit circle) */
      invertible = shInvertMatrix(&u2, &u);
      if (!invertible) {
        
        /* Zero-out radii and rotation */
        out[0] = out[1] = out[2] = 0.0f;
        
      }else{
      
        /* Extract implicit ellipse equation */
        A =     u.m[0][0]*u.m[0][0] + u.m[1][0]*u.m[1][0];
        B = 2*( u.m[0][0]*u.m[0][1] + u.m[1][0]*u.m[1][1] );
        C =     u.m[0][1]*u.m[0][1] + u.m[1][1]*u.m[1][1];
        AC = A-C;
        
        /* Find rotation and new radii */
        if (B == 0.0f) {
          
          /* 0 or 90 degree */
          out[2] = 0.0f;
          A2 = A;
          C2 = C;
          
        }else if (AC == 0.0f) {
          
          /* 45 degree */
          out[2] = PI/4;
          out[2] = SH_RAD2DEG(out[2]);
          A2 = A + B * 0.5f;
          C2 = A - B * 0.5f;
          
        }else{
          
          /* Any other angle */
          out[2] = 0.5f * SH_ATAN(B / AC);
          out[2] = SH_RAD2DEG(out[2]);
          
          K = 1 + (B*B) / (AC*AC);
          if (K <= 0.0f) K = 0.0f;
          else K = SH_SQRT(K);
          
          A2 = 0.5f * (A + C + K*AC);
          C2 = 0.5f * (A + C - K*AC);
        }
        
        /* New radii */
        out[0] = (A2 <= 0.0f ? 0.0f : 1 / SH_SQRT(A2));
        out[1] = (C2 <= 0.0f ? 0.0f : 1 / SH_SQRT(C2));
        
        /* Check for a mirror component in the transform matrix */
        if (ctm->m[0][0] * ctm->m[1][1] - ctm->m[1][0] * ctm->m[0][1] < 0.0f) {
          switch (segment) {
          case VG_SCWARC_TO: segment = VG_SCCWARC_TO; break;
          case VG_LCWARC_TO: segment = VG_LCCWARC_TO; break;
          case VG_SCCWARC_TO: segment = VG_SCWARC_TO; break;
          case VG_LCCWARC_TO: segment = VG_LCWARC_TO; break;
          default: break; }
        }
        
      }/* if (invertible) */
      
      /* Transform target point */
      SET2(p, data[5], data[6]);
      TRANSFORM2(p, context->pathTransform);
      out[3] = p.x; out[4] = p.y;
      
      /* Write coordinates back to path data */
      for (i=0; i<5; ++i)
        shRealCoordToData(dst->datatype, dst->scale, dst->bias,
                          newData, (*dataCount)++, out[i]);
      
      break;}
  }
  
  /* Write segment to new dst path data */
  newSegs[(*segCount)++] = segment | VG_ABSOLUTE;
}

VG_API_CALL void vgTransformPath(VGPath dstPath, VGPath srcPath)
{
  SHint newSegCount=0;
  SHint newDataCount=0;
  SHPath *src, *dst;
  SHuint8 *newSegs = NULL;
  SHuint8 *newData = NULL;
  SHint segCount = 0;
  SHint dataCount = 0;
  void *userData[5];
  SHint processFlags =
    SH_PROCESS_SIMPLIFY_LINES;
  
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, dstPath) ||
                   !shIsValidPath(context, srcPath),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
  
  src = (SHPath*)srcPath; dst = (SHPath*)dstPath;
  VG_RETURN_ERR_IF(!(src->caps & VG_PATH_CAPABILITY_TRANSFORM_FROM) ||
                   !(dst->caps & VG_PATH_CAPABILITY_TRANSFORM_TO),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);
  
  /* Resize path storage */
  shProcessedDataCount(src, processFlags, &newSegCount, &newDataCount);
  shResizePathData(dst, newSegCount, newDataCount, &newSegs, &newData);
  VG_RETURN_ERR_IF(!newData, VG_OUT_OF_MEMORY_ERROR, VG_NO_RETVAL);
  
  /* Transform src path into new data */
  segCount = dst->segCount;
  dataCount = dst->dataCount;
  userData[0] = newSegs; userData[1] = &segCount;
  userData[2] = newData; userData[3] = &dataCount;
  userData[4] = dst;
  shProcessPathData(src, processFlags, shTransformSegment, userData);
  
  /* Free old arrays */
  free(dst->segs);
  free(dst->data);
  
  /* Adjust new properties */
  dst->segs = newSegs;
  dst->data = newData;
  dst->segCount = segCount;
  dst->dataCount = dataCount;

  /* Mark change */
  dst->cacheDataValid = VG_FALSE;
  
  VG_RETURN_ERR(VG_NO_ERROR, VG_NO_RETVAL);
}

static void shInterpolateSegment(SHPath *p, VGPathSegment segment,
                                 VGPathCommand originalCommand,
                                 SHfloat *data, void *userData)
{
  SHuint8* procSegs      = (SHuint8*) ((void**)userData)[0];
  SHint*   procSegCount  = (SHint*)   ((void**)userData)[1];
  SHfloat* procData      = (SHfloat*) ((void**)userData)[2];
  SHint*   procDataCount = (SHint*)   ((void**)userData)[3];
  SHint segindex, i;
  
  /* Write the processed segment back */
  procSegs[(*procSegCount)++] = segment | VG_ABSOLUTE;
  
  /* Write the processed data back (exclude first 2 pen coords!) */
  segindex = (segment >> 1);
  for (i=2; i<2+shCoordsPerCommand[segindex]; ++i)
    procData[(*procDataCount)++] = data[i];
}

VG_API_CALL VGboolean vgInterpolatePath(VGPath dstPath, VGPath startPath,
                                        VGPath endPath, VGfloat amount)
{
  SHPath *dst, *start, *end;
  SHuint8 *procSegs1, *procSegs2;
  SHfloat *procData1, *procData2;
  SHint procSegCount1=0, procSegCount2=0;
  SHint procDataCount1=0, procDataCount2=0;
  SHuint8 *newSegs, *newData;
  void *userData[4];
  SHint segment1, segment2;
  SHint segindex, s,d,i;
  SHint processFlags =
    SH_PROCESS_SIMPLIFY_LINES |
    SH_PROCESS_SIMPLIFY_CURVES;
  
  VG_GETCONTEXT(VG_FALSE);
  
  VG_RETURN_ERR_IF(!shIsValidPath(context, dstPath) ||
                   !shIsValidPath(context, startPath) ||
                   !shIsValidPath(context, endPath),
                   VG_BAD_HANDLE_ERROR, VG_FALSE);
  
  dst = (SHPath*)dstPath; start = (SHPath*)startPath; end = (SHPath*)endPath;
  VG_RETURN_ERR_IF(!(start->caps & VG_PATH_CAPABILITY_INTERPOLATE_FROM) ||
                   !(end->caps & VG_PATH_CAPABILITY_INTERPOLATE_FROM) ||
                   !(dst->caps & VG_PATH_CAPABILITY_INTERPOLATE_TO),
                   VG_PATH_CAPABILITY_ERROR, VG_FALSE);
  
  /* Segment count must be equal */
  VG_RETURN_ERR_IF(start->segCount != end->segCount,
                   VG_NO_ERROR, VG_FALSE);
  
  /* Allocate storage for processed path data */
  shProcessedDataCount(start, processFlags, &procSegCount1, &procDataCount1);
  shProcessedDataCount(end, processFlags, &procSegCount2, &procDataCount2);
  procSegs1 = (SHuint8*)malloc(procSegCount1 * sizeof(SHuint8));
  procSegs2 = (SHuint8*)malloc(procSegCount2 * sizeof(SHuint8));
  procData1 = (SHfloat*)malloc(procDataCount1 * sizeof(SHfloat));
  procData2 = (SHfloat*)malloc(procDataCount2 * sizeof(SHfloat));
  if (!procSegs1 || !procSegs2 || !procData1 || !procData2) {
    free(procSegs1); free(procSegs2); free(procData1); free(procData2);
    VG_RETURN_ERR(VG_OUT_OF_MEMORY_ERROR, VG_FALSE);
  }
  
  /* Process data of start path */
  procSegCount1 = 0; procDataCount1 = 0;
  userData[0] = procSegs1; userData[1] = &procSegCount1;
  userData[2] = procData1; userData[3] = &procDataCount1;
  shProcessPathData(start, processFlags, shInterpolateSegment, userData);
  
  /* Process data of end path */
  procSegCount2 = 0; procDataCount2 = 0;
  userData[0] = procSegs2; userData[1] = &procSegCount2;
  userData[2] = procData2; userData[3] = &procDataCount2;
  shProcessPathData(end, processFlags, shInterpolateSegment, userData);
  SH_ASSERT(procSegCount1 == procSegCount2 &&
            procDataCount1 == procDataCount2);
  
  /* Resize dst path storage to include interpolated data */
  shResizePathData(dst, procSegCount1, procDataCount1, &newSegs, &newData);
  if (!newData) {
    free(procSegs1); free(procData1);
    free(procSegs2); free(procData2);
    VG_RETURN_ERR(VG_OUT_OF_MEMORY_ERROR, VG_FALSE);
  }
  
  /* Interpolate data between paths */
  for (s=0, d=0; s<procSegCount1; ++s) {
    
    segment1 = (procSegs1[s] & 0x1E);
    segment2 = (procSegs2[s] & 0x1E);
    
    /* Pick the right arc type */
    if (shIsArcSegment(segment1) &&
        shIsArcSegment(segment2)) {
      
      if (amount < 0.5f)
        segment2 = segment1;
      else
        segment1 = segment2;
    }
    
    /* Segment types must match */
    if (segment1 != segment2) {
      free(procSegs1); free(procData1);
      free(procSegs2); free(procData2);
      free(newSegs); free(newData);
      VG_RETURN_ERR(VG_NO_ERROR, VG_FALSE);
    }
    
    /* Interpolate values */
    segindex = (segment1 >> 1);
    newSegs[s] = segment1 | VG_ABSOLUTE;
    for (i=0; i<shCoordsPerCommand[segindex]; ++i, ++d) {
      SHfloat diff = procData2[d] - procData1[d];
      SHfloat value = procData1[d] + amount * diff;
      shRealCoordToData(dst->datatype, dst->scale, dst->bias,
                        newData, dst->dataCount + d, value);
    }
  }
  
  /* Free processed data */
  free(procSegs1); free(procData1);
  free(procSegs2); free(procData2);
  
  /* Assign interpolated data */
  dst->segs = newSegs;
  dst->data = newData;
  dst->segCount += procSegCount1;
  dst->dataCount += procDataCount1;

  /* Mark change */
  dst->cacheDataValid = VG_FALSE;
  
  VG_RETURN_ERR(VG_NO_ERROR, VG_TRUE);
}
