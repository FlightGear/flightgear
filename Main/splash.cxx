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

#include <Debug/logstream.hxx>
#include <Main/options.hxx>
#include <Objects/texload.h>

#include "splash.hxx"


static GLuint splash_texid;
static GLubyte *splash_texbuf;


// Initialize the splash screen
void fgSplashInit ( void ) {
    string tpath, fg_tpath;
    int width, height;

    FG_LOG( FG_GENERAL, FG_INFO, "Initializing splash screen" );
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
    tpath = current_options.get_fg_root() + "/Textures/Splash2.rgb";

    if ( (splash_texbuf = 
	  read_rgb_texture(tpath.c_str(), &width, &height)) == NULL )
    {
	// Try compressed
	fg_tpath = tpath + ".gz";
	if ( (splash_texbuf = 
	      read_rgb_texture(fg_tpath.c_str(), &width, &height)) == NULL )
	{
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "Error in loading splash screen texture " << tpath );
	    exit(-1);
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

    // first clear the screen;
    xglClearColor(0.0, 0.0, 0.0, 1.0);
    xglClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    // now draw the logo
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
// Revision 1.7  1998/11/06 21:18:14  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.6  1998/10/17 01:34:25  curt
// C++ ifying ...
//
// Revision 1.5  1998/09/26 13:17:29  curt
// Clear screen to "black" before drawing splash screen.
//
// Revision 1.4  1998/08/27 17:02:08  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.3  1998/08/25 16:59:10  curt
// Directory reshuffling.
//
// Revision 1.2  1998/07/13 21:01:40  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.1  1998/07/06 02:42:36  curt
// Initial revision.
//

