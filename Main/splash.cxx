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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <string.h>

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Main/options.hxx>
#include <Objects/texload.h>

#include "splash.hxx"


static GLuint splash_texid;
static GLubyte *splash_texbuf;


// Initialize the splash screen
void fgSplashInit ( void ) {
    char tpath[256], fg_tpath[256];
    int width, height;

    fgPrintf( FG_GENERAL, FG_INFO, "Initializing splash screen\n");
#ifdef GL_VERSION_1_1
    xglGenTextures(1, &splash_texid);
    xglBindTexture(GL_TEXTURE_2D, splash_texid);
#elif GL_EXT_texture_object
    xglGenTexturesEXT(1, &splash_texid);
    xglBindTextureEXT(GL_TEXTURE_2D, splash_texid);
#else
#  error port me
#endif

    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    xglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // load in the texture data
    current_options.get_fg_root(tpath);
    strcat(tpath, "/Textures/");
    strcat(tpath, "Splash2.rgb");

    if ( (splash_texbuf = read_rgb_texture(tpath, &width, &height)) == NULL ) {
	// Try compressed
	strcpy(fg_tpath, tpath);
	strcat(fg_tpath, ".gz");
	if ( (splash_texbuf = read_rgb_texture(fg_tpath, &width, &height)) 
	     == NULL ) {
	    fgPrintf( FG_GENERAL, FG_EXIT, 
		      "Error in loading splash screen texture %s\n", tpath );
	} 
    } 

    xglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, 
		   GL_UNSIGNED_BYTE, (GLvoid *)(splash_texbuf) );
}


// Update the splash screen with progress specified from 0.0 to 1.0
void fgSplashUpdate ( double progress ) {
    int xmin, ymin, xmax, ymax;
    int xsize = 480;
    int ysize = 380;

    xmin = (640 - xsize) / 2;
    xmax = xmin + xsize;

    ymin = (480 - ysize) / 2;
    ymax = ymin + ysize;

    xglMatrixMode(GL_PROJECTION);
    xglPushMatrix();
    xglLoadIdentity();
    gluOrtho2D(0, 640, 0, 480);
    xglMatrixMode(GL_MODELVIEW);
    xglPushMatrix();
    xglLoadIdentity();

    xglDisable(GL_DEPTH_TEST);
    xglDisable(GL_LIGHTING);
    xglEnable(GL_TEXTURE_2D);
#ifdef GL_VERSION_1_1
    xglBindTexture(GL_TEXTURE_2D, splash_texid);
#elif GL_EXT_texture_object
    xglBindTextureEXT(GL_TEXTURE_2D, splash_texid);
#else
#  error port me
#endif
    xglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    xglBegin(GL_POLYGON);
    xglTexCoord2f(0.0, 0.0); glVertex2f(xmin, ymin);
    xglTexCoord2f(1.0, 0.0); glVertex2f(xmax, ymin);
    xglTexCoord2f(1.0, 1.0); glVertex2f(xmax, ymax);
    xglTexCoord2f(0.0, 1.0); glVertex2f(xmin, ymax); 
    xglEnd();

    xglutSwapBuffers();

    xglEnable(GL_DEPTH_TEST);
    xglEnable(GL_LIGHTING);
    xglDisable(GL_TEXTURE_2D);

    xglMatrixMode(GL_PROJECTION);
    xglPopMatrix();
    xglMatrixMode(GL_MODELVIEW);
    xglPopMatrix();
}


// $Log$
// Revision 1.3  1998/08/25 16:59:10  curt
// Directory reshuffling.
//
// Revision 1.2  1998/07/13 21:01:40  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.1  1998/07/06 02:42:36  curt
// Initial revision.
//

