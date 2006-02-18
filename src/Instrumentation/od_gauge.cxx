// Owner Drawn Gauge helper class
//
// Written by Harald JOHNSEN, started May 2005.
//
// Copyright (C) 2005  Harald JOHNSEN
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <plib/ssg.h>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>
#include <simgear/debug/logstream.hxx>
#include SG_GLU_H

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include "od_gauge.hxx"

FGODGauge::FGODGauge() :
    rtAvailable( false ),
    rt( 0 )
{
}

// done here and not in init() so we don't allocate a rendering context if it is
// never used
void FGODGauge::allocRT () {
    GLint colorBits = 0;
    glGetIntegerv( GL_BLUE_BITS, &colorBits );
    textureWH = 256;
    rt = new RenderTexture();
    if( colorBits < 8 )
        rt->Reset("rgba=5,5,5,1 ctt");
    else
        rt->Reset("rgba ctt");

    if( rt->Initialize(256, 256, true) ) {
        SG_LOG(SG_ALL, SG_INFO, "FGODGauge:Initialize sucessfull");
        if (rt->BeginCapture())
        {
            SG_LOG(SG_ALL, SG_INFO, "FGODGauge:BeginCapture sucessfull, RTT available");
            rtAvailable = true;
            glViewport(0, 0, textureWH, textureWH);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            gluOrtho2D( -256.0, 256.0, -256.0, 256.0 );
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glDisable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glDisable(GL_CULL_FACE);
            glDisable(GL_FOG);
            glDisable(GL_DEPTH_TEST);
            glClearColor(0.0, 0.0, 0.0, 0.0);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindTexture(GL_TEXTURE_2D, 0);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.0f);
            glDisable(GL_SMOOTH);
            glEnable(GL_BLEND);
            glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
            rt->EndCapture();
        } else
            SG_LOG(SG_ALL, SG_WARN, "FGODGauge:BeginCapture failed, RTT not available, using backbuffer");
    } else
        SG_LOG(SG_ALL, SG_WARN, "FGODGauge:Initialize failed, RTT not available, using backbuffer");
}

FGODGauge::~FGODGauge() {
    delete rt;
}

void FGODGauge::init () {
}

void FGODGauge::update (double dt) {
}

void FGODGauge::beginCapture(int viewSize) {
    if( ! rt )
        allocRT();
    if(rtAvailable) {
        rt->BeginCapture();
    }
    else
        set2D();
     textureWH = viewSize;
    glViewport(0, 0, textureWH, textureWH);
}

void FGODGauge::beginCapture(void) {
    if( ! rt )
        allocRT();
    if(rtAvailable) {
        rt->BeginCapture();
    }
    else
        set2D();
}

void FGODGauge::Clear(void) {
    if(rtAvailable) {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else {
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
          glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        glRectf(-256.0, -256.0, 256.0, 256.0);
        glEnable(GL_BLEND);
        glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
        glEnable(GL_ALPHA_TEST);
    }
}

void FGODGauge::endCapture(GLuint texID) {
    glBindTexture(GL_TEXTURE_2D, texID);
    // don't use mimaps if we don't update them
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, textureWH, textureWH);
    if(rtAvailable)
        rt->EndCapture();
    else
        set3D();
    glBindTexture(GL_TEXTURE_2D, 0);
}

void FGODGauge::setSize(int viewSize) {
    textureWH = viewSize;
    glViewport(0, 0, textureWH, textureWH);
}

bool FGODGauge::serviceable(void) {
    return rtAvailable;
}

/**
 * Locate a texture SSG node in a branch.
 */
static const char *strip_path(const char *name) {
    /* Remove all leading path information. */
    const char* seps = "\\/" ;
    const char* fn = & name [ strlen ( name ) - 1 ] ;
    for ( ; fn != name && strchr(seps,*fn) == NULL ; fn-- )
        /* Search back for a seperator */ ;
    if ( strchr(seps,*fn) != NULL )
        fn++ ;
    return fn ;
}

static ssgSimpleState *
find_texture_node (ssgEntity * node, const char * name)
{
  if( node->isAKindOf( ssgTypeLeaf() ) ) {
    ssgLeaf *leaf = (ssgLeaf *) node;
    ssgSimpleState *state = (ssgSimpleState *) leaf->getState();
    if( state ) {
        ssgTexture *tex = state->getTexture();
        if( tex ) {
            const char * texture_name = tex->getFilename();
            if (texture_name) {
                texture_name = strip_path( texture_name );
                if ( !strcmp(name, texture_name) )
                    return state;
            }
        }
    }
  }
  else {
    int nKids = node->getNumKids();
    for (int i = 0; i < nKids; i++) {
      ssgSimpleState * result =
        find_texture_node(((ssgBranch*)node)->getKid(i), name);
      if (result != 0)
        return result;
    }
  } 
  return 0;
}

void FGODGauge::set_texture(const char * name, GLuint new_texture) {
    ssgEntity * root = globals->get_scenery()->get_aircraft_branch();
    name = strip_path( name );
    ssgSimpleState * node = find_texture_node( root, name );
    if( node )
        node->setTexture( new_texture );
}

void FGODGauge::set2D() {
    glPushAttrib ( GL_ENABLE_BIT | GL_VIEWPORT_BIT  | GL_TRANSFORM_BIT | GL_LIGHTING_BIT ) ;

    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_SMOOTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport ( 0, 0, textureWH, textureWH ) ;
    glMatrixMode   ( GL_PROJECTION ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;
    gluOrtho2D( -256.0, 256.0, -256.0, 256.0 );
    glMatrixMode   ( GL_MODELVIEW ) ;
    glPushMatrix   () ;
    glLoadIdentity () ;

    glAlphaFunc(GL_GREATER, 0.0f);

}

void FGODGauge::set3D() {
    glMatrixMode   ( GL_PROJECTION ) ;
    glPopMatrix    () ;
    glMatrixMode   ( GL_MODELVIEW ) ;
    glPopMatrix    () ;
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
    glPopAttrib    () ;
}
