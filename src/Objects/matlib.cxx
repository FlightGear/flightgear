// materialmgr.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/misc/exception.hxx>

#include <string.h>
#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>

#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/tileentry.hxx>

#include "newmat.hxx"
#include "matlib.hxx"

SG_USING_NAMESPACE(std);
SG_USING_STD(string);


// global material management class
FGMaterialLib material_lib;


// Constructor
FGMaterialLib::FGMaterialLib ( void ) {
  set_step(0);
}


static int gen_test_light_map() {
    static const int env_tex_res = 32;
    int half_res = env_tex_res / 2;
    unsigned char env_map[env_tex_res][env_tex_res][4];
    GLuint tex_name;

    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double dist = sqrt(x*x + y*y);
            if ( dist > 1.0 ) { dist = 1.0; }

            // cout << x << "," << y << " " << (int)(dist * 255) << ","
            //      << (int)((1.0 - dist) * 255) << endl;
            env_map[i][j][0] = (int)(dist * 255);
            env_map[i][j][1] = (int)((1.0 - dist) * 255);
            env_map[i][j][2] = 0;
            env_map[i][j][3] = 255;
        }
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glGenTextures( 1, &tex_name );
    glBindTexture( GL_TEXTURE_2D, tex_name );
  
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, env_tex_res, env_tex_res, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, env_map);

    return tex_name;
}


// generate standard colored directional light environment texture map
static int gen_standard_dir_light_map( int r, int g, int b, int alpha ) {
    const int env_tex_res = 32;
    int half_res = env_tex_res / 2;
    unsigned char env_map[env_tex_res][env_tex_res][4];
    GLuint tex_name;

    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double dist = sqrt(x*x + y*y);
            if ( dist > 1.0 ) { dist = 1.0; }
            double bright = cos( dist * SGD_PI_2 );
            if ( bright < 0.3 ) { bright = 0.3; }
            env_map[i][j][0] = r;
            env_map[i][j][1] = g;
            env_map[i][j][2] = b;
            env_map[i][j][3] = (int)(bright * alpha);
        }
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glGenTextures( 1, &tex_name );
    glBindTexture( GL_TEXTURE_2D, tex_name );
  
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, env_tex_res, env_tex_res, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, env_map);

    return tex_name;
}


// generate the directional vasi light environment texture map
static int gen_vasi_light_map() {
    const int env_tex_res = 256;
    int half_res = env_tex_res / 2;
    unsigned char env_map[env_tex_res][env_tex_res][4];
    GLuint tex_name;

    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double dist = sqrt(x*x + y*y);
            if ( dist > 1.0 ) { dist = 1.0; }
            double bright = cos( dist * SGD_PI_2 );

            // top half white, bottom half red
            env_map[i][j][0] = 255;
            if ( i > half_res ) {
                // white
                env_map[i][j][1] = 255;
                env_map[i][j][2] = 255;
            } else if ( i == half_res - 1 || i == half_res ) {
                // pink
                env_map[i][j][1] = 127;
                env_map[i][j][2] = 127;
            } else {
                // red
                env_map[i][j][1] = 0;
                env_map[i][j][2] = 0;
            }
            env_map[i][j][3] = (int)(bright * 255);
        }
    }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    glGenTextures( 1, &tex_name );
    glBindTexture( GL_TEXTURE_2D, tex_name );
  
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, env_tex_res, env_tex_res, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, env_map);

    return tex_name;
}


// Load a library of material properties
bool FGMaterialLib::load( const string& mpath ) {

    SGPropertyNode materials;

    SG_LOG( SG_INPUT, SG_INFO, "Reading materials from " << mpath );
    try {
        readProperties( mpath, &materials );
    } catch (const sg_exception &ex) {
        SG_LOG( SG_INPUT, SG_ALERT, "Error reading materials: "
                << ex.getMessage() );
        throw ex;
    }

    int nMaterials = materials.nChildren();
    for (int i = 0; i < nMaterials; i++) {
        const SGPropertyNode * node = materials.getChild(i);
        if (!strcmp(node->getName(), "material")) {
            FGNewMat * m = new FGNewMat(node);

            vector<SGPropertyNode_ptr>names = node->getChildren("name");
            for ( unsigned int j = 0; j < names.size(); j++ ) {
                string name = names[j]->getStringValue();
                m->ref();
                // cerr << "Material " << name << endl;
                matlib[name] = m;
                SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material "
                        << names[j]->getStringValue() );
            }
        } else {
            SG_LOG(SG_INPUT, SG_ALERT,
                   "Skipping bad material entry " << node->getName());
        }
    }

    // hard coded ground light state
    ssgSimpleState *gnd_lights = new ssgSimpleState;
    gnd_lights->ref();
    gnd_lights->disable( GL_TEXTURE_2D );
    gnd_lights->enable( GL_CULL_FACE );
    gnd_lights->enable( GL_COLOR_MATERIAL );
    gnd_lights->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    gnd_lights->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    gnd_lights->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    gnd_lights->enable( GL_BLEND );
    gnd_lights->disable( GL_ALPHA_TEST );
    gnd_lights->disable( GL_LIGHTING );
    matlib["GROUND_LIGHTS"] = new FGNewMat(gnd_lights);

    GLuint tex_name;

    // hard coded runway white light state
    tex_name = gen_standard_dir_light_map( 235, 235, 215, 255 );
    ssgSimpleState *rwy_white_lights = new ssgSimpleState();
    rwy_white_lights->ref();
    rwy_white_lights->disable( GL_LIGHTING );
    rwy_white_lights->enable ( GL_CULL_FACE ) ;
    rwy_white_lights->enable( GL_TEXTURE_2D );
    rwy_white_lights->enable( GL_BLEND );
    rwy_white_lights->enable( GL_ALPHA_TEST );
    rwy_white_lights->enable( GL_COLOR_MATERIAL );
    rwy_white_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_lights->setTexture( tex_name );
    matlib["RWY_WHITE_LIGHTS"] = new FGNewMat(rwy_white_lights);
    // For backwards compatibility ... remove someday
    matlib["RUNWAY_LIGHTS"] = new FGNewMat(rwy_white_lights);
    matlib["RWY_LIGHTS"] = new FGNewMat(rwy_white_lights);
    // end of backwards compatitibilty

    // hard coded runway medium intensity white light state
    tex_name = gen_standard_dir_light_map( 235, 235, 215, 205 );
    ssgSimpleState *rwy_white_medium_lights = new ssgSimpleState();
    rwy_white_medium_lights->ref();
    rwy_white_medium_lights->disable( GL_LIGHTING );
    rwy_white_medium_lights->enable ( GL_CULL_FACE ) ;
    rwy_white_medium_lights->enable( GL_TEXTURE_2D );
    rwy_white_medium_lights->enable( GL_BLEND );
    rwy_white_medium_lights->enable( GL_ALPHA_TEST );
    rwy_white_medium_lights->enable( GL_COLOR_MATERIAL );
    rwy_white_medium_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_medium_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_medium_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_medium_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_medium_lights->setTexture( tex_name );
    matlib["RWY_WHITE_MEDIUM_LIGHTS"] = new FGNewMat(rwy_white_medium_lights);

    // hard coded runway low intensity white light state
    tex_name = gen_standard_dir_light_map( 235, 235, 215, 155 );
    ssgSimpleState *rwy_white_low_lights = new ssgSimpleState();
    rwy_white_low_lights->ref();
    rwy_white_low_lights->disable( GL_LIGHTING );
    rwy_white_low_lights->enable ( GL_CULL_FACE ) ;
    rwy_white_low_lights->enable( GL_TEXTURE_2D );
    rwy_white_low_lights->enable( GL_BLEND );
    rwy_white_low_lights->enable( GL_ALPHA_TEST );
    rwy_white_low_lights->enable( GL_COLOR_MATERIAL );
    rwy_white_low_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_low_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_white_low_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_low_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_white_low_lights->setTexture( tex_name );
    matlib["RWY_WHITE_LOW_LIGHTS"] = new FGNewMat(rwy_white_low_lights);

    // hard coded runway yellow light state
    tex_name = gen_standard_dir_light_map( 235, 235, 20, 255 );
    ssgSimpleState *rwy_yellow_lights = new ssgSimpleState();
    rwy_yellow_lights->ref();
    rwy_yellow_lights->disable( GL_LIGHTING );
    rwy_yellow_lights->enable ( GL_CULL_FACE ) ;
    rwy_yellow_lights->enable( GL_TEXTURE_2D );
    rwy_yellow_lights->enable( GL_BLEND );
    rwy_yellow_lights->enable( GL_ALPHA_TEST );
    rwy_yellow_lights->enable( GL_COLOR_MATERIAL );
    rwy_yellow_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_lights->setTexture( tex_name );
    matlib["RWY_YELLOW_LIGHTS"] = new FGNewMat(rwy_yellow_lights);

    // hard coded runway medium intensity yellow light state
    tex_name = gen_standard_dir_light_map( 235, 235, 20, 205 );
    ssgSimpleState *rwy_yellow_medium_lights = new ssgSimpleState();
    rwy_yellow_medium_lights->ref();
    rwy_yellow_medium_lights->disable( GL_LIGHTING );
    rwy_yellow_medium_lights->enable ( GL_CULL_FACE ) ;
    rwy_yellow_medium_lights->enable( GL_TEXTURE_2D );
    rwy_yellow_medium_lights->enable( GL_BLEND );
    rwy_yellow_medium_lights->enable( GL_ALPHA_TEST );
    rwy_yellow_medium_lights->enable( GL_COLOR_MATERIAL );
    rwy_yellow_medium_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_medium_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_medium_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_medium_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_medium_lights->setTexture( tex_name );
    matlib["RWY_YELLOW_MEDIUM_LIGHTS"] = new FGNewMat(rwy_yellow_medium_lights);

    // hard coded runway low intensity yellow light state
    tex_name = gen_standard_dir_light_map( 235, 235, 20, 155 );
    ssgSimpleState *rwy_yellow_low_lights = new ssgSimpleState();
    rwy_yellow_low_lights->ref();
    rwy_yellow_low_lights->disable( GL_LIGHTING );
    rwy_yellow_low_lights->enable ( GL_CULL_FACE ) ;
    rwy_yellow_low_lights->enable( GL_TEXTURE_2D );
    rwy_yellow_low_lights->enable( GL_BLEND );
    rwy_yellow_low_lights->enable( GL_ALPHA_TEST );
    rwy_yellow_low_lights->enable( GL_COLOR_MATERIAL );
    rwy_yellow_low_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_low_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_yellow_low_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_low_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_yellow_low_lights->setTexture( tex_name );
    matlib["RWY_YELLOW_LOW_LIGHTS"] = new FGNewMat(rwy_yellow_low_lights);

    // hard coded runway red light state
    tex_name = gen_standard_dir_light_map( 235, 20, 20, 255 );
    ssgSimpleState *rwy_red_lights = new ssgSimpleState();
    rwy_red_lights->ref();
    rwy_red_lights->disable( GL_LIGHTING );
    rwy_red_lights->enable ( GL_CULL_FACE ) ;
    rwy_red_lights->enable( GL_TEXTURE_2D );
    rwy_red_lights->enable( GL_BLEND );
    rwy_red_lights->enable( GL_ALPHA_TEST );
    rwy_red_lights->enable( GL_COLOR_MATERIAL );
    rwy_red_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_lights->setTexture( tex_name );
    matlib["RWY_RED_LIGHTS"] = new FGNewMat(rwy_red_lights);

    // hard coded medium intensity runway red light state
    tex_name = gen_standard_dir_light_map( 235, 20, 20, 205 );
    ssgSimpleState *rwy_red_medium_lights = new ssgSimpleState();
    rwy_red_medium_lights->ref();
    rwy_red_medium_lights->disable( GL_LIGHTING );
    rwy_red_medium_lights->enable ( GL_CULL_FACE ) ;
    rwy_red_medium_lights->enable( GL_TEXTURE_2D );
    rwy_red_medium_lights->enable( GL_BLEND );
    rwy_red_medium_lights->enable( GL_ALPHA_TEST );
    rwy_red_medium_lights->enable( GL_COLOR_MATERIAL );
    rwy_red_medium_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_medium_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_medium_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_medium_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_medium_lights->setTexture( tex_name );
    matlib["RWY_RED_MEDIUM_LIGHTS"] = new FGNewMat(rwy_red_medium_lights);

    // hard coded low intensity runway red light state
    tex_name = gen_standard_dir_light_map( 235, 20, 20, 205 );
    ssgSimpleState *rwy_red_low_lights = new ssgSimpleState();
    rwy_red_low_lights->ref();
    rwy_red_low_lights->disable( GL_LIGHTING );
    rwy_red_low_lights->enable ( GL_CULL_FACE ) ;
    rwy_red_low_lights->enable( GL_TEXTURE_2D );
    rwy_red_low_lights->enable( GL_BLEND );
    rwy_red_low_lights->enable( GL_ALPHA_TEST );
    rwy_red_low_lights->enable( GL_COLOR_MATERIAL );
    rwy_red_low_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_low_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_red_low_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_low_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_red_low_lights->setTexture( tex_name );
    matlib["RWY_RED_LOW_LIGHTS"] = new FGNewMat(rwy_red_low_lights);

    // hard coded runway green light state
    tex_name = gen_standard_dir_light_map( 20, 235, 20, 255 );
    ssgSimpleState *rwy_green_lights = new ssgSimpleState();
    rwy_green_lights->ref();
    rwy_green_lights->disable( GL_LIGHTING );
    rwy_green_lights->enable ( GL_CULL_FACE ) ;
    rwy_green_lights->enable( GL_TEXTURE_2D );
    rwy_green_lights->enable( GL_BLEND );
    rwy_green_lights->enable( GL_ALPHA_TEST );
    rwy_green_lights->enable( GL_COLOR_MATERIAL );
    rwy_green_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_lights->setTexture( tex_name );
    matlib["RWY_GREEN_LIGHTS"] = new FGNewMat(rwy_green_lights);

    // hard coded medium intensity runway green light state
    tex_name = gen_standard_dir_light_map( 20, 235, 20, 205 );
    ssgSimpleState *rwy_green_medium_lights = new ssgSimpleState();
    rwy_green_medium_lights->ref();
    rwy_green_medium_lights->disable( GL_LIGHTING );
    rwy_green_medium_lights->enable ( GL_CULL_FACE ) ;
    rwy_green_medium_lights->enable( GL_TEXTURE_2D );
    rwy_green_medium_lights->enable( GL_BLEND );
    rwy_green_medium_lights->enable( GL_ALPHA_TEST );
    rwy_green_medium_lights->enable( GL_COLOR_MATERIAL );
    rwy_green_medium_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_medium_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_medium_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_medium_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_medium_lights->setTexture( tex_name );
    matlib["RWY_GREEN_MEDIUM_LIGHTS"] = new FGNewMat(rwy_green_medium_lights);

    // hard coded low intensity runway green light state
    tex_name = gen_standard_dir_light_map( 20, 235, 20, 205 );
    ssgSimpleState *rwy_green_low_lights = new ssgSimpleState();
    rwy_green_low_lights->ref();
    rwy_green_low_lights->disable( GL_LIGHTING );
    rwy_green_low_lights->enable ( GL_CULL_FACE ) ;
    rwy_green_low_lights->enable( GL_TEXTURE_2D );
    rwy_green_low_lights->enable( GL_BLEND );
    rwy_green_low_lights->enable( GL_ALPHA_TEST );
    rwy_green_low_lights->enable( GL_COLOR_MATERIAL );
    rwy_green_low_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_low_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_green_low_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_low_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_green_low_lights->setTexture( tex_name );
    matlib["RWY_GREEN_LOW_LIGHTS"] = new FGNewMat(rwy_green_low_lights);

    // hard coded runway vasi light state
    ssgSimpleState *rwy_vasi_lights = new ssgSimpleState();
    rwy_vasi_lights->ref();
    rwy_vasi_lights->disable( GL_LIGHTING );
    rwy_vasi_lights->enable ( GL_CULL_FACE ) ;
    rwy_vasi_lights->enable( GL_TEXTURE_2D );
    rwy_vasi_lights->enable( GL_BLEND );
    rwy_vasi_lights->enable( GL_ALPHA_TEST );
    rwy_vasi_lights->enable( GL_COLOR_MATERIAL );
    rwy_vasi_lights->setMaterial ( GL_AMBIENT, 1.0, 1.0, 1.0, 1.0 );
    rwy_vasi_lights->setMaterial ( GL_DIFFUSE, 1.0, 1.0, 1.0, 1.0 );
    rwy_vasi_lights->setMaterial ( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    rwy_vasi_lights->setMaterial ( GL_EMISSION, 0.0, 0.0, 0.0, 0.0 );
    rwy_vasi_lights->setTexture( gen_vasi_light_map() );
    matlib["RWY_VASI_LIGHTS"] = new FGNewMat(rwy_vasi_lights);

    return true;
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &tex_path )
{
    string material_name = tex_path;
    int pos = tex_path.rfind( "/" );
    material_name = material_name.substr( pos + 1 );

    return add_item( material_name, tex_path );
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &mat_name, const string &full_path )
{
    int pos = full_path.rfind( "/" );
    string tex_name = full_path.substr( pos + 1 );
    string tex_path = full_path.substr( 0, pos );

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material " 
	    << mat_name << " (" << full_path << ")");

    material_lib.matlib[mat_name] = new FGNewMat(full_path);

    return true;
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &mat_name, ssgSimpleState *state )
{
    FGNewMat *m = new FGNewMat(state);

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material given a premade "
	    << "ssgSimpleState = " << mat_name );

    material_lib.matlib[mat_name] = m;

    return true;
}


// find a material record by material name
FGNewMat *FGMaterialLib::find( const string& material ) {
    FGNewMat *result = NULL;
    material_map_iterator it = matlib.find( material );
    if ( it != end() ) {
	result = it->second;
	return result;
    }

    return NULL;
}


// Destructor
FGMaterialLib::~FGMaterialLib ( void ) {
    // Free up all the material entries first
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	FGNewMat *slot = it->second;
	slot->deRef();
	if ( slot->getRef() <= 0 ) {
            delete slot;
        }
    }
}


// Set the step for all of the state selectors in the material slots
void FGMaterialLib::set_step ( int step )
{
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	const string &key = it->first;
	SG_LOG( SG_GENERAL, SG_INFO,
		"Updating material " << key << " to step " << step );
	FGNewMat *slot = it->second;
	slot->get_state()->selectStep(step);
    }
}


// Get the step for the state selectors
int FGMaterialLib::get_step ()
{
  material_map_iterator it = begin();
  return it->second->get_state()->getSelectStep();
}


// Load one pending "deferred" texture.  Return true if a texture
// loaded successfully, false if no pending, or error.
void FGMaterialLib::load_next_deferred() {
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	/* we don't need the key, but here's how we'd get it if we wanted it. */
        // const string &key = it->first;
	FGNewMat *slot = it->second;
	if (slot->load_texture())
	  return;
    }
}
