// tileentry.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>
#include <Objects/apt_signs.hxx>
#include <Objects/matlib.hxx>
#include <Objects/newmat.hxx>
#include <Objects/obj.hxx>

#include "tileentry.hxx"
#include "tilemgr.hxx"


// Constructor
FGTileEntry::FGTileEntry ( const SGBucket& b )
    : ncount( 0 ),
      center( Point3D( 0.0 ) ),
      tile_bucket( b ),
      terra_transform( new ssgTransform ),
      rwy_lights_transform( new ssgTransform ),
      terra_range( new ssgRangeSelector ),
      rwy_lights_range( new ssgRangeSelector ),
      loaded(false),
      pending_models(0)
{
    nodes.clear();

    // update the contents
    // if ( vec3_ptrs.size() || vec2_ptrs.size() || index_ptrs.size() ) {
    //     SG_LOG( SG_TERRAIN, SG_ALERT, 
    //             "Attempting to overwrite existing or"
    //             << " not properly freed leaf data." );
    //     exit(-1);
    // }
}


// Destructor
FGTileEntry::~FGTileEntry () {
    // cout << "nodes = " << nodes.size() << endl;;
    // delete[] nodes;
}


#if 0
// Please keep this for reference.  We use Norman's optimized routine,
// but here is what the routine really is doing.
void
FGTileEntry::WorldCoordinate( sgCoord *obj_pos, Point3D center,
                              double lat, double lon, double elev, double hdg)
{
    // setup transforms
    Point3D geod( lon * SGD_DEGREES_TO_RADIANS,
                  lat * SGD_DEGREES_TO_RADIANS,
                  elev );
	
    Point3D world_pos = sgGeodToCart( geod );
    Point3D offset = world_pos - center;
	
    sgMat4 POS;
    sgMakeTransMat4( POS, offset.x(), offset.y(), offset.z() );

    sgVec3 obj_rt, obj_up;
    sgSetVec3( obj_rt, 0.0, 1.0, 0.0); // Y axis
    sgSetVec3( obj_up, 0.0, 0.0, 1.0); // Z axis

    sgMat4 ROT_lon, ROT_lat, ROT_hdg;
    sgMakeRotMat4( ROT_lon, lon, obj_up );
    sgMakeRotMat4( ROT_lat, 90 - lat, obj_rt );
    sgMakeRotMat4( ROT_hdg, hdg, obj_up );

    sgMat4 TUX;
    sgCopyMat4( TUX, ROT_hdg );
    sgPostMultMat4( TUX, ROT_lat );
    sgPostMultMat4( TUX, ROT_lon );
    sgPostMultMat4( TUX, POS );

    sgSetCoord( obj_pos, TUX );
}
#endif


// Norman's 'fast hack' for above
static void WorldCoordinate( sgCoord *obj_pos, Point3D center, double lat,
                             double lon, double elev, double hdg )
{
    double lon_rad = lon * SGD_DEGREES_TO_RADIANS;
    double lat_rad = lat * SGD_DEGREES_TO_RADIANS;
    double hdg_rad = hdg * SGD_DEGREES_TO_RADIANS;

    // setup transforms
    Point3D geod( lon_rad, lat_rad, elev );
	
    Point3D world_pos = sgGeodToCart( geod );
    Point3D offset = world_pos - center;

    sgMat4 mat;

    SGfloat sin_lat = (SGfloat)sin( lat_rad );
    SGfloat cos_lat = (SGfloat)cos( lat_rad );
    SGfloat cos_lon = (SGfloat)cos( lon_rad );
    SGfloat sin_lon = (SGfloat)sin( lon_rad );
    SGfloat sin_hdg = (SGfloat)sin( hdg_rad ) ;
    SGfloat cos_hdg = (SGfloat)cos( hdg_rad ) ;

    mat[0][0] =  cos_hdg * (SGfloat)sin_lat * (SGfloat)cos_lon - sin_hdg * (SGfloat)sin_lon;
    mat[0][1] =  cos_hdg * (SGfloat)sin_lat * (SGfloat)sin_lon + sin_hdg * (SGfloat)cos_lon;
    mat[0][2] =	-cos_hdg * (SGfloat)cos_lat;
    mat[0][3] =	 SG_ZERO;

    mat[1][0] = -sin_hdg * (SGfloat)sin_lat * (SGfloat)cos_lon - cos_hdg * (SGfloat)sin_lon;
    mat[1][1] = -sin_hdg * (SGfloat)sin_lat * (SGfloat)sin_lon + cos_hdg * (SGfloat)cos_lon;
    mat[1][2] =	 sin_hdg * (SGfloat)cos_lat;
    mat[1][3] =	 SG_ZERO;

    mat[2][0] = (SGfloat)cos_lat * (SGfloat)cos_lon;
    mat[2][1] = (SGfloat)cos_lat * (SGfloat)sin_lon;
    mat[2][2] =	(SGfloat)sin_lat;
    mat[2][3] =  SG_ZERO;

    mat[3][0] = offset.x();
    mat[3][1] = offset.y();
    mat[3][2] = offset.z();
    mat[3][3] = SG_ONE ;

    sgSetCoord( obj_pos, mat );
}


// recurse an ssg tree and call removeKid() on every node from the
// bottom up.  Leaves the original branch in existance, but empty so
// it can be removed by the calling routine.
static void my_remove_branch( ssgBranch * branch ) {
    for ( ssgEntity *k = branch->getKid( 0 );
	  k != NULL; 
	  k = branch->getNextKid() )
    {
	if ( k -> isAKindOf ( ssgTypeBranch() ) ) {
	    my_remove_branch( (ssgBranch *)k );
	    branch -> removeKid ( k );
	} else if ( k -> isAKindOf ( ssgTypeLeaf() ) ) {
	    branch -> removeKid ( k ) ;
	}
    }
}

// ADA
#define TEXRES_X 256
#define TEXRES_Y 256
unsigned char env_map[TEXRES_X][TEXRES_Y][4];
// SetColor & SetColor2 functions were provided by Christian Mayer (26 April 2001) - used without change
void setColor(float x, float y, float z, float angular_size, float r,
float g, float b, float a)
{
    //normalize
    float inv_length = 1.0 / sqrt(x*x + y*y + z*z);
    x *= inv_length; y *= inv_length; z *= inv_length;
  
    float cos_angular_size = cos(angular_size*(22.0/7.0)/180.0);

    for( int s = 0; s < TEXRES_X; s++) {
        for( int t = 0; t < TEXRES_Y; t++) {

            float s_2 = (float)s/TEXRES_X - 0.5; // centre of texture 0,0
            float t_2 = (float)t/TEXRES_Y - 0.5; // elev

            float rx, ry, rz;

            if ((1.0 - 4.0*s_2*s_2 - 4.0*t_2*t_2) >= 0.0) {
                // sphere
                float m = 4.0 * sqrt(1.0 - 4.0*s_2*s_2 - 4.0*t_2*t_2);
                rx = m * s_2;
                ry = m * t_2;
                rz = m*m / 8.0 - 1.0;
            } else {
                // singularity
                rx = 0.0;
                ry = 0.0;
                rz = -1.0;
            }

            float tx = rx;  //mirroring on the z=0 plane
            float ty = ry;  //assumes that the normal is allways
            float tz = -rz; //n(0.0, 0.0, 1.0)
         
            if ( cos_angular_size < (x*tx + y*ty + z*tz) ) {
                env_map[s][t][0] = (unsigned char) r * 255;  
                env_map[s][t][1] = (unsigned char) g * 255;
                env_map[s][t][2] = (unsigned char) b * 255;
                env_map[s][t][3] = (unsigned char) a * 255;
            }
        }
    }
}

// elevation_size, float azimuth_size are the *total* angular size of the light
void setColor2(float elevation_size,float azimuth_size, float r, float g, float b, float a)
{
    for( int s = 0; s < TEXRES_X; s++) {
        for( int t = 0; t < TEXRES_Y; t++) {
            float s_2 = (float)s/TEXRES_X - 0.5;
            float t_2 = (float)t/TEXRES_Y - 0.5;

            float rx, ry, rz;

            if ((1.0 - 4.0*s_2*s_2 - 4.0*t_2*t_2) >= 0.0) {

                float m = 4.0 * sqrt(1.0 - 4.0*s_2*s_2 - 4.0*t_2*t_2);
                rx = m * s_2;
                ry = m * t_2;
                rz = m*m / 8.0 - 1.0;
            } else {
                rx = 0.0;
                ry = 0.0;
                rz = -1.0;
            }

            float tx = rx;  //mirroring on the z=0 plane to reverse
            float ty = ry;  //OpenGLs automatic mirroring
            float tz = -rz; 

            //get elevation => project t onto the x-z-plane
            float tz_proj1 = tz / sqrt(tx*tx + tz*tz);
            float televation = acos( -tz_proj1 ) * 180.0 / 3.1415;

            //get azi => project t onto the y-z-plane
            float tz_proj2 = tz / sqrt(ty*ty + tz*tz);
            float tazimuth = acos( -tz_proj2 ) * 180.0 / 3.1415;

            //note televation and tazimuth are the angles *between* the
            //temporary vector and the normal (0,0,-1). They are *NOT*
            //the elevation and azimuth angles

            //square:
            //if (((elevation_size > televation) || (elevation_size < -televation)) &&
            //    ((azimuth_size   > tazimuth  ) || (azimuth_size   < -tazimuth  ))) 
            //elliptical
            if (((televation*televation) / (elevation_size*elevation_size / 4.0) +
                 (tazimuth  *tazimuth  ) / (azimuth_size  *azimuth_size   / 4.0)) <= 1.0)
            {
                env_map[s][t][0] = (unsigned char) r * 255;  
                env_map[s][t][1] = (unsigned char) g * 255;
                env_map[s][t][2] = (unsigned char) b * 255;
                env_map[s][t][3] = (unsigned char) a * 255;
            }
        }
    }
}

// 23 March 2001
// This function performs billboarding of polygons drawn using the UP and RIGHT vectors obtained
// from the transpose of the MODEL_VIEW_MATRIX of the ssg_current_context. Each polygon is drawn
// at the coordinate array and material state as passed thro arguments.
void *fgBillboard( ssgBranch *lightmaps, ssgVertexArray *light_maps, ssgSimpleState *lightmap_state, float size) {
    sgMat4 tmat;
    sgVec3 rt, up, nrt, nup, pt, quads[4], lmaps[4];

    ssgGetModelviewMatrix ( tmat );
    sgSetVec3 (rt, tmat[0][0], tmat[1][0], tmat[2][0]);
    sgSetVec3 (up, tmat[0][1], tmat[1][1], tmat[2][1]);
    sgSetVec3 (nrt, tmat[0][0], tmat[1][0], tmat[2][0]);
    sgSetVec3 (nup, tmat[0][1], tmat[1][1], tmat[2][1]);
    sgNegateVec3 (nrt);
    sgNegateVec3 (nup);

    sgAddVec3 (quads[0], nrt, nup);
    sgAddVec3 (quads[1],  rt, nup);
    sgAddVec3 (quads[2],  rt,  up);
    sgAddVec3 (quads[3], nrt,  up);

    sgScaleVec3 (quads[0], size);
    sgScaleVec3 (quads[1], size);
    sgScaleVec3 (quads[2], size);
    sgScaleVec3 (quads[3], size);

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 0.0, 1.0 );

    sgVec2 texcoords[4];
    sgSetVec2( texcoords[0], 1.0, 1.0 );
    sgSetVec2( texcoords[1], 0.0, 1.0 );
    sgSetVec2( texcoords[2], 0.0, 0.0 );
    sgSetVec2( texcoords[3], 1.0, 0.0 );
    
    for (int j = 0; j < 4; j++ ) {
        sgCopyVec3(lmaps[j]	,quads[j]);
    }

    for ( int i = 0; i < light_maps->getNum(); ++i ) {
        // Allocate ssg structure
        ssgVertexArray   *vl = new ssgVertexArray( 1 );
        ssgTexCoordArray *tl = new ssgTexCoordArray( 1 );
        ssgColourArray   *cl = new ssgColourArray( 1 );

        float *temp = light_maps->get(i);
        sgSetVec3(pt,temp[0],temp[1],temp[2]);

        for (int k=0; k<4; k++) {
            sgAddVec3( quads[k],lmaps[k], pt );			
            vl->add(quads[k]);
            tl->add(texcoords[k]);
            cl->add(color);
        }

        ssgLeaf *leaf = NULL;
        leaf = new ssgVtxTable ( GL_TRIANGLE_FAN, vl, NULL, tl, cl );
        leaf->setState( lightmap_state );
        lightmaps->addKid( leaf );
    }

    return NULL;
}

ssgBranch* FGTileEntry::gen_runway_lights( ssgVertexArray *points,ssgVertexArray *normal,
                                           ssgVertexArray *dir, int type[]) 
{

    //************** HARD CODED RUNWAY LIGHT TEXTURES BEGIN ************************    
    GLuint texEdge, texTaxi, texTouchdown;
    GLuint texThreshold;
    GLuint texVasi, texWhite, texRed, texGreen, texYellow;

    //VASI lights
    setColor(0.0,0.0,1.0,360.0, 0, 0, 0, 0);
    setColor2(10.0, 40.0, 1, 1, 1, 1);
    setColor2(6.0, 40.0, 1, 0.5, 0.5, 1);
    setColor2(5.0, 40.0, 1, 0, 0, 1);
    glGenTextures(1, &texVasi);
    glBindTexture(GL_TEXTURE_2D, texVasi);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *vasi_state;
    vasi_state = new ssgSimpleState();
    vasi_state->ref();
    vasi_state->setTexture( texVasi );
    vasi_state->disable( GL_LIGHTING );
    vasi_state->enable( GL_TEXTURE_2D );
    vasi_state->setShadeModel( GL_SMOOTH );

    //EDGE
    setColor(0.0,0.0,-1.0,180.0, 1, 1, 0.5, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texEdge);
    glBindTexture(GL_TEXTURE_2D, texEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *edge_state;
    edge_state = new ssgSimpleState();
    edge_state->ref();
    edge_state->setTexture( texEdge );
    edge_state->disable( GL_LIGHTING );
    edge_state->enable( GL_TEXTURE_2D );
    edge_state->setShadeModel( GL_SMOOTH );

    //TOUCHDOWN
    setColor(0.0,0.0,-1.0,180.0, 0, 1, 0, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texTouchdown);
    glBindTexture(GL_TEXTURE_2D, texTouchdown);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *touchdown_state;
    touchdown_state = new ssgSimpleState();
    touchdown_state->ref();
    touchdown_state->setTexture( texTouchdown );
    touchdown_state->disable( GL_LIGHTING );
    touchdown_state->enable( GL_TEXTURE_2D );
    touchdown_state->setShadeModel( GL_SMOOTH );
	
    //THRESHOLD
    setColor(0.0,0.0,-1.0,180.0, 1, 0, 0, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texThreshold);
    glBindTexture(GL_TEXTURE_2D, texThreshold);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *threshold_state;
    threshold_state = new ssgSimpleState();
    threshold_state->ref();
    threshold_state->setTexture( texThreshold );
    threshold_state->disable( GL_LIGHTING );
    threshold_state->enable( GL_TEXTURE_2D );
    threshold_state->setShadeModel( GL_SMOOTH );

    //TAXI
    setColor(0.0,0.0,-1.0,180.0, 0, 0, 1, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texTaxi);
    glBindTexture(GL_TEXTURE_2D, texTaxi);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *taxi_state;
    taxi_state = new ssgSimpleState();
    taxi_state->ref();
    taxi_state->setTexture( texTaxi );
    taxi_state->disable( GL_LIGHTING );
    taxi_state->enable( GL_TEXTURE_2D );
    taxi_state->setShadeModel( GL_SMOOTH );

    //WHITE
    setColor(0.0,0.0,-1.0,180.0, 1, 1, 1, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texWhite);
    glBindTexture(GL_TEXTURE_2D, texWhite);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *white_state;
    white_state = new ssgSimpleState();
    white_state->ref();
    white_state->setTexture( texWhite );
    white_state->disable( GL_LIGHTING );
    white_state->enable( GL_TEXTURE_2D );
    white_state->setShadeModel( GL_SMOOTH );

    //RED
    setColor(0.0,0.0,-1.0,180.0, 1, 0, 0, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texRed);
    glBindTexture(GL_TEXTURE_2D, texRed);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *red_state;
    red_state = new ssgSimpleState();
    red_state->ref();
    red_state->setTexture( texRed );
    red_state->disable( GL_LIGHTING );
    red_state->enable( GL_TEXTURE_2D );
    red_state->setShadeModel( GL_SMOOTH );

    //GREEN
    setColor(0.0,0.0,-1.0,180.0, 0, 1, 0, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texGreen);
    glBindTexture(GL_TEXTURE_2D, texGreen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *green_state;
    green_state = new ssgSimpleState();
    green_state->ref();
    green_state->setTexture( texGreen );
    green_state->disable( GL_LIGHTING );
    green_state->enable( GL_TEXTURE_2D );
    green_state->setShadeModel( GL_SMOOTH );

    //YELLOW
    setColor(0.0,0.0,-1.0,180.0, 1, 1, 0, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &texYellow);
    glBindTexture(GL_TEXTURE_2D, texYellow);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEXRES_X, TEXRES_Y, 0, GL_RGBA, GL_UNSIGNED_BYTE, env_map);
    ssgSimpleState *yellow_state;
    yellow_state = new ssgSimpleState();
    yellow_state->ref();
    yellow_state->setTexture( texYellow );
    yellow_state->disable( GL_LIGHTING );
    yellow_state->enable( GL_TEXTURE_2D );
    yellow_state->setShadeModel( GL_SMOOTH );
//************** HARD CODED RUNWAY LIGHT TEXTURES END ************************    

    ssgBranch *runway_lights = new ssgBranch;
    sgVec3 v2,v3,inf,side;

    ssgLeaf *leaf1 = NULL;
    ssgLeaf *leaf2 = NULL;
    ssgLeaf *leaf7 = NULL;
    ssgLeaf *leaf8 = NULL;
    ssgLeaf *leaf9 = NULL;

    ssgVertexArray   *vlw = new ssgVertexArray( 1 );
    ssgNormalArray   *nlw = new ssgNormalArray( 1 );
    ssgVertexArray   *vlt = new ssgVertexArray( 1 );
    ssgNormalArray   *nlt = new ssgNormalArray( 1 );
    ssgVertexArray   *vlr = new ssgVertexArray( 1 );
    ssgNormalArray   *nlr = new ssgNormalArray( 1 );
    ssgVertexArray   *vlg = new ssgVertexArray( 1 );
    ssgNormalArray   *nlg = new ssgNormalArray( 1 );
    ssgVertexArray   *vly = new ssgVertexArray( 1 );
    ssgNormalArray   *nly = new ssgNormalArray( 1 );

    for ( int i = 0; i < points->getNum()-1; i=i++ ) {

        // Allocate ssg structure
        ssgVertexArray   *vl = new ssgVertexArray( 1 );
        ssgNormalArray   *nl = new ssgNormalArray( 1 );
        ssgVertexArray   *vl1 = new ssgVertexArray( 1 );
        ssgNormalArray   *nl1 = new ssgNormalArray( 1 );

        float *n1 = normal->get(i);
        float *d1 = dir->get(i);

        /* TEMPORARY CODE BEGIN 
           // calculate normal using 1st, 2nd & last vertices of the group
           sgVec3 n1;
           sgMakeNormal (n1, points->get(0), points->get(1), points->get(points->getNum()-1) );
           sgVec3 d1;
           sgSubVec3(d1,points->get(1),points->get(0));
           printf("%f %f %f\n",n1[0],n1[1],n1[2]);
           printf("%f %f %f\n",d1[0],d1[1],d1[2]);
           type[i] = 2;
           ----TEMPORARY CODE END */


        sgNormaliseVec3 ( n1 );
        sgNormaliseVec3 ( d1 );
        sgVec3 d2;
        d2[0] = -d1[0];
        d2[1] = -d1[1];
        d2[2] = -d1[2];

        sgVectorProductVec3(side,n1,d1);
        sgScaleVec3 (inf,n1,-50);
        sgScaleVec3 (side,5);

        float *v1 = points->get(i);
        sgAddVec3(v2,v1,inf);
        sgAddVec3(v3,v2,side);

        if ( type[i] == 1) {        //POINT,WHITE

            vlw->add(v1);
            nlw->add(d1);

        } else if (type[i] == 2) {  //POINT,TAXI

            vlt->add(v1);
            nlt->add(d1);

        } else if (type[i] == 3) {  //SINGLE POLYGON,VASI

            vl->add(v1);
            nl->add(d1);
            vl->add(v3);
            nl->add(d1);
            vl->add(v2);
            nl->add(d1);

            ssgLeaf *leaf3 = NULL;
            leaf3 = new ssgVtxTable ( GL_POLYGON, vl, nl, NULL, NULL );
            leaf3->setState( vasi_state );
            runway_lights->addKid( leaf3 );

        } else if (type[i] == 4) {  //BACK-TO-BACK POLYGONS,TOUCHDOWN/THRESHOLD
	        
            vl->add(v1);
            nl->add(d1);
            vl->add(v3);
            nl->add(d1);
            vl->add(v2);
            nl->add(d1);

            vl1->add(v3);
            nl1->add(d2);
            vl1->add(v1);
            nl1->add(d2);
            vl1->add(v2);
            nl1->add(d2);

            ssgLeaf *leaf41 = NULL;
            leaf41 = new ssgVtxTable ( GL_POLYGON, vl, nl, NULL, NULL );
            leaf41->setState( touchdown_state );
            runway_lights->addKid( leaf41 );

            ssgLeaf *leaf42 = NULL;
            leaf42 = new ssgVtxTable ( GL_POLYGON, vl1, nl1, NULL, NULL );
            leaf42->setState( threshold_state );
            runway_lights->addKid( leaf42 );

        } else if ( type[i] == 5) {        //POINT,WHITE, SEQUENCE LIGHTS (RABBIT)

            vl->add(v1);
            nl->add(d1);
            ssgLeaf *leaf5 = NULL;
            leaf5 = new ssgVtxTable ( GL_POINTS, vl, nl, NULL, NULL );
            leaf5->setState( white_state );
            lightmaps_sequence->addKid (leaf5);

        } else if ( type[i] == 6) {        //POINT,WHITE, SEQUENCE LIGHTS (RABBIT)

            vl->add(v1);
            nl->add(d1);
            ssgLeaf *leaf6 = NULL;
            leaf6 = new ssgVtxTable ( GL_POINTS, vl, nl, NULL, NULL );
            leaf6->setState( yellow_state );
            ols_transform->addKid (leaf6);
            // DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS
            //			lightmaps_sequence->addKid (leaf6);
            // DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS

        } else if (type[i] == 7) {  //POINT,RED

            vlr->add(v1);
            nlr->add(d1);

        } else if (type[i] == 8) {  //POINT,GREEN

            vlg->add(v1);
            nlg->add(d1);

        } else if (type[i] == 9) {  //POINT,YELLOW

            vly->add(v1);
            nly->add(d1);

        }

    }

    leaf1 = new ssgVtxTable ( GL_POINTS, vlw, nlw, NULL, NULL );
    leaf1->setState( white_state );
    runway_lights->addKid( leaf1 );

    leaf2 = new ssgVtxTable ( GL_POINTS, vlt, nlt, NULL, NULL );
    leaf2->setState( taxi_state );
    runway_lights->addKid( leaf2 );
	
    leaf7 = new ssgVtxTable ( GL_POINTS, vlr, nlr, NULL, NULL );
    leaf7->setState( red_state );
    runway_lights->addKid( leaf7 );

    leaf8 = new ssgVtxTable ( GL_POINTS, vlg, nlg, NULL, NULL );
    leaf8->setState( green_state );
    // DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS
    ols_transform->ref();
    lightmaps_sequence->addKid (ols_transform);
    // DO NOT DELETE THIS CODE - This is to compare a discrete FLOLS (without LOD) with analog FLOLS
    lightmaps_sequence->addKid (leaf8);

    leaf9 = new ssgVtxTable ( GL_POINTS, vly, nly, NULL, NULL );
    leaf9->setState( yellow_state );
    runway_lights->addKid( leaf9 );

    lightmaps_sequence->select(0xFFFFFF);	
    return runway_lights;
}
// ADA
	
#ifdef WISH_PLIB_WAS_THREADED // but it isn't

// Schedule tile to be freed/removed
void FGTileEntry::sched_removal() {
    global_tile_mgr.ready_to_delete( this );
}

#endif


// Clean up the memory used by this tile and delete the arrays used by
// ssg as well as the whole ssg branch
void FGTileEntry::free_tile() {
    int i;
    SG_LOG( SG_TERRAIN, SG_INFO,
	    "FREEING TILE = (" << tile_bucket << ")" );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting " << nodes.size() << " nodes" );
    nodes.clear();

    // delete the ssg structures
    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting (leaf data) vertex, normal, and "
	    << " texture coordinate arrays" );

    for ( i = 0; i < (int)vec3_ptrs.size(); ++i ) {
	delete [] vec3_ptrs[i];
    }
    vec3_ptrs.clear();

    for ( i = 0; i < (int)vec2_ptrs.size(); ++i ) {
	delete [] vec2_ptrs[i];
    }
    vec2_ptrs.clear();

    for ( i = 0; i < (int)index_ptrs.size(); ++i ) {
	delete index_ptrs[i];
    }
    index_ptrs.clear();

    // delete the terrain branch (this should already have been
    // disconnected from the scene graph)
    ssgDeRefDelete( terra_transform );

    if ( gnd_lights_transform ) {
	// delete the terrain lighting branch (this should already have been
	// disconnected from the scene graph)
	ssgDeRefDelete( gnd_lights_transform );
    }

    if ( rwy_lights_transform ) {
	// delete the terrain lighting branch (this should already have been
	// disconnected from the scene graph)
	ssgDeRefDelete( rwy_lights_transform );
    }

    // ADA
    if ( lightmaps_transform ) {
	// delete the terrain lighting branch (this should already have been
        // disconnected from the scene graph)
	ssgDeRefDelete( lightmaps_transform );
    }
    // ADA
}


// Update the ssg transform node for this tile so it can be
// properly drawn relative to our (0,0,0) point
void FGTileEntry::prep_ssg_node( const Point3D& p, sgVec3 up, float vis) {
    if ( !loaded ) return;

    SetOffset( p );

// #define USE_UP_AND_COMING_PLIB_FEATURE
#ifdef USE_UP_AND_COMING_PLIB_FEATURE
    terra_range->setRange( 0, SG_ZERO );
    terra_range->setRange( 1, vis + bounding_radius );
    if ( gnd_lights_range ) {
    gnd_lights_range->setRange( 0, SG_ZERO );
    gnd_lights_range->setRange( 1, vis * 1.5 + bounding_radius );
    }
    if ( rwy_lights_range ) {
      rwy_lights_range->setRange( 0, SG_ZERO );
      rwy_lights_range->setRange( 1, vis * 1.5 + bounding_radius );
    }
#else
    float ranges[2];
    ranges[0] = SG_ZERO;
    ranges[1] = vis + bounding_radius;
    terra_range->setRanges( ranges, 2 );
    if ( gnd_lights_range ) {
	ranges[1] = vis * 1.5 + bounding_radius;
	gnd_lights_range->setRanges( ranges, 2 );
    }
    if ( rwy_lights_range ) {
	ranges[1] = vis * 1.5 + bounding_radius;
	rwy_lights_range->setRanges( ranges, 2 );
    }
#endif
    sgVec3 sgTrans;
    sgSetVec3( sgTrans, offset.x(), offset.y(), offset.z() );
    terra_transform->setTransform( sgTrans );

    if ( gnd_lights_transform ) {
	// we need to lift the lights above the terrain to avoid
	// z-buffer fighting.  We do this based on our altitude and
	// the distance this tile is away from scenery center.

        // we expect 'up' to be a unit vector coming in, but since we
        // modify the value of lift_vec, we need to create a local
        // copy.
	sgVec3 lift_vec;
	sgCopyVec3( lift_vec, up );

	double agl;
	if ( current_aircraft.fdm_state ) {
	    agl = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
		- globals->get_scenery()->get_cur_elev();
	} else {
	    agl = 0.0;
	}

	// sgTrans just happens to be the
	// vector from scenery center to the center of this tile which
	// is what we want to calculate the distance of
	sgVec3 to;
	sgCopyVec3( to, sgTrans );
	double dist = sgLengthVec3( to );

	if ( general.get_glDepthBits() > 16 ) {
	    sgScaleVec3( lift_vec, 10.0 + agl / 100.0 + dist / 10000 );
	} else {
	    sgScaleVec3( lift_vec, 10.0 + agl / 20.0 + dist / 5000 );
	}

	sgVec3 lt_trans;
	sgCopyVec3( lt_trans, sgTrans );

	sgAddVec3( lt_trans, lift_vec );
	gnd_lights_transform->setTransform( lt_trans );

	// select which set of lights based on sun angle
	float sun_angle = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
	if ( sun_angle > 95 ) {
	    gnd_lights_brightness->select(0x04);
	} else if ( sun_angle > 92 ) {
	    gnd_lights_brightness->select(0x02);
	} else if ( sun_angle > 89 ) {
	    gnd_lights_brightness->select(0x01);
	} else {
	    gnd_lights_brightness->select(0x00);
	}
    }

    if ( rwy_lights_transform ) {
	// we need to lift the lights above the terrain to avoid
	// z-buffer fighting.  We do this based on our altitude and
	// the distance this tile is away from scenery center.

	sgVec3 lift_vec;
	sgCopyVec3( lift_vec, up );

	double agl;
	if ( current_aircraft.fdm_state ) {
	    agl = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
		- globals->get_scenery()->get_cur_elev();
	} else {
	    agl = 0.0;
	}

	// sgTrans just happens to be the
	// vector from scenery center to the center of this tile which
	// is what we want to calculate the distance of
	sgVec3 to;
	sgCopyVec3( to, sgTrans );
	double dist = sgLengthVec3( to );

	if ( general.get_glDepthBits() > 16 ) {
	    sgScaleVec3( lift_vec, 0.0 + agl / 100.0 + dist / 10000 );
	} else {
	    sgScaleVec3( lift_vec, 1.0 + agl / 20.0 + dist / 5000 );
	}

	sgVec3 lt_trans;
	sgCopyVec3( lt_trans, sgTrans );

	sgAddVec3( lt_trans, lift_vec );
	rwy_lights_transform->setTransform( lt_trans );

	// select which set of lights based on sun angle
	// float sun_angle = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
	// if ( sun_angle > 95 ) {
	//     gnd_lights_brightness->select(0x04);
	// } else if ( sun_angle > 92 ) {
	//     gnd_lights_brightness->select(0x02);
	// } else if ( sun_angle > 89 ) {
	//     gnd_lights_brightness->select(0x01);
	// } else {
	//     gnd_lights_brightness->select(0x00);
	// }
    }

    // ADA
    // Transform & Render runway lights - 23 Mar 2001
    sgSetVec3( sgTrans, offset.x(), offset.y(), offset.z() );
    if ( lightmaps_transform ) {
        static unsigned int selectnode = 0;
	// Run-time extension check.
	if (!glutExtensionSupported("GL_EXT_point_parameters")) {
            //use lightmaps on billboarded polygons
        } else {
            // using GL_EXT_point_parameters
		
            // This part is same as ground-lights code above by Curt
            sgVec3 lift_vec;
            sgCopyVec3( lift_vec, up );
            
            double agl1;
            if ( current_aircraft.fdm_state ) {
                agl1 = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
                    - globals->get_scenery()->get_cur_elev();
            } else {
                agl1 = 0.0;
            }

            // sgTrans just happens to be the
            // vector from scenery center to the center of this tile which
            // is what we want to calculate the distance of
            sgVec3 to1;
            sgCopyVec3( to1, sgTrans );
            double dist1 = sgLengthVec3( to1 );

            if ( general.get_glDepthBits() > 16 ) {
                sgScaleVec3( lift_vec, 0.0 + agl1 / 2000.0 + dist1 / 10000 );
            } else {
                sgScaleVec3( lift_vec, 0.0 + agl1 / 20.0 + dist1 / 5000 );
            }
            sgAddVec3( sgTrans, lift_vec );
            lightmaps_transform->setTransform( sgTrans );

            float sun_angle1 = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
            if ( (sun_angle1 > 89) ) {
                lightmaps_brightness->select(0x01);
                selectnode *=2;
                selectnode = selectnode | 0x000001;
                if (selectnode > 0xFFFFFF) selectnode = 1;
                lightmaps_sequence->select(selectnode);
            } else {
                lightmaps_brightness->select(0x00);
                lightmaps_sequence->select(0x000000);
            } 
	} // end of GL_EXT_point_parameters section

    } // end of runway lights section
    // ADA

}


// Set up lights rendering call backs
static int fgLightsPredraw( ssgEntity *e ) {
#if 0
#ifdef GL_EXT_point_parameters
    if (glutExtensionSupported("GL_EXT_point_parameters")) {
	static float quadratic[3] = {1.0, 0.01, 0.0001};
	glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT, quadratic);
	glPointParameterfEXT(GL_POINT_SIZE_MIN_EXT, 1.0); 
	glPointSize(4.0);
    }
#endif
#endif
    return true;
}

static int fgLightsPostdraw( ssgEntity *e ) {
#if 0
#ifdef GL_EXT_point_parameters
    if (glutExtensionSupported("GL_EXT_point_parameters")) {
	static float default_attenuation[3] = {1.0, 0.0, 0.0};
	glPointParameterfvEXT(GL_DISTANCE_ATTENUATION_EXT,
			      default_attenuation);
	glPointSize(1.0);
    }
#endif
#endif
    return true;
}


ssgLeaf* FGTileEntry::gen_lights( ssgVertexArray *lights, int inc, float bright ) {
    // generate a repeatable random seed
    float *p1 = lights->get( 0 );
    unsigned int *seed = (unsigned int *)p1;
    sg_srandom( *seed );

    int size = lights->getNum() / inc;

    // Allocate ssg structure
    ssgVertexArray   *vl = new ssgVertexArray( size + 1 );
    ssgNormalArray   *nl = NULL;
    ssgTexCoordArray *tl = NULL;
    ssgColourArray   *cl = new ssgColourArray( size + 1 );

    sgVec4 color;
    for ( int i = 0; i < lights->getNum(); ++i ) {
	// this loop is slightly less efficient than it otherwise
	// could be, but we want a red light to always be red, and a
	// yellow light to always be yellow, etc. so we are trying to
	// preserve the random sequence.
	float zombie = sg_random();
	if ( i % inc == 0 ) {
	    vl->add( lights->get(i) );

	    // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
	    float factor = sg_random();
	    factor *= factor;

	    if ( zombie > 0.5 ) {
		// 50% chance of yellowish
		sgSetVec4( color, 0.9, 0.9, 0.3, bright - factor * 0.2 );
	    } else if ( zombie > 0.15 ) {
		// 35% chance of whitish
		sgSetVec4( color, 0.9, 0.9, 0.8, bright - factor * 0.2 );
	    } else if ( zombie > 0.05 ) {
		// 10% chance of orangish
		sgSetVec4( color, 0.9, 0.6, 0.2, bright - factor * 0.2 );
	    } else {
		// 5% chance of redish
		sgSetVec4( color, 0.9, 0.2, 0.2, bright - factor * 0.2 );
	    }
	    cl->add( color );
	}
    }

    // create ssg leaf
    ssgLeaf *leaf = 
	new ssgVtxTable ( GL_POINTS, vl, nl, tl, cl );

    // assign state
    FGNewMat *newmat = material_lib.find( "LIGHTS" );
    leaf->setState( newmat->get_state() );
    leaf->setCallback( SSG_CALLBACK_PREDRAW, fgLightsPredraw );
    leaf->setCallback( SSG_CALLBACK_POSTDRAW, fgLightsPostdraw );

    return leaf;
}


bool FGTileEntry::obj_load( const std::string& path,
		       ssgBranch* geometry,
		       ssgBranch* rwy_lights,
		       ssgVertexArray* ground_lights, bool is_base )
{
    Point3D c;			// returned center point
    double br;			// returned bounding radius

    // try loading binary format
    if ( fgBinObjLoad( path, is_base,
		       &c, &br, geometry, rwy_lights, ground_lights ) )
    {
	if ( is_base ) {
	    center = c;
	    bounding_radius = br;
	}
    } else {
	// next try the older ascii format, this is some ugly
	// weirdness because the ascii loader is *old* and hasn't been
	// updated, but hopefully we can can the ascii format soon.
	ssgBranch *tmp = fgAsciiObjLoad( path, this, ground_lights, is_base );
	if ( tmp ) {
	    return (NULL != tmp);
	} else {
	    // default to an ocean tile
	    if ( fgGenTile( path, tile_bucket, &c, &br, geometry ) ) {
		center = c;
		bounding_radius = br;
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"Warning: failed to generate ocean tile!" );
	    }
	}
    }

    return (NULL != geometry);
}


void
FGTileEntry::load( const SGPath& base, bool is_base )
{
    SG_LOG( SG_TERRAIN, SG_INFO, "load() base = " << base.str() );

    // Generate names for later use
    string index_str = tile_bucket.gen_index_str();

    SGPath tile_path = base;
    tile_path.append( tile_bucket.gen_base_path() );

    SGPath basename = tile_path;
    basename.append( index_str );
    // string path = basename.str();

    SG_LOG( SG_TERRAIN, SG_INFO, "Loading tile " << basename.str() );

#define FG_MAX_LIGHTS 1000

    // obj_load() will generate ground lighting for us ...
    ssgVertexArray *light_pts = new ssgVertexArray( 100 );

    // ADA
    ssgVertexArray *lights_rway = new ssgVertexArray( 100 );
    ssgVertexArray *lights_dir = new ssgVertexArray( 100 );
    ssgVertexArray *lights_normal = new ssgVertexArray( 100 );
    int lights_type[FG_MAX_LIGHTS];
    // ADA

    ssgBranch* new_tile = new ssgBranch;

    // Check for master .stg (scene terra gear) file
    SGPath stg_name = basename;
    stg_name.concat( ".stg" );

    sg_gzifstream in( stg_name.str() );

    if ( in.is_open() ) {
	string token, name;

	while ( ! in.eof() ) {
	    in >> token;

	    if ( token == "OBJECT_BASE" ) {
		in >> name >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " name = " << name );

		SGPath custom_path = tile_path;
		custom_path.append( name );

		ssgBranch *geometry = new ssgBranch;
		if ( obj_load( custom_path.str(),
			       geometry, NULL, light_pts, true ) )
		{
		    new_tile -> addKid( geometry );
		} else {
		    delete geometry;
		}
	    } else if ( token == "OBJECT" ) {
		in >> name >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_DEBUG, "token = " << token
			<< " name = " << name );

		SGPath custom_path = tile_path;
		custom_path.append( name );

		ssgBranch *geometry = new ssgBranch;
		ssgBranch *rwy_lights = new ssgBranch;
		if ( obj_load( custom_path.str(),
			       geometry, rwy_lights, NULL, false ) )
		{
		    if ( geometry -> getNumKids() > 0 ) {
			new_tile -> addKid( geometry );
		    } else {
			delete geometry;
		    }
		    if ( rwy_lights -> getNumKids() > 0 ) {
			rwy_lights_range -> addKid( rwy_lights );
		    } else {
			delete rwy_lights;
		    }
		} else {
		    delete geometry;
		    delete rwy_lights;
		}

	    } else if ( token == "OBJECT_STATIC" ) {
		// load object info
		double lon, lat, elev, hdg;
		in >> name >> lon >> lat >> elev >> hdg >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " name = " << name 
			<< " pos = " << lon << ", " << lat
			<< " elevation = " << elev
			<< " heading = " << hdg );

		// object loading is deferred to main render thread,
		// but lets figure out the paths right now.
		SGPath custom_path = tile_path;
		custom_path.append( name );

		sgCoord obj_pos;
		WorldCoordinate( &obj_pos, center, lat, lon, elev, hdg );
		
		ssgTransform *obj_trans = new ssgTransform;
		obj_trans->setTransform( &obj_pos );

		// wire as much of the scene graph together as we can
		new_tile->addKid( obj_trans );

		// bump up the pending models count
		pending_models++;

		// push an entry onto the model load queue
		FGDeferredModel *dm
		    = new FGDeferredModel( custom_path.str(), tile_path.str(),
					   this, obj_trans );
		FGTileMgr::model_ready( dm );
	    } else if ( token == "OBJECT_TAXI_SIGN" ) {
		// load object info
		double lon, lat, elev, hdg;
		in >> name >> lon >> lat >> elev >> hdg >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " name = " << name 
			<< " pos = " << lon << ", " << lat
			<< " elevation = " << elev
			<< " heading = " << hdg );

		// load the object itself
		SGPath custom_path = tile_path;
		custom_path.append( name );

		sgCoord obj_pos;
		WorldCoordinate( &obj_pos, center, lat, lon, elev, hdg );

		ssgTransform *obj_trans = new ssgTransform;
		obj_trans->setTransform( &obj_pos );

		ssgBranch *custom_obj
		    = gen_taxi_sign( custom_path.str(), name );

		// wire the pieces together
		if ( custom_obj != NULL ) {
		    obj_trans -> addKid( custom_obj );
		}
		new_tile->addKid( obj_trans );
	    } else if ( token == "OBJECT_RUNWAY_SIGN" ) {
		// load object info
		double lon, lat, elev, hdg;
		in >> name >> lon >> lat >> elev >> hdg >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " name = " << name 
			<< " pos = " << lon << ", " << lat
			<< " elevation = " << elev
			<< " heading = " << hdg );

		// load the object itself
		SGPath custom_path = tile_path;
		custom_path.append( name );

		sgCoord obj_pos;
		WorldCoordinate( &obj_pos, center, lat, lon, elev, hdg );

		ssgTransform *obj_trans = new ssgTransform;
		obj_trans->setTransform( &obj_pos );

		ssgBranch *custom_obj
		    = gen_runway_sign( custom_path.str(), name );

		// wire the pieces together
		if ( custom_obj != NULL ) {
		    obj_trans -> addKid( custom_obj );
		}
		new_tile->addKid( obj_trans );
	    } else if ( token == "RWY_LIGHTS" ) {
		double lon, lat, hdg, len, width;
		string common, end1, end2;
		in >> lon >> lat >> hdg >> len >> width
		   >> common >> end1 >> end2;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " pos = " << lon << ", " << lat
			<< " hdg = " << hdg
			<< " size = " << len << ", " << width
			<< " codes = " << common << " "
			<< end1 << " " << end2 );
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"Unknown token " << token << " in "
			<< stg_name.str() );
		in >> ::skipws;
	    }
	}
    } else {
        // no .stg file, generate an ocean tile on the fly for this
        // area
        ssgBranch *geometry = new ssgBranch;
        Point3D c;
        double br;
        if ( fgGenTile( basename.str(), tile_bucket, &c, &br, geometry ) ) {
            center = c;
            bounding_radius = br;
            new_tile -> addKid( geometry );
        } else {
            delete geometry;
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Warning: failed to generate ocean tile!" );
        }
    }

    if ( new_tile != NULL ) {
        terra_range->addKid( new_tile );
    }

    terra_transform->addKid( terra_range );

    // calculate initial tile offset
    SetOffset( globals->get_scenery()->get_center() );
    sgCoord sgcoord;
    sgSetCoord( &sgcoord,
		offset.x(), offset.y(), offset.z(),
		0.0, 0.0, 0.0 );
    terra_transform->setTransform( &sgcoord );
    // terrain->addKid( terra_transform );

    // Add ground lights to scene graph if any exist
    gnd_lights_transform = NULL;
    gnd_lights_range = NULL;
    if ( light_pts->getNum() ) {
	SG_LOG( SG_TERRAIN, SG_DEBUG, "generating lights" );
	gnd_lights_transform = new ssgTransform;
	gnd_lights_range = new ssgRangeSelector;
	gnd_lights_brightness = new ssgSelector;
	ssgLeaf *lights;

	lights = gen_lights( light_pts, 4, 0.7 );
	gnd_lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 2, 0.85 );
	gnd_lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 1, 1.0 );
	gnd_lights_brightness->addKid( lights );

	gnd_lights_range->addKid( gnd_lights_brightness );
	gnd_lights_transform->addKid( gnd_lights_range );
	gnd_lights_transform->setTransform( &sgcoord );
    }

    // Add runway lights to scene graph if any exist
    if ( rwy_lights_range->getNumKids() > 0 ) {
	SG_LOG( SG_TERRAIN, SG_INFO, "adding runway lights" );
	rwy_lights_transform->addKid( rwy_lights_range );
	rwy_lights_transform->setTransform( &sgcoord );
    }

    // ADA
    // Create runway lights - 23 Mar 2001
    lightmaps_transform = NULL;
    lightmaps_sequence = NULL;
    ols_transform = NULL;
    //    lightmaps_range = NULL;

    if ( lights_rway->getNum() ) {
	SG_LOG( SG_TERRAIN, SG_DEBUG, "generating airport lights" );
	lightmaps_transform = new ssgTransform;
        //	lightmaps_range = new ssgRangeSelector;
	lightmaps_brightness = new ssgSelector;
	lightmaps_sequence = new ssgSelector;
	ols_transform = new ssgTransform;
        ssgBranch *lightmaps_branch;

        // call function to generate the runway lights
        lightmaps_branch = gen_runway_lights( lights_rway, 
                                              lights_normal, lights_dir, lights_type);
	lightmaps_brightness->addKid( lightmaps_branch );
	
	// build the runway lights' scene
        //    lightmaps_range->addKid( lightmaps_brightness ); //dont know why this doesnt work !!
        //	lightmaps_transform->addKid( lightmaps_range );    //dont know why this doesnt work !!
	lightmaps_transform->addKid( lightmaps_brightness );
        lightmaps_sequence->setTraversalMaskBits( SSGTRAV_HOT );
	lightmaps_transform->addKid( lightmaps_sequence );
	lightmaps_transform->setTransform( &sgcoord );
    }
    // ADA
}


void
FGTileEntry::add_ssg_nodes( ssgBranch* terrain_branch,
			    ssgBranch* gnd_lights_branch,
			    ssgBranch* rwy_lights_branch )
{
    // bump up the ref count so we can remove this later without
    // having ssg try to free the memory.
    terra_transform->ref();
    terrain_branch->addKid( terra_transform );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
            "connected a tile into scene graph.  terra_transform = "
            << terra_transform );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "num parents now = "
            << terra_transform->getNumParents() );

    if ( gnd_lights_transform != NULL ) {
	// bump up the ref count so we can remove this later without
	// having ssg try to free the memory.
	gnd_lights_transform->ref();
	gnd_lights_branch->addKid( gnd_lights_transform );
    }

    if ( rwy_lights_transform != NULL ) {
	// bump up the ref count so we can remove this later without
	// having ssg try to free the memory.
	rwy_lights_transform->ref();
	rwy_lights_branch->addKid( rwy_lights_transform );
    }

    // ADA
    if ( lightmaps_transform != 0 ) {
	// bump up the ref count so we can remove this later without
	// having ssg try to free the memory.
	lightmaps_transform->ref();
	gnd_lights_branch->addKid( lightmaps_transform );
    }
    // ADA

    loaded = true;
}


void
FGTileEntry::disconnect_ssg_nodes()
{
    SG_LOG( SG_TERRAIN, SG_INFO, "disconnecting ssg nodes" );

    if ( ! loaded ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "removing a not-fully loaded tile!" );
    } else {
        SG_LOG( SG_TERRAIN, SG_INFO, "removing a fully loaded tile!  terra_transform = " << terra_transform );
    }
        
    // find the terrain branch parent
    int pcount = terra_transform->getNumParents();
    if ( pcount > 0 ) {
	// find the first parent (should only be one)
	ssgBranch *parent = terra_transform->getParent( 0 ) ;
	if( parent ) {
	    // disconnect the tile (we previously ref()'d it so it
	    // won't get freed now)
	    parent->removeKid( terra_transform );
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "parent pointer is NULL!  Dying" );
	    exit(-1);
	}
    } else {
	SG_LOG( SG_TERRAIN, SG_ALERT,
		"Parent count is zero for an ssg tile!  Dying" );
	exit(-1);
    }

    // find the ground lighting branch
    if ( gnd_lights_transform ) {
	pcount = gnd_lights_transform->getNumParents();
	if ( pcount > 0 ) {
	    // find the first parent (should only be one)
	    ssgBranch *parent = gnd_lights_transform->getParent( 0 ) ;
	    if( parent ) {
		// disconnect the light branch (we previously ref()'d
		// it so it won't get freed now)
		parent->removeKid( gnd_lights_transform );
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"parent pointer is NULL!  Dying" );
		exit(-1);
	    }
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "Parent count is zero for an ssg light tile!  Dying" );
	    exit(-1);
	}
    }

    // find the runway lighting branch
    if ( rwy_lights_transform ) {
	pcount = rwy_lights_transform->getNumParents();
	if ( pcount > 0 ) {
	    // find the first parent (should only be one)
	    ssgBranch *parent = rwy_lights_transform->getParent( 0 ) ;
	    if( parent ) {
		// disconnect the light branch (we previously ref()'d
		// it so it won't get freed now)
		parent->removeKid( rwy_lights_transform );
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"parent pointer is NULL!  Dying" );
		exit(-1);
	    }
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "Parent count is zero for an ssg light tile!  Dying" );
	    exit(-1);
	}
    }

    // ADA
    //runway lights - 23 Mar 2001
    // Delete runway lights and free memory
    if ( lightmaps_transform ) {
	// delete the runway lighting branch
	pcount = lightmaps_transform->getNumParents();
	if ( pcount > 0 ) {
	    // find the first parent (should only be one)
	    ssgBranch *parent = lightmaps_transform->getParent( 0 ) ;
	    if( parent ) {
		parent->removeKid( lightmaps_transform );
		lightmaps_transform = NULL;
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"lightmaps parent pointer is NULL!  Dying" );
		exit(-1);
	    }
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "Parent count is zero for an ssg lightmap tile!  Dying" );
	    exit(-1);
	}
    }
    // ADA
}
