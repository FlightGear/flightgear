// pt_lights.cxx -- build a 'directional' light on the fly
//
// Written by Curtis Olson, started March 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <plib/sg.h>

#include "newmat.hxx"
#include "matlib.hxx"

#include "pt_lights.hxx"


// Generate a directional light
ssgLeaf *gen_directional_light( sgVec3 pt, sgVec3 dir, sgVec3 up ) {

    // calculate a vector perpendicular to dir and up
    sgVec3 perp;
    sgVectorProductVec3( perp, dir, up );

    ssgVertexArray   *vl = new ssgVertexArray( 3 );
    ssgNormalArray   *nl = new ssgNormalArray( 1 );
    ssgColourArray   *cl = new ssgColourArray( 1 );

    // front face
    sgVec3 tmp3;
    sgCopyVec3( tmp3, pt );
    vl->add( tmp3 );
    sgAddVec3( tmp3, up );
    vl->add( tmp3 );
    sgAddVec3( tmp3, perp );
    vl->add( tmp3 );

    nl->add( dir );
    nl->add( dir );
    nl->add( dir );

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );
    sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
    cl->add( color );
    cl->add( color );

    /*
    // temporarily do back face
    sgCopyVec3( tmp3, pt );
    vl->add( tmp3 );
    sgAddVec3( tmp3, up );
    vl->add( tmp3 );
    sgAddVec3( tmp3, perp );
    vl->add( tmp3 );

    sgNegateVec3( dir );
    nl->add( dir );
    nl->add( dir );
    nl->add( dir );

    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );
    sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
    cl->add( color );
    cl->add( color );
    */

    /* ssgTexCoordArray *tl = new ssgTexCoordArray( 4 );
    sgVec2 tmp2;
    sgSetVec2( tmp2, 0.0, 0.0 );
    tl->add( tmp2 );
    sgSetVec2( tmp2, 1.0, 0.0 );
    tl->add( tmp2 );
    sgSetVec2( tmp2, 1.0, 1.0 );
    tl->add( tmp2 );
    sgSetVec2( tmp2, 0.0, 1.0 );
    tl->add( tmp2 );
    */

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_TRIANGLES, vl, nl, NULL, cl );

    FGNewMat *newmat = material_lib.find( "RUNWAY_LIGHTS" );
    // FGNewMat *newmat = material_lib.find( "IrrCropPastureCover" );
    leaf->setState( newmat->get_state() );

    return leaf;
}


// Generate a directional light
ssgLeaf *gen_normal_line( sgVec3 pt, sgVec3 dir, sgVec3 up ) {

    ssgVertexArray   *vl = new ssgVertexArray( 3 );
    ssgNormalArray   *nl = new ssgNormalArray( 1 );
    ssgColourArray   *cl = new ssgColourArray( 1 );

    sgVec3 tmp3;
    sgCopyVec3( tmp3, pt );
    vl->add( tmp3 );
    sgAddVec3( tmp3, dir );
    vl->add( tmp3 );

    sgVec4 color;
    sgSetVec4( color, 1.0, 0.0, 0.0, 1.0 );
    cl->add( color );
    cl->add( color );

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_LINES, vl, NULL, NULL, cl );

    FGNewMat *newmat = material_lib.find( "GROUND_LIGHTS" );
    leaf->setState( newmat->get_state() );

    return leaf;
}


ssgBranch *gen_directional_lights( const point_list &nodes,
                                   const point_list &normals,
                                   const int_list &pnt_i,
                                   const int_list &nml_i,
                                   sgVec3 up )
{
    ssgBranch *result = new ssgBranch;

    sgVec3 nup;
    sgNormalizeVec3( nup, up );

    unsigned int i;
    sgVec3 pt, normal;
    for ( i = 0; i < pnt_i.size(); ++i ) {
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        sgSetVec3( normal, normals[nml_i[i]][0], normals[nml_i[i]][1],
                   normals[nml_i[i]][2] );
        ssgLeaf *light = gen_directional_light( pt, normal, nup );
        result->addKid( light );
        // light = gen_normal_line( pt, normal, nup );
        // result->addKid( light );
    }

    return result;
}
