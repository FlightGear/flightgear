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
    fgOPTIONS *o;
    char path[256], fgpath[256];
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

    fgclose(f);

    return(1);
}


// Destructor
fgMATERIAL_MGR::~fgMATERIAL_MGR ( void ) {
}


// $Log$
// Revision 1.2  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.1  1998/05/30 01:56:45  curt
// Added material.cxx material.hxx
//
