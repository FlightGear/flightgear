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

#include <string.h>

#include <simgear/compiler.h>

#include SG_GLU_H

#include <simgear/debug/logstream.hxx>
#include <simgear/screen/texture.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>

#include "globals.hxx"
#include "fg_props.hxx"
#include "splash.hxx"
#include "fg_os.hxx"



static SGTexture splash;


// Initialize the splash screen
void fgSplashInit ( const char *splash_texture ) {
    if (!fgGetBool("/sim/startup/splash-screen"))
        return;

    SG_LOG( SG_GENERAL, SG_INFO, "Initializing splash screen" );

    splash.bind();

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

    splash.read_rgb_texture(tpath.c_str());
    if (!splash.usable())
    {
	// Try compressed
	SGPath fg_tpath = tpath;
	fg_tpath.concat( ".gz" );

        splash.read_rgb_texture(fg_tpath.c_str());
	if ( !splash.usable() )
	{
	    SG_LOG( SG_GENERAL, SG_ALERT, 
		    "Error in loading splash screen texture " << tpath.str() );
	    exit(-1);
	} 
    } 

    splash.select();
}


// Update the splash screen with alpha specified from 0.0 to 1.0
void fgSplashUpdate ( float alpha ) {
    int xmin, ymin, xmax, ymax;
    int xsize = 512;
    int ysize = 512;

    if ( !fgGetInt("/sim/startup/xsize")
	 || !fgGetInt("/sim/startup/ysize") ) {
	return;
    }

    xmin = (fgGetInt("/sim/startup/xsize") - xsize) / 2;
    xmax = xmin + xsize;

    ymin = (fgGetInt("/sim/startup/ysize") - ysize) / 2;
    ymax = ymin + ysize;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, fgGetInt("/sim/startup/xsize"),
	       0, fgGetInt("/sim/startup/ysize"));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // draw the background
    glColor4f( 0.0, 0.0, 0.0, alpha );
    glBegin(GL_POLYGON);
    glVertex2f(0.0, 0.0);
    glVertex2f(fgGetInt("/sim/startup/xsize"), 0.0);
    glVertex2f(fgGetInt("/sim/startup/xsize"), fgGetInt("/sim/startup/ysize"));
    glVertex2f(0.0, fgGetInt("/sim/startup/ysize")); 
    glEnd();

    // now draw the logo
    if (fgGetBool("/sim/startup/splash-screen") && splash.usable()) {
        glEnable(GL_TEXTURE_2D);
        splash.bind();
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        glColor4f( 1.0, 1.0, 1.0, alpha );
        glBegin(GL_POLYGON);
        glTexCoord2f(0.0, 0.0); glVertex2f(xmin, ymin);
        glTexCoord2f(1.0, 0.0); glVertex2f(xmax, ymin);
        glTexCoord2f(1.0, 1.0); glVertex2f(xmax, ymax);
        glTexCoord2f(0.0, 1.0); glVertex2f(xmin, ymax); 
        glEnd();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}


