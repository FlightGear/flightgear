// features.cxx - Exporting simulator features
//
// Copyright (C) 2000  Alexander Perry - alex.perry@ieee.org
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

#if defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#else
#  include <iostream>
#endif

#include <simgear/math/sg_types.hxx>

FG_USING_NAMESPACE(std);

#include "features.hxx"



////////////////////////////////////////////////////////////////////////
// Declare how to do registrations
////////////////////////////////////////////////////////////////////////

   void FGFeature::register_float  ( char *name, float  *value )
			{ register_void ( name, 'F', (void*) value ); };

   void FGFeature::register_double ( char *name, double *value )
			{ register_void ( name, 'D', (void*) value ); };

   void FGFeature::register_int    ( char *name, int    *value )
			{ register_void ( name, 'I', (void*) value ); };

   void FGFeature::register_char80 ( char *name, char   *value )
			{ register_void ( name, 'C', (void*) value ); };

   void FGFeature::register_string ( char *name, char   *value )
			{ register_void ( name, 'A', (void*) value ); };

   void FGFeature::register_alias  ( char *name, char   *value )
			{ register_void ( name, '-', (void*) value ); };



////////////////////////////////////////////////////////////////////////
// Everything else stubbed out
////////////////////////////////////////////////////////////////////////

   void FGFeature::register_void ( char *name, char itstype, void *value )
			{ return; };

   int FGFeature::modify   ( char *name, char *newvalue )
	{ return 1; /* failed */ };

   int   FGFeature::index_count ( void ) { return 0; };
   char* FGFeature::index_name  ( int index ) { return NULL; };
   char* FGFeature::index_value ( int index ) { return NULL; };
   char* FGFeature::get_value ( char *name )  { return NULL; };

   void  FGFeature::maybe_rebuild_menu() {};

   void  FGFeature::update() {};

// end of features.cxx
