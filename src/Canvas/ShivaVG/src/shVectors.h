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

#ifndef __SHVECTORS_H
#define __SHVECTORS_H

#include "shDefs.h"

/* Vector structures
 *--------------------------------------------------------------*/
typedef struct
{
  SHfloat x,y;
} SHVector2;

void SHVector2_ctor(SHVector2 *v);
void SHVector2_dtor(SHVector2 *v);

typedef struct
{
  SHfloat x,y,z;
} SHVector3;

void SHVector3_ctor(SHVector3 *v);
void SHVector3_dtor(SHVector3 *v);

typedef struct
{
  SHfloat x,y,z,w;
} SHVector4;

void SHVector4_ctor(SHVector4 *v);
void SHVector4_dtor(SHVector4 *v);

typedef struct
{
  SHfloat x,y,w,h;
} SHRectangle;

void SHRectangle_ctor(SHRectangle *r);
void SHRectangle_dtor(SHRectangle *r);
void shRectangleSet(SHRectangle *r, SHfloat x,
                    SHfloat y, SHfloat w, SHfloat h);

typedef struct
{
  SHfloat m[3][3];
} SHMatrix3x3;

void SHMatrix3x3_ctor(SHMatrix3x3 *m);
void SHMatrix3x3_dtor(SHMatrix3x3 *m);

/*------------------------------------------------------------
 * Vector Arrays
 *------------------------------------------------------------*/

#define _ITEM_T SHVector2
#define _ARRAY_T SHVector2Array
#define _FUNC_T shVector2Array
#define _ARRAY_DECLARE
#include "shArrayBase.h"

/*--------------------------------------------------------
 * Macros for typical vector operations. The only way to
 * inline in C is to actually write a macro
 *--------------------------------------------------------- */

#define SET2(v,xs,ys) { v.x=xs; v.y=ys; }
#define SET3(v,xs,ys,zs) { v.x=xs; v.y=ys; v.z=zs; }
#define SET4(v,xs,ys,zs,ws) { v.x=xs; v.y=ys; v.z=zs; v.w=ws; }

#define SET2V(v1,v2) { v1.x=v2.x; v1.y=v2.y; }
#define SET3V(v1,v2) { v1.x=v2.x; v1.y=v2.y; v1.z=v2.z; }
#define SET4V(v1,v2) { v1.x=v2.x; v1.y=v2.y; v1.z=v2.z; v1.w=v2.w; }

#define EQ2(v,xx,yy)       ( v.x==xx && v.y==yy )
#define EQ3(v,xx,yy,zz)    ( v.x==xx && v.y==yy && v.z==zz )
#define EQ4(v,xx,yy,zz,ww) ( v.x==xx && v.y==yy && v.z==zz && v.w==ww )

#define ISZERO2(v) ( v.x==0.0f && v.y==0.0f )
#define ISZERO3(v) ( v.x==0.0f && v.y==0.0f && v.z==0.0f)
#define ISZERO4(v) ( v.x==0.0f && v.y==0.0f && v.z==0.0f && v.w==0.0f )

#define EQ2V(v1,v2) ( v1.x==v2.x && v1.y==v2.y )
#define EQ3V(v1,v2) ( v1.x==v2.x && v1.y==v2.y && v1.z==v2.z )
#define EQ4V(v1,v2) ( v1.x==v2.x && v1.y==v2.y && v1.z==v2.z && v1.w==v2.w )

#define ADD2(v,xx,yy)       { v.x+=xx; v.y+=yy; }
#define ADD3(v,xx,yy,zz)    { v.x+=xx; v.y+=yy; v.z+=zz; }
#define ADD4(v,xx,yy,zz,ww) { v.x+=xx; v.y+=yy; v.z+=zz; v.w+=ww; }

#define ADD2V(v1,v2) { v1.x+=v2.x; v1.y+=v2.y; }
#define ADD3V(v1,v2) { v1.x+=v2.x; v1.y+=v2.y; v1.z+=v2.z; }
#define ADD4V(v1,v2) { v1.x+=v2.x; v1.y+=v2.y; v1.z+=v2.z; v1.w+=v2.w; }

#define SUB2(v,xx,yy)       { v.x-=xx; v.y-=yy; }
#define SUB3(v,xx,yy,zz)    { v.x-=xx; v.y-=yy; v.z-=zz; }
#define SUB4(v,xx,yy,zz,ww) { v.x-=xx; v.y-=yy; v.z-=zz; v.w-=v2.w; }

#define SUB2V(v1,v2) { v1.x-=v2.x; v1.y-=v2.y; }
#define SUB3V(v1,v2) { v1.x-=v2.x; v1.y-=v2.y; v1.z-=v2.z; }
#define SUB4V(v1,v2) { v1.x-=v2.x; v1.y-=v2.y; v1.z-=v2.z; v1.w-=v2.w; }

#define MUL2(v,f) { v.x*=f; v.y*=f; }
#define MUL3(v,f) { v.x*=f; v.y*=f; v.z*=z; }
#define MUL4(v,f) { v.x*=f; v.y*=f; v.z*=z; v.w*=w; }

#define DIV2(v,f) { v.x/=f; v.y/=f; }
#define DIV3(v,f) { v.x/=f; v.y/=f; v.z/=z; }
#define DIV4(v,f) { v.x/=f; v.y/=f; v.z/=z; v.w/=w; }

#define ABS2(v) { v.x=SH_ABS(v.x); v.y=SH_ABS(v.y); }
#define ABS3(v) { v.x=SH_ABS(v.x); v.y=SH_ABS(v.y); v.z=SH_ABS(v.z); }
#define ABS4(v) { v.x=SH_ABS(v.x); v.y=SH_ABS(v.y); v.z=SH_ABS(v.z); v.w=SH_ABS(v.w); }

#define NORMSQ2(v) (v.x*v.x + v.y*v.y)
#define NORMSQ3(v) (v.x*v.x + v.y*v.y + v.z*v.z)
#define NORMSQ4(v) (v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w)

#define NORM2(v) SH_SQRT(NORMSQ2(v))
#define NORM3(v) SH_SQRT(NORMSQ3(v))
#define NORM4(v) SH_SQRT(NORMSQ4(v))

#define NORMALIZE2(v) { SHfloat n=NORM2(v); v.x/=n; v.y/=n; }
#define NORMALIZE3(v) { SHfloat n=NORM3(v); v.x/=n; v.y/=n; v.z/=n; }
#define NORMALIZE4(v) { SHfloat n=NORM4(v); v.x/=n; v.y/=n; v.z/=n; v.w/=w; }

#define DOT2(v1,v2) (v1.x*v2.x + v1.y*v2.y)
#define DOT3(v1,v2) (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z)
#define DOT4(v1,v2) (v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w)

#define CROSS2(v1,v2) (v1.x*v2.y - v2.x*v1.y)

#define ANGLE2(v1,v2) (SH_ACOS( DOT2(v1,v2) / (NORM2(v1)*NORM2(v2)) ))
#define ANGLE2N(v1,v2) (SH_ACOS( DOT2(v1,v2) ))

#define OFFSET2V(v, o, s) { v.x += o.x*s; v.y += o.y*s; }
#define OFFSET3V(v, o, s) { v.x += o.x*s; v.y += o.y*s; v.z += o.z*s; }
#define OFFSET4V(v, o, s) { v.x += o.x*s; v.y += o.y*s; v.z += o.z*s; v.w += o.w*s; }

/*-----------------------------------------------------
 * Macros for matrix operations
 *-----------------------------------------------------*/

#define SETMAT(mat, m00, m01, m02, m10, m11, m12, m20, m21, m22) { \
mat.m[0][0] = m00; mat.m[0][1] = m01; mat.m[0][2] = m02; \
  mat.m[1][0] = m10; mat.m[1][1] = m11; mat.m[1][2] = m12; \
  mat.m[2][0] = m20; mat.m[2][1] = m21; mat.m[2][2] = m22; }

#define SETMATMAT(m1, m2) { \
int i,j; \
  for(i=0;i<3;i++) \
  for(j=0;j<3;j++) \
    m1.m[i][j] = m2.m[i][j]; }

#define MULMATS(mat, s) { \
int i,j; \
  for(i=0;i<3;i++) \
  for(j=0;j<3;j++) \
    mat.m[i][j] *= s; }

#define DIVMATS(mat, s) { \
int i,j; \
  for(i=0;i<3;i++) \
  for(j=0;j<3;j++) \
    mat.m[i][j] /= s; }

#define MULMATMAT(m1, m2, mout) { \
int i,j; \
  for(i=0;i<3;i++) \
  for(j=0;j<3;j++) \
    mout.m[i][j] = \
      m1.m[i][0] * m2.m[0][j] + \
      m1.m[i][1] * m2.m[1][j] + \
      m1.m[i][2] * m2.m[2][j]; }

#define IDMAT(mat) SETMAT(mat, 1,0,0, 0,1,0, 0,0,1)

#define TRANSLATEMATL(mat, tx, ty) { \
SHMatrix3x3 trans,temp; \
  SETMAT(trans, 1,0,tx, 0,1,ty, 0,0,1); \
  MULMATMAT(trans, mat, temp); \
  SETMATMAT(mat, temp); }

#define TRANSLATEMATR(mat, tx, ty) { \
SHMatrix3x3 trans,temp; \
  SETMAT(trans, 1,0,tx, 0,1,ty, 0,0,1); \
  MULMATMAT(mat, trans, temp); \
  SETMATMAT(mat, temp); }

#define SCALEMATL(mat, sx, sy) { \
SHMatrix3x3 scale, temp; \
  SETMAT(scale, sx,0,0, 0,sy,0, 0,0,1); \
  MULMATMAT(scale, mat, temp); \
  SETMATMAT(mat, temp); }

#define SCALEMATR(mat, sx, sy) { \
SHMatrix3x3 scale, temp; \
  SETMAT(scale, sx,0,0, 0,sy,0, 0,0,1); \
  MULMATMAT(mat, scale, temp); \
  SETMATMAT(mat, temp); }

#define SHEARMATL(mat, shx, shy) {\
SHMatrix3x3 shear, temp;\
  SETMAT(shear, 1,shx,0, shy,1,0, 0,0,1); \
  MULMATMAT(shear, mat, temp); \
  SETMATMAT(mat, temp); }

#define SHEARMATR(mat, shx, shy) {\
SHMatrix3x3 shear, temp;\
  SETMAT(shear, 1,shx,0, shy,1,0, 0,0,1); \
  MULMATMAT(mat, shear, temp); \
  SETMATMAT(mat, temp); }

#define ROTATEMATL(mat, a) { \
SHfloat cosa=SH_COS(a), sina=SH_SIN(a); \
  SHMatrix3x3 rot, temp; \
  SETMAT(rot, cosa,-sina,0, sina,cosa,0, 0,0,1); \
  MULMATMAT(rot, mat, temp); \
  SETMATMAT(mat, temp); }

#define ROTATEMATR(mat, a) { \
SHfloat cosa=SH_COS(a), sina=SH_SIN(a); \
  SHMatrix3x3 rot, temp; \
  SETMAT(rot, cosa,-sina,0, sina,cosa,0, 0,0,1); \
  MULMATMAT(mat, rot, temp); \
  SETMATMAT(mat, temp); }

#define TRANSFORM2TO(v, mat, vout) { \
vout.x = v.x*mat.m[0][0] + v.y*mat.m[0][1] + 1*mat.m[0][2]; \
  vout.y = v.x*mat.m[1][0] + v.y*mat.m[1][1] + 1*mat.m[1][2]; }

#define TRANSFORM2(v, mat) { \
SHVector2 temp; TRANSFORM2TO(v, mat, temp); v = temp; }

#define TRANSFORM2DIRTO(v, mat, vout) { \
vout.x = v.x*mat.m[0][0] + v.y*mat.m[0][1]; \
  vout.y = v.x*mat.m[1][0] + v.y*mat.m[1][1]; }

#define TRANSFORM2DIR(v, mat) { \
SHVector2 temp; TRANSFORM2DIRTO(v, mat, temp); v = temp; }


/*--------------------------------------------------------
 * Additional functions
 *--------------------------------------------------------- */

void shMatrixToGL(SHMatrix3x3 *m, SHfloat mgl[16]);

SHint shInvertMatrix(SHMatrix3x3 *m, SHMatrix3x3 *mout);

SHfloat shVectorOrientation(SHVector2 *v);

int shLineLineXsection(SHVector2 *o1, SHVector2 *v1,
                       SHVector2 *o2, SHVector2 *v2,
                       SHVector2 *xsection);

#endif/* __SHVECTORS_H */
