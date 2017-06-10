// WakeMesh.hxx -- Mesh for the computation of a wing wake.
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

#ifndef _FG_WAKEMESH_HXX
#define _FG_WAKEMESH_HXX

#include "AeroElement.hxx"

class WakeMesh : public SGReferenced {
public:
    WakeMesh(double _span, double _chord);
    virtual ~WakeMesh();
    double computeAoA(double vel, double rho, double weight);
    SGVec3d getInducedVelocityAt(const SGVec3d& at) const;

#ifndef FG_TESTLIB
protected:
#endif
    int nelm;
    double span, chord;
    std::vector<AeroElement_ptr> elements;
    double **influenceMtx, **Gamma;
};

typedef SGSharedPtr<WakeMesh> WakeMesh_ptr;

#endif
