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

#include <Debug/fg_debug.h>
#include <Include/fg_zlib.h>
#include <Main/options.hxx>

#include "material.hxx"


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
    char path[256], fgpath[256];
    char line[256], *line_ptr;
    fgFile f;

    o = &current_options;

    // build the path name to the material db
    path[0] = '\0';
    strcat(path, o->fg_root);
    strcat(path, "/Scenery/");
    strcat(path, "Materials");
    strcpy(fgpath, path);
    strcat(fgpath, ".gz");

    // first try "path.gz"
    if ( (f = fgopen(fgpath, "rb")) == NULL ) {
        // next try "path"    
        if ( (f = fgopen(path, "rb")) == NULL ) {
            fgPrintf(FG_GENERAL, FG_EXIT, "Cannot open file: %s\n", path);
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
	} else if ( strncmp(line_ptr, "texture", 7) == 0 ) {
	    line_ptr += 7;
	    while ( ( (line_ptr[0] == ' ') || (line_ptr[0] == '\t') || 
		      (line_ptr[0] == '=') ) &&
		    (line_ptr[0] != '\n') ) {
		line_ptr++;
	    }
	    // printf("texture name = %s\n", line_ptr);
	    sscanf(line_ptr, "%s\n", m.texture_name);
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
