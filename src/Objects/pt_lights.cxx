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

#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>

#include "pt_lights.hxx"


// strobe pre-draw (we want a larger point size)
static int StrobePreDraw( ssgEntity *e ) {
    glPushAttrib( GL_POINT_BIT );
    glPointSize(4.0);
    glEnable(GL_POINT_SMOOTH);

    return true;
}

// strobe post-draw (we want a larger point size)
static int StrobePostDraw( ssgEntity *e ) {
    glPopAttrib();

    return true;
}


// Generate a directional light
ssgLeaf *gen_directional_light( sgVec3 pt, sgVec3 dir, sgVec3 up, 
                                const SGMaterial *mat ) {

    // calculate a vector perpendicular to dir and up
    sgVec3 perp;
    sgVectorProductVec3( perp, dir, up );

    ssgVertexArray   *vl = new ssgVertexArray( 3 );
    ssgNormalArray   *nl = new ssgNormalArray( 3 );
    ssgColourArray   *cl = new ssgColourArray( 3 );

    // front face
    sgVec3 tmp3;
    sgCopyVec3( tmp3, pt );
    vl->add( tmp3 );
    sgAddVec3( tmp3, up );
    vl->add( tmp3 );
    sgAddVec3( tmp3, perp );
    vl->add( tmp3 );
    // sgSubVec3( tmp3, up );
    // vl->add( tmp3 );

    nl->add( dir );
    nl->add( dir );
    nl->add( dir );
    // nl->add( dir );

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );
    sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
    cl->add( color );
    cl->add( color );
    // cl->add( color );

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
    tl->add( tmp2 ); */

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_TRIANGLES, vl, nl, NULL, cl );

    if ( mat != NULL ) {
        leaf->setState( mat->get_state() );
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Warning: mat = NULL" );
    }

    return leaf;
}


static void calc_center_point( const point_list &nodes,
                               const int_list &pnt_i,
                               sgVec3 result ) {
    sgVec3 pt;
    sgSetVec3( pt, nodes[pnt_i[0]][0], nodes[pnt_i[0]][1], nodes[pnt_i[0]][2] );

    double minx = pt[0];
    double maxx = pt[0];
    double miny = pt[1];
    double maxy = pt[1];
    double minz = pt[2];
    double maxz = pt[2];

    for ( unsigned int i = 0; i < pnt_i.size(); ++i ) {
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        if ( pt[0] < minx ) { minx = pt[0]; }
        if ( pt[0] > maxx ) { minx = pt[0]; }
        if ( pt[1] < miny ) { miny = pt[1]; }
        if ( pt[1] > maxy ) { miny = pt[1]; }
        if ( pt[2] < minz ) { minz = pt[2]; }
        if ( pt[2] > maxz ) { minz = pt[2]; }
    }

    sgSetVec3( result, (minx + maxx) / 2.0, (miny + maxy) / 2.0,
               (minz + maxz) / 2.0 );
}


static ssgTransform *gen_dir_light_group( const point_list &nodes,
                                          const point_list &normals,
                                          const int_list &pnt_i,
                                          const int_list &nml_i,
                                          const SGMaterial *mat,
                                          sgVec3 up, bool vertical = false )
{
    sgVec3 center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;


    // find a vector perpendicular to the normal.
    sgVec3 perp1;
    if ( !vertical ) {
        // normal isn't vertical so we can use up as our first vector
        sgNormalizeVec3( perp1, up );
    } else {
        // normal is vertical so we have to work a bit harder to
        // determine our first vector
        sgVec3 pt1, pt2;
        sgSetVec3( pt1, nodes[pnt_i[0]][0], nodes[pnt_i[0]][1],
                   nodes[pnt_i[0]][2] );
        sgSetVec3( pt2, nodes[pnt_i[1]][0], nodes[pnt_i[1]][1],
                   nodes[pnt_i[1]][2] );

        sgSubVec3( perp1, pt2, pt1 );
        sgNormalizeVec3( perp1 );
    }

    ssgVertexArray *vl = new ssgVertexArray( 3 * pnt_i.size() );
    ssgNormalArray *nl = new ssgNormalArray( 3 * pnt_i.size() );
    ssgColourArray *cl = new ssgColourArray( 3 * pnt_i.size() );

    unsigned int i;
    sgVec3 pt, normal;
    for ( i = 0; i < pnt_i.size(); ++i ) {
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        sgSubVec3( pt, center );
        sgSetVec3( normal, normals[nml_i[i]][0], normals[nml_i[i]][1],
                   normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        sgVec3 perp2;
        sgVectorProductVec3( perp2, normal, perp1 );

        // front face
        sgVec3 tmp3;
        sgCopyVec3( tmp3, pt );
        vl->add( tmp3 );
        sgAddVec3( tmp3, perp1 );
        vl->add( tmp3 );
        sgAddVec3( tmp3, perp2 );
        vl->add( tmp3 );
        // sgSubVec3( tmp3, perp1 );
        // vl->add( tmp3 );

        nl->add( normal );
        nl->add( normal );
        nl->add( normal );
        // nl->add( normal );

        sgVec4 color;
        sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
        cl->add( color );
        sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
        cl->add( color );
        cl->add( color );
        // cl->add( color );
    }

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_TRIANGLES, vl, nl, NULL, cl );

    if ( mat != NULL ) {
        leaf->setState( mat->get_state() );
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "Warning: material = NULL" );
    }

    // put an LOD on each lighting component
    ssgRangeSelector *lod = new ssgRangeSelector;
    lod->setRange( 0, SG_ZERO );
    lod->setRange( 1, 20000 );
    lod->addKid( leaf );

    // create the transformation.
    sgCoord coord;
    sgSetCoord( &coord, center[0], center[1], center[2], 0.0, 0.0, 0.0 );
    ssgTransform *trans = new ssgTransform;
    trans->setTransform( &coord );
    trans->addKid( lod );

    return trans;
}


static ssgTransform *gen_reil_lights( const point_list &nodes,
                                      const point_list &normals,
                                      const int_list &pnt_i,
                                      const int_list &nml_i,
                                      SGMaterialLib *matlib,
                                      sgVec3 up )
{
    sgVec3 center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    sgVec3 nup;
    sgNormalizeVec3( nup, up );

    ssgVertexArray   *vl = new ssgVertexArray( 3 * pnt_i.size() );
    ssgNormalArray   *nl = new ssgNormalArray( 3 * pnt_i.size() );
    ssgColourArray   *cl = new ssgColourArray( 3 * pnt_i.size() );

    unsigned int i;
    sgVec3 pt, normal;
    for ( i = 0; i < pnt_i.size(); ++i ) {
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        sgSubVec3( pt, center );
        sgSetVec3( normal, normals[nml_i[i]][0], normals[nml_i[i]][1],
                   normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        sgVec3 perp;
        sgVectorProductVec3( perp, normal, nup );

        // front face
        sgVec3 tmp3;
        sgCopyVec3( tmp3, pt );
        vl->add( tmp3 );
        sgAddVec3( tmp3, nup );
        vl->add( tmp3 );
        sgAddVec3( tmp3, perp );
        vl->add( tmp3 );
        // sgSubVec3( tmp3, nup );
        // vl->add( tmp3 );

        nl->add( normal );
        nl->add( normal );
        nl->add( normal );
        // nl->add( normal );

        sgVec4 color;
        sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
        cl->add( color );
        sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
        cl->add( color );
        cl->add( color );
        // cl->add( color );
    }

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_TRIANGLES, vl, nl, NULL, cl );

    SGMaterial *mat = matlib->find( "RWY_WHITE_LIGHTS" );

    if ( mat != NULL ) {
        leaf->setState( mat->get_state() );
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't find material = RWY_WHITE_LIGHTS" );
    }

    leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
    leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

    ssgTimedSelector *reil = new ssgTimedSelector;

    // need to add this twice to work around an ssg bug
    reil->addKid( leaf );
    reil->addKid( leaf );

    reil->setDuration( 60 );
    reil->setLimits( 0, 2 );
    reil->setMode( SSG_ANIM_SHUTTLE );
    reil->control( SSG_ANIM_START );
   
    // put an LOD on each lighting component
    ssgRangeSelector *lod = new ssgRangeSelector;
    lod->setRange( 0, SG_ZERO );
    lod->setRange( 1, 12000 );
    lod->addKid( reil );

    // create the transformation.
    sgCoord coord;
    sgSetCoord( &coord, center[0], center[1], center[2], 0.0, 0.0, 0.0 );
    ssgTransform *trans = new ssgTransform;
    trans->setTransform( &coord );
    trans->addKid( lod );

    return trans;
}


static ssgTransform *gen_odals_lights( const point_list &nodes,
                                       const point_list &normals,
                                       const int_list &pnt_i,
                                       const int_list &nml_i,
                                       SGMaterialLib *matlib,
                                       sgVec3 up )
{
    sgVec3 center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    ssgTimedSelector *odals = new ssgTimedSelector;

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );

    // we don't want directional lights here
    SGMaterial *mat = matlib->find( "GROUND_LIGHTS" );
    if ( mat == NULL ) {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't material = GROUND_LIGHTS" );
    }

    // center line strobes
    int i;
    sgVec3 pt;
    for ( i = (int)pnt_i.size() - 1; i >= 2; --i ) {
        ssgVertexArray   *vl = new ssgVertexArray( 1 );
        ssgColourArray   *cl = new ssgColourArray( 1 );
     
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        sgSubVec3( pt, center );
        vl->add( pt );

        cl->add( color );

        ssgLeaf *leaf = 
            new ssgVtxTable ( GL_POINTS, vl, NULL, NULL, cl );

        leaf->setState( mat->get_state() );
        leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
        leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

        odals->addKid( leaf );
    }

    // runway end strobes
    ssgVertexArray   *vl = new ssgVertexArray( 2 );
    ssgColourArray   *cl = new ssgColourArray( 2 );
     
    sgSetVec3( pt, nodes[pnt_i[0]][0], nodes[pnt_i[0]][1],
               nodes[pnt_i[0]][2] );
    sgSubVec3( pt, center );
    vl->add( pt );
    cl->add( color );

    sgSetVec3( pt, nodes[pnt_i[1]][0], nodes[pnt_i[1]][1],
               nodes[pnt_i[1]][2] );
    sgSubVec3( pt, center );
    vl->add( pt );
    cl->add( color );

    ssgLeaf *leaf = 
        new ssgVtxTable ( GL_POINTS, vl, NULL, NULL, cl );

    leaf->setState( mat->get_state() );
    leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
    leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

    odals->addKid( leaf );

    // setup animition

    odals->setDuration( 10 );
    odals->setLimits( 0, pnt_i.size() - 1 );
    odals->setMode( SSG_ANIM_SHUTTLE );
    odals->control( SSG_ANIM_START );
   
    // put an LOD on each lighting component
    ssgRangeSelector *lod = new ssgRangeSelector;
    lod->setRange( 0, SG_ZERO );
    lod->setRange( 1, 12000 );
    lod->addKid( odals );

    // create the transformation.
    sgCoord coord;
    sgSetCoord( &coord, center[0], center[1], center[2], 0.0, 0.0, 0.0 );
    ssgTransform *trans = new ssgTransform;
    trans->setTransform( &coord );
    trans->addKid( lod );

    return trans;
}


static ssgTransform *gen_rabbit_lights( const point_list &nodes,
                                        const point_list &normals,
                                        const int_list &pnt_i,
                                        const int_list &nml_i,
                                        SGMaterialLib *matlib,
                                        sgVec3 up )
{
    sgVec3 center;
    calc_center_point( nodes, pnt_i, center );
    // cout << center[0] << "," << center[1] << "," << center[2] << endl;

    sgVec3 nup;
    sgNormalizeVec3( nup, up );

    ssgTimedSelector *rabbit = new ssgTimedSelector;

    SGMaterial *mat = matlib->find( "RWY_WHITE_LIGHTS" );
    if ( mat == NULL ) {
        SG_LOG( SG_TERRAIN, SG_ALERT,
                "Warning: can't material = RWY_WHITE_LIGHTS" );
    }

    int i;
    sgVec3 pt, normal;
    for ( i = (int)pnt_i.size() - 1; i >= 0; --i ) {
        ssgVertexArray   *vl = new ssgVertexArray( 3 );
        ssgNormalArray   *nl = new ssgNormalArray( 3 );
        ssgColourArray   *cl = new ssgColourArray( 3 );
     
        sgSetVec3( pt, nodes[pnt_i[i]][0], nodes[pnt_i[i]][1],
                   nodes[pnt_i[i]][2] );
        sgSubVec3( pt, center );

        sgSetVec3( normal, normals[nml_i[i]][0], normals[nml_i[i]][1],
                   normals[nml_i[i]][2] );

        // calculate a vector perpendicular to dir and up
        sgVec3 perp;
        sgVectorProductVec3( perp, normal, nup );

        // front face
        sgVec3 tmp3;
        sgCopyVec3( tmp3, pt );
        vl->add( tmp3 );
        sgAddVec3( tmp3, nup );
        vl->add( tmp3 );
        sgAddVec3( tmp3, perp );
        vl->add( tmp3 );
        // sgSubVec3( tmp3, nup );
        // vl->add( tmp3 );

        nl->add( normal );
        nl->add( normal );
        nl->add( normal );
        // nl->add( normal );

        sgVec4 color;
        sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
        cl->add( color );
        sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
        cl->add( color );
        cl->add( color );
        // cl->add( color );

        ssgLeaf *leaf = 
            new ssgVtxTable ( GL_TRIANGLES, vl, nl, NULL, cl );

        leaf->setState( mat->get_state() );
        leaf->setCallback( SSG_CALLBACK_PREDRAW, StrobePreDraw );
        leaf->setCallback( SSG_CALLBACK_POSTDRAW, StrobePostDraw );

        rabbit->addKid( leaf );
    }

    rabbit->setDuration( 10 );
    rabbit->setLimits( 0, pnt_i.size() - 1 );
    rabbit->setMode( SSG_ANIM_SHUTTLE );
    rabbit->control( SSG_ANIM_START );
   
    // put an LOD on each lighting component
    ssgRangeSelector *lod = new ssgRangeSelector;
    lod->setRange( 0, SG_ZERO );
    lod->setRange( 1, 12000 );
    lod->addKid( rabbit );

    // create the transformation.
    sgCoord coord;
    sgSetCoord( &coord, center[0], center[1], center[2], 0.0, 0.0, 0.0 );
    ssgTransform *trans = new ssgTransform;
    trans->setTransform( &coord );
    trans->addKid( lod );

    return trans;
}


#if 0 // debugging infrastructure
// Generate a normal line 
static ssgLeaf *gen_normal_line( SGMaterialLib *matlib,
                                 sgVec3 pt, sgVec3 dir, sgVec3 up ) {

    ssgVertexArray *vl = new ssgVertexArray( 3 );
    ssgColourArray *cl = new ssgColourArray( 3 );

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

    SGMaterial *mat = matlib->find( "GROUND_LIGHTS" );
    leaf->setState( mat->get_state() );

    return leaf;
}
#endif


ssgBranch *gen_directional_lights( const point_list &nodes,
                                   const point_list &normals,
                                   const int_list &pnt_i,
                                   const int_list &nml_i,
                                   SGMaterialLib *matlib,
                                   const string &material,
                                   sgVec3 up )
{
    sgVec3 nup;
    sgNormalizeVec3( nup, up );

    SGMaterial *mat = matlib->find( material );

    if ( material == "RWY_REIL_LIGHTS" ) {
        // cout << "found a reil" << endl;
        ssgTransform *reil = gen_reil_lights( nodes, normals, pnt_i, nml_i,
                                              matlib, up );
        return reil;
    } else if ( material == "RWY_ODALS_LIGHTS" ) {
        // cout << "found a odals" << endl;
        ssgTransform *odals = gen_odals_lights( nodes, normals, pnt_i, nml_i,
                                                matlib, up );
        return odals;
    } else if ( material == "RWY_SEQUENCED_LIGHTS" ) {
        // cout << "found a rabbit" << endl;
        ssgTransform *rabbit = gen_rabbit_lights( nodes, normals,
                                                  pnt_i, nml_i,
                                                  matlib, up );
        return rabbit;
    } else if ( material == "RWY_BLUE_TAXIWAY_LIGHTS" ) {
        ssgTransform *light_group = gen_dir_light_group( nodes, normals, pnt_i,
                                                         nml_i, mat, up,
                                                         true );
        return light_group;
    } else {
        ssgTransform *light_group = gen_dir_light_group( nodes, normals, pnt_i,
                                                         nml_i, mat, up );
        return light_group;
    }

    return NULL;
}
