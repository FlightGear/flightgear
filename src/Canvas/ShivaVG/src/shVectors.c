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

#include "shVectors.h"

#define _ITEM_T SHVector2
#define _ARRAY_T SHVector2Array
#define _FUNC_T shVector2Array
#define _COMPARE_T(v1,v2) EQ2V(v1,v2)
#define _ARRAY_DEFINE
#include "shArrayBase.h"

void SHVector2_ctor(SHVector2 *v) {
  v->x=0.0f; v->y=0.0f;
}

void SHVector2_dtor(SHVector2 *v) {
}

void SHVector3_ctor(SHVector3 *v) {
  v->x=0.0f; v->y=0.0f; v->z=0.0f;
}

void SHVector3_dtor(SHVector3 *v) {
}

void SHVector4_ctor(SHVector4 *v) {
  v->x=0.0f; v->y=0.0f; v->z=0.0f; v->w=0.0f;
}

void SHVector4_dtor(SHVector4 *v) {
}

void SHRectangle_ctor(SHRectangle *r) {
  r->x=0.0f; r->y=0.0f; r->w=0.0f; r->h=0.0f;
}

void SHRectangle_dtor(SHRectangle *r) {
}

void shRectangleSet(SHRectangle *r, SHfloat x,
                    SHfloat y, SHfloat w, SHfloat h)
{  
  r->x=x; r->y=y; r->w=w; r->h=h;
}

void SHMatrix3x3_ctor(SHMatrix3x3 *mt)
{
  IDMAT((*mt));
}

void SHMatrix3x3_dtor(SHMatrix3x3 *mt)
{
}

void shMatrixToGL(SHMatrix3x3 *m, SHfloat mgl[16])
{
  /* When 2D vectors are specified OpenGL defaults Z to 0.0f so we
     have to shift the third column of our 3x3 matrix to right */
  mgl[0] = m->m[0][0]; mgl[4] = m->m[0][1]; mgl[8]  = 0.0f; mgl[12] = m->m[0][2];
  mgl[1] = m->m[1][0]; mgl[5] = m->m[1][1]; mgl[9]  = 0.0f; mgl[13] = m->m[1][2];
  mgl[2] = m->m[2][0]; mgl[6] = m->m[2][1]; mgl[10] = 1.0f; mgl[14] = m->m[2][1];
  mgl[3] = 0.0f;       mgl[7] = 0.0f;       mgl[11] = 0.0f; mgl[15] = 1.0f;
}

int shInvertMatrix(SHMatrix3x3 *m, SHMatrix3x3 *mout)
{
  /* Calculate determinant */
  SHfloat D0 = m->m[1][1]*m->m[2][2] - m->m[2][1]*m->m[1][2];
  SHfloat D1 = m->m[2][0]*m->m[1][2] - m->m[1][0]*m->m[2][2];
  SHfloat D2 = m->m[1][0]*m->m[2][1] - m->m[2][0]*m->m[1][1]; 
  SHfloat D = m->m[0][0]*D0 + m->m[0][1]*D1 + m->m[0][2]*D2;
  
  /* Check if singular */
  if( D == 0.0f ) return 0;
  D = 1.0f / D;
  
  /* Calculate inverse */
  mout->m[0][0] = D * D0;
  mout->m[1][0] = D * D1;
  mout->m[2][0] = D * D2;
  mout->m[0][1] = D * (m->m[2][1]*m->m[0][2] - m->m[0][1]*m->m[2][2]);
  mout->m[1][1] = D * (m->m[0][0]*m->m[2][2] - m->m[2][0]*m->m[0][2]);
  mout->m[2][1] = D * (m->m[2][0]*m->m[0][1] - m->m[0][0]*m->m[2][1]);
  mout->m[0][2] = D * (m->m[0][1]*m->m[1][2] - m->m[1][1]*m->m[0][2]);
  mout->m[1][2] = D * (m->m[1][0]*m->m[0][2] - m->m[0][0]*m->m[1][2]);
  mout->m[2][2] = D * (m->m[0][0]*m->m[1][1] - m->m[1][0]*m->m[0][1]);
  
  return 1;
}

SHfloat shVectorOrientation(SHVector2 *v) {
  SHfloat norm = (SHfloat)NORM2((*v));
  SHfloat cosa = v->x/norm;
  SHfloat sina = v->y/norm;
  return (SHfloat)(sina>=0 ? SH_ACOS(cosa) : 2*PI-SH_ACOS(cosa));
}

int shLineLineXsection(SHVector2 *o1, SHVector2 *v1,
                       SHVector2 *o2, SHVector2 *v2,
                       SHVector2 *xsection)
{
  SHfloat rightU = o2->x - o1->x;
  SHfloat rightD = o2->y - o1->y;
  
  SHfloat D  = v1->x  * (-v2->y) - v1->y   * (-v2->x);
  SHfloat DX = rightU * (-v2->y) - rightD * (-v2->x);
/*SHfloat DY = v1.x   * rightD  - v1.y   * rightU;*/
  
  SHfloat t1 = DX / D;
  
  if (D == 0.0f)
    return 0;
  
  xsection->x = o1->x + t1*v1->x;
  xsection->y = o1->y + t1*v1->y;
  return 1;
}
