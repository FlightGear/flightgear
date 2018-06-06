// AeroElement.hxx -- Mesh element for the computation of a wing wake.
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

#ifndef _FG_AEROELEMENT_HXX
#define _FG_AEROELEMENT_HXX

#include <simgear/math/SGMathFwd.hxx>


class AeroElement : public SGReferenced {
public:
    AeroElement(const SGVec3d& n1, const SGVec3d& n2, const SGVec3d& n3,
                const SGVec3d& n4);
    const SGVec3d& getNormal(void) const { return normal; }
    const SGVec3d& getCollocationPoint(void) const { return collocationPt; }
    SGVec3d getBoundVortex(void) const { return p2 - p1; }
    SGVec3d getBoundVortexMidPoint(void) const { return 0.5*(p1+p2); }
    SGVec3d getInducedVelocity(const SGVec3d& p) const;
private:
    SGVec3d vortexInducedVel(const SGVec3d& p, const SGVec3d& n1,
                             const SGVec3d& n2) const;
    SGVec3d semiInfVortexInducedVel(const SGVec3d& point, const SGVec3d& vEnd,
                                    const SGVec3d& vDir) const;
    SGVec3d p1, p2, normal, collocationPt;
};

typedef SGSharedPtr<AeroElement> AeroElement_ptr;

#endif
