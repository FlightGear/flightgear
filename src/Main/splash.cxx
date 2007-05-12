// splash.cxx -- draws the initial splash screen
//
// Written by Curtis Olson, started July 1998.  (With a little looking
// at Freidemann's panel code.) :-)
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <string.h>

#include <plib/pu.h>
#include <simgear/compiler.h>

#include SG_GLU_H

#include <simgear/debug/logstream.hxx>
#include <simgear/screen/texture.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include <GUI/new_gui.hxx>

#include "globals.hxx"
#include "fg_props.hxx"
#include "splash.hxx"
#include "fg_os.hxx"
#include "renderer.hxx"

static const char *progress_text = 0;
static SGTexture *splash = new SGTexture;
SGPropertyNode_ptr style = 0;


// Initialize the splash screen
void fgSplashInit ( const char *splash_texture ) {
    fgRequestRedraw();

    SG_LOG( SG_GENERAL, SG_INFO, "Initializing splash screen" );

    style = fgGetNode("/sim/gui/style[0]", true);

    if (!fgGetBool("/sim/startup/splash-screen"))
        return;

    splash->bind();

    SGPath tpath( globals->get_fg_root() );
    if (splash_texture == NULL || !strcmp(splash_texture, "")) {
        // load in the texture data
        int num = (int)(sg_random() * 5.0 + 1.0);
        char num_str[5];
        snprintf(num_str, 4, "%d", num);

        tpath.append( "Textures/Splash" );
        tpath.concat( num_str );
        tpath.concat( ".rgb" );
    } else
        tpath.append( splash_texture );

    splash->read_rgba_texture(tpath.c_str());
    if (!splash->usable())
    {
        // Try compressed
        SGPath fg_tpath = tpath;
        fg_tpath.concat( ".gz" );

        splash->read_rgba_texture(fg_tpath.c_str());
        if ( !splash->usable() )
        {
            SG_LOG( SG_GENERAL, SG_ALERT,
                    "Error in loading splash screen texture " << tpath.str() );
            exit(-1);
        }
    }

    splash->select();
}


void fgSplashExit ()
{
    delete splash;
    splash = 0;
}


void fgSplashProgress ( const char *s )
{
    progress_text = s;
    fgRequestRedraw();
}


// Update the splash screen with alpha specified from 0.0 to 1.0
void fgSplashUpdate ( float alpha ) {
    const int EMPTYSPACE = 80;

    int screen_width = fgGetInt("/sim/startup/xsize", 0);
    int screen_height = fgGetInt("/sim/startup/ysize", 0);

    if (!screen_width || !screen_height)
        return;

    globals->get_renderer()->resize(screen_width, screen_height);
    int size = screen_width < (screen_height - EMPTYSPACE)
            ? screen_width : screen_height - EMPTYSPACE;
    if (size > 512)
        size = 512;

    int xmin, ymin, xmax, ymax;
    xmin = (screen_width - size) / 2;
    xmax = xmin + size;

    ymin = (screen_height - size) / 2;
    ymax = ymin + size;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screen_width, 0, screen_height);
    glViewport(0, 0, screen_width, screen_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    // draw the background
    FGColor c(0.0, 0.0, 0.0);
    c.merge(style->getNode("colors/splash-screen"));
    glColor4f(c.red(), c.green(), c.blue(), alpha );
    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2f(screen_width, 0.0);
    glVertex2f(screen_width, screen_height);
    glVertex2f(0.0, screen_height);
    glEnd();

    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.1f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // now draw the logo
    if (splash && fgGetBool("/sim/startup/splash-screen", true)) {
        glEnable(GL_TEXTURE_2D);
        splash->bind();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glColor4f( 1.0, 1.0, 1.0, alpha );
        glBegin(GL_POLYGON);
        glTexCoord2f(0.0, 0.0); glVertex2f(xmin, ymin);
        glTexCoord2f(1.0, 0.0); glVertex2f(xmax, ymin);
        glTexCoord2f(1.0, 1.0); glVertex2f(xmax, ymax);
        glTexCoord2f(0.0, 1.0); glVertex2f(xmin, ymax);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    if (progress_text && fgGetBool("/sim/startup/splash-progress", true)) {
        puFont *fnt = globals->get_fontcache()->get(style->getNode("fonts/splash"));

        FGColor c(1.0, 0.9, 0.0);
        c.merge(style->getNode("colors/splash-font"));
        glColor4f(c.red(), c.green(), c.blue(), alpha);

        float height = fnt->getStringHeight("/M$");
        float descender = fnt->getStringDescender();
        float width = fnt->getFloatStringWidth(progress_text);
        fnt->drawString(progress_text, int((screen_width - width) / 2), int(10 + descender));

        const char *title = fgGetString("/sim/startup/splash-title", "");
        width = fnt->getFloatStringWidth(title);
        fnt->drawString(title, int((screen_width - width) / 2), int(screen_height - 10 - height));
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


