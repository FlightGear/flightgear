/**************************************************************************
 * fg_stl_config.hxx -- STL Portability Macros
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/

#ifndef _FG_STL_CONFIG_H
#define _FG_STL_CONFIG_H

// What this file does.
//  (1) Defines macros for some STL includes which may be affected
//      by file name length limitations.
//  (2) Defines macros for some features not supported by all C++ compilers.
//  (3) Defines 'explicit' as a null macro if the compiler doesn't support
//      the explicit keyword.
//  (4) Defines 'typename' as a null macro if the compiler doesn't support
//      the typename keyword.
//  (5) Defines bool, true and false if the compiler doesn't do so.
//  (6) Defines _FG_EXPLICIT_FUNCTION_TMPL_ARGS if the compiler
//      supports calling a function template by providing its template
//      arguments explicitly.
//  (7) Defines _FG_NEED_AUTO_PTR if STL doesn't provide auto_ptr<>.

#ifdef __GNUC__
#  if __GNUC__ == 2 && __GNUC_MINOR__ >= 8
     // g++-2.8.x and egcs-1.0.x
#    define STL_ALGORITHM  <algorithm>
#    define STL_FUNCTIONAL <functional>
#    define STL_IOMANIP    <iomanip>
#    define STL_IOSTREAM   <iostream>
#    define STL_STDEXCEPT  <stdexcept>
#    define STL_STRING     <string>
#    define STL_STRSTREAM  <strstream>

#    define _FG_EXPLICIT_FUNCTION_TMPL_ARGS
#    define _FG_NEED_AUTO_PTR

#  else 
#    error Old GNU compilers not yet supported
#  endif
#endif

// Microsoft compilers.
#ifdef _MSC_VER
#  if _MSC_VER < 1100
#    define _FG_NEED_EXPLICIT
#  endif
#endif

#ifdef _FG_NEED_EXPLICIT
#  define explicit
#endif

#ifdef _FG_NEED_TYPENAME
#  define typename
#endif

#ifdef _FG_NEED_BOOL
   typedef int bool;
#  define true 1
#  define false 0
#endif

#ifdef _FG_EXPLICIT_FUNCTION_TMPL_ARGS
#  define _FG_NULL_TMPL_ARGS <>
#else
#  define _FG_NULL_TMPL_ARGS
#endif

#endif // _FG_STL_CONFIG_H

// $Log$
// Revision 1.1  1998/08/30 14:13:49  curt
// Initial revision.  Contributed by Bernie Bright.
//
