// features.hxx - Exporting feature access to remote applications
//
// Started by Alex Perry
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


#ifndef FG_FEATURES
#define FG_FEATURES


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <time.h>
#include STL_STRING

SG_USING_NAMESPACE(std);


/*
 * FEATURES
 *
 * This class accepts static variables from arbitrary other parts
 * of the code and maintains a list of their locations and the
 * heirarchical names that the user associates with them.
 * This is essentially a runtime symbol table.  The associated
 * library dynamically builds (and rebuilds) a menu tree from this
 * information.  One or more remote console programs, that do
 * not need to be under the same operating system, can query
 * the contents of the list, request values and force changes.
 */

class FGFeature
{
public:

/* These have no effect on the value, but it should be
   considered volatile across any call to update() below
*/
   static void register_float  ( char *name, float  *value );
   static void register_double ( char *name, double *value );
   static void register_int    ( char *name, int    *value );
   static void register_char80 ( char *name, char   *value );
   static void register_string ( char *name, char   *value );

/* Note that the char80 is a fixed length string,
   statically allocated space by the registering program.
   In contrast, the string type is dynamically allocated
   and the registering program must have alloca()'d the default.
   Please don't use either of those if the numerics are usable.
*/

/* If you want to have a single variable available under two
   names, use this call.  This is equivalent to making the 
   registering call above twice, except that you don't need
   to know the memory location of the original variable.
   If you create an alias to a variable that isn't registered,
   the alias is invisible but will subsequently appear if needed.
*/
   static void register_alias ( char *name, char *value );

/* Use this when the user requests that a feature be changed
   and you have no idea where it might be located in memory.
   Expect a non-zero return code if request cannot be completed.
*/
   static int modify   ( char *name, char *newvalue );

/* Programs wishing to traverse the list of registered names will
   use the first call to find out how many names are registered,
   and then iteratively use the second call to get the names.
   The third/fourth calls will retrieve the value as a string.
*/
   static int   index_count ();
   static char *index_name  ( int index );
   static char *index_value ( int index );
   static char *get_value ( char *name );

/* It is much more efficient to build the menu if you have full
   access to the data structures.  Therefore this is implemented
   internal to the module, even though the above functions would
   suffice for an inefficient external implementation.
*/
   static void maybe_rebuild_menu();

/* This should be called regularly.  It services the connections
   to any remote clients that are monitoring or maintaining data
   about the flight gear session in progress.  This service is
   expected to operate embedded inside the NetworkOLK stuff.
*/
   static void update();

private:

   static void register_void  ( char *name, char itstype, void *value );

/* I don't have anything in mind for this section (yet).
*/


};
#endif // FG_FEATURES
