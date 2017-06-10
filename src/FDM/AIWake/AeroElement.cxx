// AeroElement.cxx -- Mesh element for the computation of a wing wake.
//
// Written by Bertrand Coconnier, started March 2017.
//
// Copyright (C) 2017  Bertrand Coconnier  - bcoconni@users.sf.net
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id$

#include <cmath>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>
#include "AeroElement.hxx"

AeroElement::AeroElement(const SGVec3d& n1, const SGVec3d& n2,
                         const SGVec3d& n3, const SGVec3d& n4)
{
    p1 = 0.75*n2 + 0.25*n1;
    p2 = 0.75*n3 + 0.25*n4;
    normal = normalize(cross(n3 - n1, n2 - n4));
    collocationPt = 0.375*(n1 + n4) + 0.125*(n2 + n3);
}

SGVec3d AeroElement::vortexInducedVel(const SGVec3d& p, const SGVec3d& n1,
                                      const SGVec3d& n2) const
{
    SGVec3d r0 = n2-n1, r1 = p-n1, r2 = p-n2;
    SGVec3d v = cross(r1, r2);
    double vSqrNorm = dot(v, v);
    double r1SqrNorm = dot(r1, r1);
    double r2SqrNorm = dot(r2, r2);

    if ((vSqrNorm < 1E-6) || (r1SqrNorm < 1E-6) || (r2SqrNorm < 1E-6))
        return SGVec3d::zeros();

    r1 /= sqrt(r1SqrNorm);
    r2 /= sqrt(r2SqrNorm);
    v *= dot(r0, r1-r2) / (4.0*M_PI*vSqrNorm);

    return v;
}

SGVec3d AeroElement::semiInfVortexInducedVel(const SGVec3d& point,
                                             const SGVec3d& vEnd,
                                             const SGVec3d& vDir) const
{
    SGVec3d r = point-vEnd;
    double rSqrNorm = dot(r, r);
    double denom = rSqrNorm - dot(r, vDir)*sqrt(rSqrNorm);

    if (fabs(denom) < 1E-6)
        return SGVec3d::zeros();

    SGVec3d v = cross(r, vDir);
    v /= 4*M_PI*denom;

    return v;
}

SGVec3d AeroElement::getInducedVelocity(const SGVec3d& p) const
{
    const SGVec3d w(-1.,0.,0.);
    SGVec3d v = semiInfVortexInducedVel(p, p1, w);
    v -= semiInfVortexInducedVel(p, p2, w);
    v += vortexInducedVel(p, p1, p2);

    return v;
}
