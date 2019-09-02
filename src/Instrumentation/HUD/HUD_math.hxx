/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 1998,2002  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net
*/

#ifndef _HUD_MATH_HXX
#define _HUD_MATH_HXX

#include <cmath>

typedef double dVec3[3];
typedef double dVec4[4];
typedef double dMat4[4][4];

inline void dScaleVec3(dVec3 dst, const double s)
{
    dst[0] *= s;
    dst[1] *= s;
    dst[2] *= s;
}

inline void dAddVec3(dVec3 dst, const dVec3 src)
{
    dst[0] += src[0];
    dst[1] += src[1];
    dst[2] += src[2];
}

inline void dCopyVec3(dVec3 dst, const dVec3 src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

inline double dSqrt(const double x) { return sqrt(x); }

inline double dScalarProductVec3(const dVec3 a, const dVec3 b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline double dLengthVec3(dVec3 const src)
{
    return dSqrt(dScalarProductVec3(src, src));
}

inline void dMultMat4(dMat4 dst, const dMat4 m1, const dMat4 m2)
{
    for (int j = 0; j < 4; j++) {
        dst[0][j] = m2[0][0] * m1[0][j] +
                    m2[0][1] * m1[1][j] +
                    m2[0][2] * m1[2][j] +
                    m2[0][3] * m1[3][j];

        dst[1][j] = m2[1][0] * m1[0][j] +
                    m2[1][1] * m1[1][j] +
                    m2[1][2] * m1[2][j] +
                    m2[1][3] * m1[3][j];

        dst[2][j] = m2[2][0] * m1[0][j] +
                    m2[2][1] * m1[1][j] +
                    m2[2][2] * m1[2][j] +
                    m2[2][3] * m1[3][j];

        dst[3][j] = m2[3][0] * m1[0][j] +
                    m2[3][1] * m1[1][j] +
                    m2[3][2] * m1[2][j] +
                    m2[3][3] * m1[3][j];
    }
}

inline void dCopyVec4(dVec4 dst, const dVec4 src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
}

inline void dCopyMat4(dMat4 dst, const dMat4 src)
{
    dCopyVec4(dst[0], src[0]);
    dCopyVec4(dst[1], src[1]);
    dCopyVec4(dst[2], src[2]);
    dCopyVec4(dst[3], src[3]);
}

inline void dPostMultMat4(dMat4 dst, const dMat4 src)
{
    dMat4 mat;
    dMultMat4(mat, src, dst);
    dCopyMat4(dst, mat);
}

#endif // _HUD_MATH_HXX