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
#include "shGeometry.h"


static int shAddVertex(SHPath *p, SHVertex *v, SHint *contourStart)
{
  /* Assert contour was open */
  SH_ASSERT((*contourStart) >= 0);
  
  /* Check vertex limit */
  if (p->vertices.size >= SH_MAX_VERTICES) return 0;
  
  /* Add vertex to subdivision */
  shVertexArrayPushBackP(&p->vertices, v);
  
  /* Increment contour size. Its stored in
     the flags of first contour vertex */
  p->vertices.items[*contourStart].flags++;
  
  return 1;
}

static void shSubrecurseQuad(SHPath *p, SHQuad *quad, SHint *contourStart)
{
  SHVertex v;
  SHVector2 mid, dif, c1, c2, c3;
  SHQuad quads[SH_MAX_RECURSE_DEPTH];
  SHQuad *q, *qleft, *qright;
  SHint qindex=0;
  quads[0] = *quad;
  
  while (qindex >= 0) {
    
    q = &quads[qindex];
    
    /* Calculate distance of control point from its
     counterpart on the line between end points */
    SET2V(mid, q->p1); ADD2V(mid, q->p3); DIV2(mid, 2);
    SET2V(dif, q->p2); SUB2V(dif, mid); ABS2(dif);
    
    /* Cancel if the curve is flat enough */
    if (dif.x + dif.y <= 1.0f || qindex == SH_MAX_RECURSE_DEPTH-1) {

      /* Add subdivision point */
      v.point = q->p3; v.flags = 0;
      if (qindex == 0) return; /* Skip last point */
      if (!shAddVertex(p, &v, contourStart)) return;
      --qindex;
      
    }else{
      
      /* Left recursion goes on top of stack! */
      qright = q; qleft = &quads[++qindex];
      
      /* Subdivide into 2 sub-curves */
      SET2V(c1, q->p1); ADD2V(c1, q->p2); DIV2(c1, 2);
      SET2V(c3, q->p2); ADD2V(c3, q->p3); DIV2(c3, 2);
      SET2V(c2, c1); ADD2V(c2, c3); DIV2(c2, 2);
  
      /* Add left recursion onto stack */
      qleft->p1 = q->p1;
      qleft->p2 = c1;
      qleft->p3 = c2;
      
      /* Add right recursion onto stack */
      qright->p1 = c2;
      qright->p2 = c3;
      qright->p3 = q->p3;
    }
  }
}

static void shSubrecurseCubic(SHPath *p, SHCubic *cubic, SHint *contourStart)
{
  SHVertex v;
  SHfloat dx1, dy1, dx2, dy2;
  SHVector2 mm, c1, c2, c3, c4, c5;
  SHCubic cubics[SH_MAX_RECURSE_DEPTH];
  SHCubic *c, *cleft, *cright;
  SHint cindex = 0;
  cubics[0] = *cubic;
  
  while (cindex >= 0) {
    
    c = &cubics[cindex];
    
    /* Calculate distance of control points from their
     counterparts on the line between end points */
    dx1 = 3.0f*c->p2.x - 2.0f*c->p1.x - c->p4.x; dx1 *= dx1;
    dy1 = 3.0f*c->p2.y - 2.0f*c->p1.y - c->p4.y; dy1 *= dy1;
    dx2 = 3.0f*c->p3.x - 2.0f*c->p4.x - c->p1.x; dx2 *= dx2;
    dy2 = 3.0f*c->p3.y - 2.0f*c->p4.y - c->p1.y; dy2 *= dy2;
    if (dx1 < dx2) dx1 = dx2;
    if (dy1 < dy2) dy1 = dy2;
    
    /* Cancel if the curve is flat enough */
    if (dx1+dy1 <= 1.0 || cindex == SH_MAX_RECURSE_DEPTH-1) {
      
      /* Add subdivision point */
      v.point = c->p4; v.flags = 0;
      if (cindex == 0) return; /* Skip last point */
      if (!shAddVertex(p, &v, contourStart)) return;
      --cindex;
      
    }else{
      
      /* Left recursion goes on top of stack! */
      cright = c; cleft = &cubics[++cindex];
      
      /* Subdivide into 2 sub-curves */
      SET2V(c1, c->p1); ADD2V(c1, c->p2); DIV2(c1, 2);
      SET2V(mm, c->p2); ADD2V(mm, c->p3); DIV2(mm, 2);
      SET2V(c5, c->p3); ADD2V(c5, c->p4); DIV2(c5, 2);
      
      SET2V(c2, c1); ADD2V(c2, mm); DIV2(c2, 2);
      SET2V(c4, mm); ADD2V(c4, c5); DIV2(c4, 2);
      
      SET2V(c3, c2); ADD2V(c3, c4); DIV2(c3, 2);
      
      /* Add left recursion to stack */
      cleft->p1 = c->p1;
      cleft->p2 = c1;
      cleft->p3 = c2;
      cleft->p4 = c3;
      
      /* Add right recursion to stack */
      cright->p1 = c3;
      cright->p2 = c4;
      cright->p3 = c5;
      cright->p4 = c->p4;
    }
  }
}

static void shSubrecurseArc(SHPath *p, SHArc *arc,
                            SHVector2 *c,SHVector2 *ux, SHVector2 *uy,
                            SHint *contourStart)
{
  SHVertex v;
  SHfloat am, cosa, sina, dx, dy;
  SHVector2 uux, uuy, c1, m;
  SHArc arcs[SH_MAX_RECURSE_DEPTH];
  SHArc *a, *aleft, *aright;
  SHint aindex=0;
  arcs[0] = *arc;
  
  while (aindex >= 0) {
    
    a = &arcs[aindex];
    
    /* Middle angle and its cos/sin */
    am = (a->a1 + a->a2)/2;
    cosa = SH_COS(am);
    sina = SH_SIN(am);
  
    /* New point */
    SET2V(uux, (*ux)); MUL2(uux, cosa);
    SET2V(uuy, (*uy)); MUL2(uuy, sina);
    SET2V(c1, (*c)); ADD2V(c1, uux); ADD2V(c1, uuy);
    
    /* Check distance from linear midpoint */
    SET2V(m, a->p1); ADD2V(m, a->p2); DIV2(m, 2);
    dx = c1.x - m.x; dy = c1.y - m.y;
    if (dx < 0.0f) dx = -dx;
    if (dy < 0.0f) dy = -dy;
    
    /* Stop if flat enough */
    if (dx+dy <= 1.0f || aindex == SH_MAX_RECURSE_DEPTH-1) {
      
      /* Add middle subdivision point */
      v.point = c1; v.flags = 0;
      if (!shAddVertex(p, &v, contourStart)) return;
      if (aindex == 0) return; /* Skip very last point */
      
      /* Add end subdivision point */
      v.point = a->p2; v.flags = 0;
      if (!shAddVertex(p, &v, contourStart)) return;
      --aindex;
      
    }else{
      
      /* Left subdivision goes on top of stack! */
      aright = a; aleft = &arcs[++aindex];
      
      /* Add left recursion to stack */
      aleft->p1 = a->p1;
      aleft->a1 = a->a1;
      aleft->p2 = c1;
      aleft->a2 = am;
      
      /* Add right recursion to stack */
      aright->p1 = c1;
      aright->a1 = am;
      aright->p2 = a->p2;
      aright->a2 = a->a2;
    }
  }
}

static void shSubdivideSegment(SHPath *p, VGPathSegment segment,
                               VGPathCommand originalCommand,
                               SHfloat *data, void *userData)
{
  SHVertex v;
  SHint *contourStart = ((SHint**)userData)[0];
  SHint *surfaceSpace = ((SHint**)userData)[1];
  SHQuad quad; SHCubic cubic; SHArc arc;
  SHVector2 c, ux, uy;
  VG_GETCONTEXT(VG_NO_RETVAL);
  
  switch (segment)
  {
  case VG_MOVE_TO:
    
    /* Set contour start here */
    (*contourStart) = p->vertices.size;
    
    /* First contour vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = 0;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_CLOSE_PATH:
    
    /* Last contour vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = SH_VERTEX_FLAG_SEGEND | SH_VERTEX_FLAG_CLOSE;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_LINE_TO:
    
    /* Last segment vertex */
    v.point.x = data[2];
    v.point.y = data[3];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_QUAD_TO:
    
    /* Recurse subdivision */
    SET2(quad.p1, data[0], data[1]);
    SET2(quad.p2, data[2], data[3]);
    SET2(quad.p3, data[4], data[5]);
    if (*surfaceSpace) {
      TRANSFORM2(quad.p1, context->pathTransform);
      TRANSFORM2(quad.p2, context->pathTransform);
      TRANSFORM2(quad.p3, context->pathTransform); }
    shSubrecurseQuad(p, &quad, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[4];
    v.point.y = data[5];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  case VG_CUBIC_TO:
    
    /* Recurse subdivision */
    SET2(cubic.p1, data[0], data[1]);
    SET2(cubic.p2, data[2], data[3]);
    SET2(cubic.p3, data[4], data[5]);
    SET2(cubic.p4, data[6], data[7]);
    if (*surfaceSpace) {
      TRANSFORM2(cubic.p1, context->pathTransform);
      TRANSFORM2(cubic.p2, context->pathTransform);
      TRANSFORM2(cubic.p3, context->pathTransform);
      TRANSFORM2(cubic.p4, context->pathTransform); }
    shSubrecurseCubic(p, &cubic, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[6];
    v.point.y = data[7];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace)
      TRANSFORM2(v.point, context->pathTransform);
    break;
    
  default:
    
    SH_ASSERT(segment==VG_SCWARC_TO || segment==VG_SCCWARC_TO ||
              segment==VG_LCWARC_TO || segment==VG_LCCWARC_TO);
    
    /* Recurse subdivision */
    SET2(arc.p1, data[0], data[1]);
    SET2(arc.p2, data[10], data[11]);
    arc.a1 = data[8]; arc.a2 = data[9];
    SET2(c,  data[2], data[3]);
    SET2(ux, data[4], data[5]);
    SET2(uy, data[6], data[7]);
    if (*surfaceSpace) {
      TRANSFORM2(arc.p1, context->pathTransform);
      TRANSFORM2(arc.p2, context->pathTransform);
      TRANSFORM2(c, context->pathTransform);
      TRANSFORM2DIR(ux, context->pathTransform);
      TRANSFORM2DIR(uy, context->pathTransform); }
    shSubrecurseArc(p, &arc, &c, &ux, &uy, contourStart);
    
    /* Last segment vertex */
    v.point.x = data[10];
    v.point.y = data[11];
    v.flags = SH_VERTEX_FLAG_SEGEND;
    if (*surfaceSpace) {
      TRANSFORM2(v.point, context->pathTransform); }
    break;
  }
  
  /* Add subdivision vertex */
  shAddVertex(p, &v, contourStart);
}

/*--------------------------------------------------
 * Processes path data by simplfying it and sending
 * each segment to subdivision callback function
 *--------------------------------------------------*/

void shFlattenPath(SHPath *p, SHint surfaceSpace)
{
  SHint contourStart = -1;
//  SHint surfSpace = surfaceSpace;
  SHint *userData[2];
  SHint processFlags =
    SH_PROCESS_SIMPLIFY_LINES |
    SH_PROCESS_SIMPLIFY_CURVES |
    SH_PROCESS_CENTRALIZE_ARCS |
    SH_PROCESS_REPAIR_ENDS;
  
  userData[0] = &contourStart;
  userData[1] = &surfaceSpace;
  
  shVertexArrayClear(&p->vertices);
  shProcessPathData(p, processFlags, shSubdivideSegment, userData);
}

/*-------------------------------------------
 * Adds a rectangle to the path's stroke.
 *-------------------------------------------*/

static void shPushStrokeQuad(SHPath *p, SHVector2 *p1, SHVector2 *p2,
                             SHVector2 *p3, SHVector2 *p4)
{
  shVector2ArrayPushBackP(&p->stroke, p1);
  shVector2ArrayPushBackP(&p->stroke, p2);
  shVector2ArrayPushBackP(&p->stroke, p3);
  shVector2ArrayPushBackP(&p->stroke, p3);
  shVector2ArrayPushBackP(&p->stroke, p4);
  shVector2ArrayPushBackP(&p->stroke, p1);
}

/*-------------------------------------------
 * Adds a triangle to the path's stroke.
 *-------------------------------------------*/

static void shPushStrokeTri(SHPath *p, SHVector2 *p1,
                            SHVector2 *p2, SHVector2 *p3)
{
  shVector2ArrayPushBackP(&p->stroke, p1);
  shVector2ArrayPushBackP(&p->stroke, p2);
  shVector2ArrayPushBackP(&p->stroke, p3);
}

/*-----------------------------------------------------------
 * Adds a miter join to the path's stroke at the given
 * turn point [c], with the end of the previous segment
 * outset [o1] and the beginning of the next segment
 * outset [o2], transiting from tangent [d1] to [d2].
 *-----------------------------------------------------------*/

static void shStrokeJoinMiter(SHPath *p, SHVector2 *c,
                              SHVector2 *o1, SHVector2 *d1,
                              SHVector2 *o2, SHVector2 *d2)
{
  /* Init miter top to first point in case lines are colinear */
  SHVector2 x; SET2V(x,(*o1));
  
  /* Find intersection of two outer turn edges
     (lines defined by origin and direction) */
  shLineLineXsection(o1, d1, o2, d2, &x);
  
  /* Add a "diamond" quad with top on intersected point
     and bottom on center of turn (on the line) */
  shPushStrokeQuad(p, &x, o1, c, o2);
}

/*-----------------------------------------------------------
 * Adds a round join to the path's stroke at the given
 * turn point [c], with the end of the previous segment
 * outset [pstart] and the beginning of the next segment
 * outset [pend], transiting from perpendicular vector
 * [tstart] to [tend].
 *-----------------------------------------------------------*/

static void shStrokeJoinRound(SHPath *p, SHVector2 *c,
                              SHVector2 *pstart, SHVector2 *tstart, 
                              SHVector2 *pend, SHVector2 *tend)
{
  SHVector2 p1, p2;
  SHfloat a, ang, cosa, sina;
  
  /* Find angle between lines */
  ang = ANGLE2((*tstart),(*tend));
  
  /* Begin with start point */
  SET2V(p1,(*pstart));
  for (a=0.0f; a<ang; a+=PI/12) {
    
    /* Rotate perpendicular vector around and
       find next offset point from center */
    cosa = SH_COS(-a);
    sina = SH_SIN(-a);
    SET2(p2, tstart->x*cosa - tstart->y*sina,
         tstart->x*sina + tstart->y*cosa);
    ADD2V(p2, (*c));
    
    /* Add triangle, save previous */
    shPushStrokeTri(p, &p1, &p2, c);
    SET2V(p1, p2);
  }
  
  /* Add last triangle */
  shPushStrokeTri(p, &p1, pend, c);
}

static void shStrokeCapRound(SHPath *p, SHVector2 *c, SHVector2 *t, SHint start)
{
  SHint a;
  SHfloat ang, cosa, sina;
  SHVector2 p1, p2;
  SHint steps = 12;
  SHVector2 tt;
  
  /* Revert perpendicular vector if start cap */
  SET2V(tt, (*t));
  if (start) MUL2(tt, -1);
  
  /* Find start point */
  SET2V(p1, (*c));
  ADD2V(p1, tt);
  
  for (a = 1; a<=steps; ++a) {
    
    /* Rotate perpendicular vector around and
       find next offset point from center */
    ang = (SHfloat)a * PI / steps;
    cosa = SH_COS(-ang);
    sina = SH_SIN(-ang);
    SET2(p2, tt.x*cosa - tt.y*sina,
         tt.x*sina + tt.y*cosa);
    ADD2V(p2, (*c));
    
    /* Add triangle, save previous */
    shPushStrokeTri(p, &p1, &p2, c);
    SET2V(p1, p2);
  }
}

static void shStrokeCapSquare(SHPath *p, SHVector2 *c, SHVector2 *t, SHint start)
{
  SHVector2 tt, p1, p2, p3, p4;
  
  /* Revert perpendicular vector if start cap */
  SET2V(tt, (*t));
  if (start) MUL2(tt, -1);
  
  /* Find four corners of the quad */
  SET2V(p1, (*c));
  ADD2V(p1, tt);
  
  SET2V(p2, p1);
  ADD2(p2, tt.y, -tt.x);
  
  SET2V(p3, p2);
  ADD2(p3, -2*tt.x, -2*tt.y);
  
  SET2V(p4, p3);
  ADD2(p4, -tt.y, tt.x);
  
  shPushStrokeQuad(p, &p1, &p2, &p3, &p4);
}

/*-----------------------------------------------------------
 * Generates stroke of a path according to VGContext state.
 * Produces quads for every linear subdivision segment or
 * dash "on" segment, handles line caps and joins.
 *-----------------------------------------------------------*/

void shStrokePath(VGContext* c, SHPath *p)
{
  /* Line width and vertex count */
  SHfloat w = c->strokeLineWidth / 2;
  SHfloat mlimit = c->strokeMiterLimit;
  SHint vertsize = p->vertices.size;
  
  /* Contour state */
  SHint contourStart = 0;
  SHint contourLength = 0;
  SHint start = 0;
  SHint end = 0;
  SHint loop = 0;
  SHint close = 0;
  SHint segend = 0;
  
  /* Current vertices */
  SHint i1=0, i2=0;
  SHVertex *v1, *v2;
  SHVector2 *p1, *p2;
  SHVector2 d, t, dprev, tprev;
  SHfloat norm, cross, mlength;
  
  /* Stroke edge points */
  SHVector2 l1, r1, l2, r2, lprev, rprev;
  
  /* Dash state */
  SHint dashIndex = 0;
  SHfloat dashLength = 0.0f, strokeLength = 0.0f;
  SHint dashSize = c->strokeDashPattern.size;
  SHfloat *dashPattern = c->strokeDashPattern.items;
  SHint dashOn = 1;
  
  /* Dash edge points */
  SHVector2 dash1, dash2;
  SHVector2 dashL1, dashR1;
  SHVector2 dashL2, dashR2;
  SHfloat nextDashLength, dashOffset;
  
  /* Discard odd dash segment */
  dashSize -= dashSize % 2;

  /* Init previous so compiler doesn't warn
     for uninitialized usage */
  SET2(tprev, 0,0); SET2(dprev, 0,0);
  SET2(lprev, 0,0); SET2(rprev, 0,0);

  
  /* Walk over subdivision vertices */
  for (i1=0; i1<vertsize; ++i1) {
    
    if (loop) {
      /* Start new contour if exists */
      if (contourStart < vertsize)
        i1 = contourStart;
      else break;
    }
    
    start = end = loop = close = segend = 0;
    i2 = i1 + 1;
    
    if (i1 == contourStart) {
      /* Contour has started. Get length */
      contourLength = p->vertices.items[i1].flags;
      start = 1;
    }
    
    if (contourLength <= 1) {
      /* Discard empty contours. */
      contourStart = i1 + 1;
      loop = 1;
      continue;
    }
    
    v1 = &p->vertices.items[i1];
    v2 = &p->vertices.items[i2];
    
    if (i2 == contourStart + contourLength-1) {
      /* Contour has ended. Check close */
      close = v2->flags & SH_VERTEX_FLAG_CLOSE;
      end = 1;
    }
    
    if (i1 == contourStart + contourLength-1) {
      /* Loop back to first edge. Check close */
      close = v1->flags & SH_VERTEX_FLAG_CLOSE;
      i2 = contourStart+1;
      contourStart = i1 + 1;
      i1 = i2 - 1;
      loop = 1;
    }
    
    if (!start && !loop) {
      /* We are inside a contour. Check segment end. */
      segend = (v1->flags & SH_VERTEX_FLAG_SEGEND);
    }
    
    if (dashSize > 0 && start &&
        (contourStart == 0 || c->strokeDashPhaseReset)) {
      
      /* Reset pattern phase at contour start */
      dashLength = -c->strokeDashPhase;
      strokeLength = 0.0f;
      dashIndex = 0;
      dashOn = 1;
      
      if (dashLength < 0.0f) {
        /* Consume dash segments forward to reach stroke start */
        while (dashLength + dashPattern[dashIndex] <= 0.0f) {
          dashLength += dashPattern[dashIndex];
          dashIndex = (dashIndex + 1) % dashSize;
          dashOn = !dashOn; }
        
      }else if (dashLength > 0.0f) {
        /* Consume dash segments backward to return to stroke start */
        dashIndex = dashSize;
        while (dashLength > 0.0f) {
          dashIndex = dashIndex ? (dashIndex-1) : (dashSize-1);
          dashLength -= dashPattern[dashIndex];
          dashOn = !dashOn; }
      }
    }
    
    /* Subdiv segment vertices and points */
    v1 = &p->vertices.items[i1];
    v2 = &p->vertices.items[i2];
    p1 = &v1->point;
    p2 = &v2->point;
    
    /* Direction vector */
    SET2(d, p2->x-p1->x, p2->y-p1->y);
    norm = NORM2(d);
    if (norm == 0.0f) d = dprev;
    else DIV2(d, norm);
    
    /* Perpendicular vector */
    SET2(t, -d.y, d.x);
    MUL2(t, w);
    cross = CROSS2(t,tprev);
    
    /* Left and right edge points */
    SET2V(l1, (*p1)); ADD2V(l1, t);
    SET2V(r1, (*p1)); SUB2V(r1, t);
    SET2V(l2, (*p2)); ADD2V(l2, t);
    SET2V(r2, (*p2)); SUB2V(r2, t);
    
    /* Check if join needed */
    if ((segend || (loop && close)) && dashOn) {
      
      switch (c->strokeJoinStyle) {
      case VG_JOIN_ROUND:
        
        /* Add a round join to stroke */
        if (cross >= 0.0f)
          shStrokeJoinRound(p, p1, &lprev, &tprev, &l1, &t);
        else{
          SHVector2 _t, _tprev;
          SET2(_t, -t.x, -t.y);
          SET2(_tprev, -tprev.x, -tprev.y);
          shStrokeJoinRound(p, p1,  &r1, &_t, &rprev, &_tprev);
        }
        
        break;
      case VG_JOIN_MITER:
        
        /* Add a miter join to stroke */
        mlength = 1/SH_COS((ANGLE2(t, tprev))/2);
        if (mlength <= mlimit) {
          if (cross > 0.0f)
            shStrokeJoinMiter(p, p1, &lprev, &dprev, &l1, &d);
          else if (cross < 0.0f)
            shStrokeJoinMiter(p, p1, &rprev, &dprev, &r1, &d);
          break;
        }/* Else fall down to bevel */
        
      case VG_JOIN_BEVEL:
        
        /* Add a bevel join to stroke */
        if (cross > 0.0f)
          shPushStrokeTri(p, &l1, &lprev, p1);
        else if (cross < 0.0f)
          shPushStrokeTri(p, &r1, &rprev, p1);
        
        break;
      }
    }else if (!start && !loop && dashOn) {
      
      /* Fill gap with previous (= bevel join) */
      if (cross > 0.0f)
        shPushStrokeTri(p, &l1, &lprev, p1);
      else if (cross < 0.0f)
        shPushStrokeTri(p, &r1, &rprev, p1);
    }
    
    
    /* Apply cap to start of a non-closed contour or
       if we are dashing and dash segment is on */
    if ((dashSize == 0 && loop && !close) ||
        (dashSize > 0 && start && dashOn)) {
      switch (c->strokeCapStyle) {
      case VG_CAP_ROUND:
        shStrokeCapRound(p, p1, &t, 1); break;
      case VG_CAP_SQUARE:
        shStrokeCapSquare(p, p1, &t, 1); break;
      default: break;
      }
    }
    
    if (loop)
      continue;
    
    /* Handle dashing */
    if (dashSize > 0) {
      
      /* Start with beginning of subdiv segment */
      SET2V(dash1, (*p1)); SET2V(dashL1, l1); SET2V(dashR1, r1);
      
      do {
        /* Interpolate point on the current subdiv segment */
        nextDashLength = dashLength + dashPattern[dashIndex];
        dashOffset = (nextDashLength - strokeLength) / norm;
        if (dashOffset > 1.0f) dashOffset = 1;
        SET2V(dash2, (*p2)); SUB2V(dash2, (*p1));
        MUL2(dash2, dashOffset); ADD2V(dash2, (*p1));
        
        /* Left and right edge points */
        SET2V(dashL2, dash2); ADD2V(dashL2, t);
        SET2V(dashR2, dash2); SUB2V(dashR2, t);
        
        /* Add quad for this dash segment */
        if (dashOn) shPushStrokeQuad(p, &dashL2, &dashL1, &dashR1, &dashR2);

        /* Move to next dash segment if inside this subdiv segment */
        if (nextDashLength <= strokeLength + norm) {
          dashIndex = (dashIndex + 1) % dashSize;
          dashLength = nextDashLength;
          dashOn = !dashOn;
          SET2V(dash1, dash2);
          SET2V(dashL1, dashL2);
          SET2V(dashR1, dashR2);
          
          /* Apply cap to dash segment */
          switch (c->strokeCapStyle) {
          case VG_CAP_ROUND:
            shStrokeCapRound(p, &dash1, &t, dashOn); break;
          case VG_CAP_SQUARE:
            shStrokeCapSquare(p, &dash1, &t, dashOn); break;
          default: break;
          }
        }
        
        /* Consume dash segments until subdiv end met */
      } while (nextDashLength < strokeLength + norm);
      
    }else{
      
      /* Add quad for this line segment */
      shPushStrokeQuad(p, &l2, &l1, &r1, &r2);
    }
    
    
    /* Apply cap to end of a non-closed contour or
       if we are dashing and dash segment is on */
    if ((dashSize == 0 && end && !close) ||
        (dashSize > 0 && end && dashOn)) {
      switch (c->strokeCapStyle) {
      case VG_CAP_ROUND:
        shStrokeCapRound(p, p2, &t, 0); break;
      case VG_CAP_SQUARE:
        shStrokeCapSquare(p, p2, &t, 0); break;
      default: break;
      }
    }
    
    /* Save previous edge */
    strokeLength += norm;
    SET2V(lprev, l2);
    SET2V(rprev, r2);
    dprev = d;
    tprev = t;
  }
}


/*-------------------------------------------------------------
 * Transforms the tessellation vertices using the given matrix
 *-------------------------------------------------------------*/

void shTransformVertices(SHMatrix3x3 *m, SHPath *p)
{
  SHVector2 *v;
  int i = 0;
  
  for (i = p->vertices.size-1; i>=0; --i) {
    v = (&p->vertices.items[i].point);
    TRANSFORM2((*v), (*m)); }
}

/*--------------------------------------------------------
 * Finds the tight bounding box of path's tesselation
 * vertices. Depends on whether the path had been
 * tesselated in user or surface space.
 *--------------------------------------------------------*/

void shFindBoundbox(SHPath *p)
{
  int i;
  
  if (p->vertices.size == 0) {
    SET2(p->min, 0,0);
    SET2(p->max, 0,0);
    return;
  }
  
  p->min.x = p->max.x = p->vertices.items[0].point.x;
  p->min.y = p->max.y = p->vertices.items[0].point.y;
  
  for (i=0; i<p->vertices.size; ++i) {
    
    SHVector2 *v = &p->vertices.items[i].point;
    if (v->x < p->min.x) p->min.x = v->x;
    if (v->x > p->max.x) p->max.x = v->x;
    if (v->y < p->min.y) p->min.y = v->y;
    if (v->y > p->max.y) p->max.y = v->y;
  }
}

/*--------------------------------------------------------
 * Outputs a tight bounding box of a path in path's own
 * coordinate system.
 *--------------------------------------------------------*/

VG_API_CALL void vgPathBounds(VGPath path,
                              VGfloat * minX, VGfloat * minY,
                              VGfloat * width, VGfloat * height)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);

  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

  VG_RETURN_ERR_IF(minX == NULL || minY == NULL ||
                   width == NULL || height == NULL,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* TODO: check output pointer alignment */

  p = (SHPath*)path;
  VG_RETURN_ERR_IF(!(p->caps & VG_PATH_CAPABILITY_PATH_BOUNDS),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);

  /* Update path geometry */
  shFlattenPath(p, 0);
  shFindBoundbox(p);

  /* Output bounds */
  *minX = p->min.x;
  *minY = p->min.y;
  *width = p->max.x - p->min.x;
  *height = p->max.y - p->min.y;
  
  VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------
 * Outputs a bounding box of a path defined by its control
 * points that is guaranteed to enclose the path geometry
 * after applying the current path-user-to-surface transform
 *------------------------------------------------------------*/

VG_API_CALL void vgPathTransformedBounds(VGPath path,
                                         VGfloat * minX, VGfloat * minY,
                                         VGfloat * width, VGfloat * height)
{
  SHPath *p = NULL;
  VG_GETCONTEXT(VG_NO_RETVAL);

  VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                   VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

  VG_RETURN_ERR_IF(minX == NULL || minY == NULL ||
                   width == NULL || height == NULL,
                   VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

  /* TODO: check output pointer alignment */

  p = (SHPath*)path;
  VG_RETURN_ERR_IF(!(p->caps & VG_PATH_CAPABILITY_PATH_BOUNDS),
                   VG_PATH_CAPABILITY_ERROR, VG_NO_RETVAL);

  /* Update path geometry */
  shFlattenPath(p, 1);
  shFindBoundbox(p);

  /* Output bounds */
  *minX = p->min.x;
  *minY = p->min.y;
  *width = p->max.x - p->min.x;
  *height = p->max.y - p->min.y;

  /* Invalidate subdivision for rendering */
  p->cacheDataValid = VG_FALSE;
  
  VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGfloat vgPathLength(VGPath path,
                                 VGint startSegment, VGint numSegments)
{
  return 0.0f;
}

VG_API_CALL void vgPointAlongPath(VGPath path,
                                  VGint startSegment, VGint numSegments,
                                  VGfloat distance,
                                  VGfloat * x, VGfloat * y,
                                  VGfloat * tangentX, VGfloat * tangentY)
{
}
