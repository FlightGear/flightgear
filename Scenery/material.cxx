// material.cxx -- class to handle material properties
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

#include <Debug/fg_debug.h>
#include <Include/fg_zlib.h>
#include <Main/options.hxx>

#include "material.hxx"
#include "texload.h"

// global material management class
fgMATERIAL_MGR material_mgr;


// Constructor
fgMATERIAL::fgMATERIAL ( void ) {
}


// Sorting routines
void fgMATERIAL::init_sort_list( void );


int fgMATERIAL::append_sort_list( fgFRAGMENT *object );


// Destructor
fgMATERIAL::~fgMATERIAL ( void ) {
}


// Constructor
fgMATERIAL_MGR::fgMATERIAL_MGR ( void ) {
}


// Load a library of material properties
int fgMATERIAL_MGR::load_lib ( void ) {
    fgMATERIAL m;
    fgOPTIONS *o;
    char material_name[256];
    char mpath[256], fg_mpath[256], tpath[256], fg_tpath[256];
    char line[256], *line_ptr, value[256];
    GLubyte *texbuf;
    fgFile f;
    int width, height;
    int alpha;

    o = &current_options;

    // build the path name to the material db
    mpath[0] = '\0';
    strcat(mpath, o->fg_root);
    strcat(mpath, "/Scenery/");
    strcat(mpath, "Materials");
    strcpy(fg_mpath, mpath);
    strcat(fg_mpath, ".gz");

    // first try "path.gz"
    if ( (f = fgopen(fg_mpath, "rb")) == NULL ) {
        // next try "path"    
        if ( (f = fgopen(mpath, "rb")) == NULL ) {
            fgPrintf(FG_GENERAL, FG_EXIT, "Cannot open file: %s\n", mpath);
        }       
    }

    while ( fggets(f, line, 250) != NULL ) {
        // printf("%s", line);

	// strip leading white space
	line_ptr = line;
	while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') ) &&
		(line_ptr[0] != '\n') ) {
	    line_ptr++;
	    
	}

        if ( line_ptr[0] == '#' ) {
	    // ignore lines that start with '#'
	} else if ( line_ptr[0] == '\n' ) {
	    // ignore blank lines
	} else if ( strstr(line_ptr, "{") ) {
	    // start of record
	    alpha = 0;
	    m.ambient[0]  = m.ambient[1]  = m.ambient[2]  = m.ambient[3]  = 0.0;
	    m.diffuse[0]  = m.diffuse[1]  = m.diffuse[2]  = m.diffuse[3]  = 0.0;
	    m.specular[0] = m.specular[1] = m.specular[2] = m.specular[3] = 0.0;
	    m.emissive[0] = m.emissive[1] = m.emissive[2] = m.emissive[3] = 0.0;

	    material_name[0] = '\0';
	    sscanf(line_ptr, "%s", material_name);
	    if ( ! strlen(material_name) ) {
		fgPrintf( FG_TERRAIN, FG_INFO, "Bad material name in '%s'\n",
			  line );
	    }
	    printf("  Loading material = %s\n", material_name);
	} else if ( strncmp(line_ptr, "alpha", 5) == 0 ) {
	    line_ptr += 5;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    sscanf(line_ptr, "%s\n", value);
	    if ( strcmp(value, "no") == 0 ) {
		alpha = 0;
	    } else if ( strcmp(value, "yes") == 0 ) {
		alpha = 1;
	    } else {
		fgPrintf( FG_TERRAIN, FG_INFO, "Bad alpha value '%s'\n", line );
	    }
	} else if ( strncmp(line_ptr, "texture", 7) == 0 ) {
	    line_ptr += 7;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    // printf("texture name = %s\n", line_ptr);
	    sscanf(line_ptr, "%s\n", m.texture_name);

	    // create the texture object and bind it
#ifdef GL_VERSION_1_1
	    xglGenTextures(1, &m.texture_id);
	    xglBindTexture(GL_TEXTURE_2D, m.texture_id);
#elif GL_EXT_texture_object
	    xglGenTexturesEXT(1, &m.texture_id);
	    xglBindTextureEXT(GL_TEXTURE_2D, m.texture_id);
#else
#  error port me
#endif

	    // set the texture parameters for this texture
	    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) ;
	    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;
	    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			      /* GL_LINEAR */ 
			      /* GL_NEAREST_MIPMAP_LINEAR */
			      GL_LINEAR_MIPMAP_LINEAR ) ;
	    xglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
	    xglHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST ) ;

	    /* load in the texture data */
	    tpath[0] = '\0';
	    strcat(tpath, o->fg_root);
	    strcat(tpath, "/Textures/");
	    strcat(tpath, m.texture_name);
	    strcat(tpath, ".rgb");

	    if ( alpha == 0 ) {
		// load rgb texture

		// Try uncompressed
		if ( (texbuf = read_rgb_texture(tpath, &width, &height))
		     == NULL ) {
		    // Try compressed
		    strcpy(fg_tpath, tpath);
		    strcat(fg_tpath, ".gz");
		    if ( (texbuf = read_rgb_texture(fg_tpath, &width, &height)) 
			 == NULL ) {
			fgPrintf( FG_GENERAL, FG_EXIT, 
				  "Error in loading texture %s\n", tpath );
			return(0);
		    } 
		} 

		/* xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
			      GL_RGB, GL_UNSIGNED_BYTE, texbuf); */

		gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, width, height, 
				    GL_RGB, GL_UNSIGNED_BYTE, texbuf );
	    } else if ( alpha == 1 ) {
		// load rgba (alpha) texture

		// Try uncompressed
		if ( (texbuf = read_alpha_texture(tpath, &width, &height))
		     == NULL ) {
		    // Try compressed
		    strcpy(fg_tpath, tpath);
		    strcat(fg_tpath, ".gz");
		    if ((texbuf = read_alpha_texture(fg_tpath, &width, &height))
			== NULL ) {
			fgPrintf( FG_GENERAL, FG_EXIT, 
				  "Error in loading texture %s\n", tpath );
			return(0);
		    } 
		} 

		xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			      GL_RGBA, GL_UNSIGNED_BYTE, texbuf);
	    }

	} else if ( strncmp(line_ptr, "ambient", 7) == 0 ) {
	    line_ptr += 7;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    sscanf( line_ptr, "%f %f %f %f", 
		    &m.ambient[0], &m.ambient[1], &m.ambient[2], &m.ambient[3]);
	} else if ( strncmp(line_ptr, "diffuse", 7) == 0 ) {
	    line_ptr += 7;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    sscanf( line_ptr, "%f %f %f %f", 
		    &m.diffuse[0], &m.diffuse[1], &m.diffuse[2], &m.diffuse[3]);
	} else if ( strncmp(line_ptr, "specular", 8) == 0 ) {
	    line_ptr += 8;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    sscanf( line_ptr, "%f %f %f %f", 
		    &m.specular[0], &m.specular[1], 
		    &m.specular[2], &m.specular[3]);
	} else if ( strncmp(line_ptr, "emissive", 8) == 0 ) {
	    line_ptr += 8;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    sscanf( line_ptr, "%f %f %f %f", 
		    &m.emissive[0], &m.emissive[1], 
		    &m.emissive[2], &m.emissive[3]);
	} else if ( line_ptr[0] == '}' ) {
	    // end of record, lets add this one to the list
	    material_mgr.material_map[material_name] = m;
	} else {
	    fgPrintf(FG_TERRAIN, FG_INFO, 
		     "Unknown line in material properties file\n");
	}
    }

    fgclose(f);

    return(1);
}


// Initialize the transient list of fragments for each material property
void fgMATERIAL_MGR::init_transient_material_lists( void ) {
    map < string, fgMATERIAL, less<string> > :: iterator mapcurrent = 
	material_mgr.material_map.begin();
    map < string, fgMATERIAL, less<string> > :: iterator maplast = 
	material_mgr.material_map.end();

    while ( mapcurrent != maplast ) {
        // (char *)key = (*mapcurrent).first;
        // (fgMATERIAL)value = (*mapcurrent).second;
	(*mapcurrent).second.list_size = 0;

        *mapcurrent++;
    }
}


// Destructor
fgMATERIAL_MGR::~fgMATERIAL_MGR ( void ) {
}


// $Log$
// Revision 1.7  1998/07/04 00:54:28  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.6  1998/06/27 16:54:59  curt
// Check for GL_VERSION_1_1 or GL_EXT_texture_object to decide whether to use
//   "EXT" versions of texture management routines.
//
// Revision 1.5  1998/06/17 21:36:39  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.4  1998/06/12 00:58:04  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
//
// Revision 1.3  1998/06/05 22:39:53  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.2  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.1  1998/05/30 01:56:45  curt
// Added material.cxx material.hxx
//
