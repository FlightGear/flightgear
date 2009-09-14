/**************************************************************************
 * compiler.h -- C++ Compiler Portability Macros
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

#ifndef _COMPILER_H
#define _COMPILER_H

// What this file does.
//  (1)  Defines macros for some STL includes which may be affected
//       by file name length limitations.
//  (2)  Defines macros for some features not supported by all C++ compilers.
//  (3)  Defines 'explicit' as a null macro if the compiler doesn't support
//       the explicit keyword.
//  (4)  Defines 'typename' as a null macro if the compiler doesn't support
//       the typename keyword.
//  (5)  Defines bool, true and false if the compiler doesn't do so.
//  (6)  Defines FG_EXPLICIT_FUNCTION_TMPL_ARGS if the compiler
//       supports calling a function template by providing its template
//       arguments explicitly.
//  (7)  Defines FG_NEED_AUTO_PTR if STL doesn't provide auto_ptr<>.
//  (8)  Defines FG_NO_ARROW_OPERATOR if the compiler is unable
//       to support operator->() for iterators.
//  (9)  Defines FG_USE_EXCEPTIONS if the compiler supports exceptions.
//       Note: no FlightGear code uses exceptions.
//  (10) Define FG_NAMESPACES if the compiler supports namespaces.
//  (11) FG_MATH_FN_IN_NAMESPACE_STD -- not used??
//  (12) Define FG_HAVE_STD if std namespace is supported.
//  (13) Defines FG_CLASS_PARTIAL_SPECIALIZATION if the compiler 
//       supports partial specialization of class templates.
//  (14) Defines FG_HAVE_STD_INCLUDES to use ISO C++ Standard headers.
//  (15) Defines FG_HAVE_STREAMBUF if <streambuf> of <streambuf.h> are present.
//  (16) Define FG_MATH_EXCEPTION_CLASH if math.h defines an exception class
//       that clashes with the one defined in <stdexcept>.

#ifdef __GNUC__
#  if __GNUC__ == 2 
#    if __GNUC_MINOR__ < 8

       // g++-2.7.x
#      define STL_ALGORITHM  <algorithm>
#      define STL_FUNCTIONAL <functional>
#      define STL_IOMANIP    <iomanip.h>
#      define STL_IOSTREAM   <iostream.h>
#      define STL_STDEXCEPT  <stdexcept>
#      define STL_STRING     <string>
#      define STL_STRSTREAM  <strstream.h>

#      define FG_NEED_AUTO_PTR
#      define FG_NO_DEFAULT_TEMPLATE_ARGS
#      define FG_INCOMPLETE_FUNCTIONAL
#      define FG_NO_ARROW_OPERATOR

#    elif __GNUC_MINOR__ >= 8

       // g++-2.8.x and egcs-1.x
#      define FG_EXPLICIT_FUNCTION_TMPL_ARGS
#      define FG_NEED_AUTO_PTR
#      define FG_MEMBER_TEMPLATES
#      define FG_NAMESPACES
#      define FG_HAVE_STD
#      define FG_HAVE_STREAMBUF
#      define FG_CLASS_PARTIAL_SPECIALIZATION

#      define STL_ALGORITHM  <algorithm>
#      define STL_FUNCTIONAL <functional>
#      define STL_IOMANIP    <iomanip>
#      define STL_IOSTREAM   <iostream>
#      define STL_STDEXCEPT  <stdexcept>
#      define STL_STRING     <string>
#      define STL_STRSTREAM  <strstream>

#    endif
#  else
#    error Time to upgrade. GNU compilers < 2.7 not supported
#  endif
#endif

//
// Metrowerks 
//
#if defined(__MWERKS__)
/*
  CodeWarrior compiler from Metrowerks, Inc.
*/
#  if (__MWERKS__ < 0x0900) //|| macintosh
#    define FG_HAVE_TRAITS
#    define FG_HAVE_STD_INCLUDES
#    define FG_HAVE_STD
#    define FG_NAMESPACES

#    define STL_ALGORITHM  <algorithm>
#    define STL_FUNCTIONAL <functional>
#    define STL_IOMANIP    <iomanip>
#    define STL_IOSTREAM   <iostream>
#    define STL_STDEXCEPT  <stdexcept>
#    define STL_STRING     <string>

// Temp:
#    define bcopy(from, to, n) memcpy(to, from, n)

// -rp- please use FG_MEM_COPY everywhere !
#    define FG_MEM_COPY(to,from,n) memcpy(to, from, n)

#  elif (__MWERKS__ >= 0x0900) && __INTEL__
#    error still to be supported...
#  else
#    error unknown Metrowerks compiler
#  endif
#endif

//
// Microsoft compilers.
//
#ifdef _MSC_VER
#  if _MSC_VER == 1200 // msvc++ 6.0
#    define FG_NAMESPACES
#    define FG_HAVE_STD
#    define FG_HAVE_STD_INCLUDES
#    define FG_HAVE_STREAMBUF

#    define STL_ALGORITHM  <algorithm>
#    define STL_FUNCTIONAL <functional>
#    define STL_IOMANIP    <iomanip>
#    define STL_IOSTREAM   <iostream>
#    define STL_STDEXCEPT  <stdexcept>
#    define STL_STRING     <string>
#    define STL_STRSTREAM  <strstream>

#    pragma warning(disable: 4786) // identifier was truncated to '255' characters
#    pragma warning(disable: 4244) // conversion from double to float
#    pragma warning(disable: 4305) //

#  elif _MSC_VER == 1100 // msvc++ 5.0
#    error MSVC++ 5.0 still to be supported...
#  else
#    error What version of MSVC++ is this?
#  endif
#endif

#ifdef __BORLANDC__
# if defined(HAVE_SGI_STL_PORT)

// Use quotes around long file names to get around Borland's include hackery

#  define STL_ALGORITHM  "algorithm"
#  define STL_FUNCTIONAL "functional"

#  define FG_MATH_EXCEPTION_CLASH

# else

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRING     <string>
#  define STL_STRSTREAM  <strstream>

#  define FG_INCOMPLETE_FUNCTIONAL

# endif // HAVE_SGI_STL_PORT

#  define FG_NO_DEFAULT_TEMPLATE_ARGS
#  define FG_NAMESPACES
#  define FG_HAVE_STD

#endif // __BORLANDC__

//
// Native SGI compilers
//

#if defined ( sgi ) && !defined( __GNUC__ )
#  define FG_HAVE_NATIVE_SGI_COMPILERS

#  define FG_EXPLICIT_FUNCTION_TMPL_ARGS
#  define FG_NEED_AUTO_PTR
#  define FG_MEMBER_TEMPLATES
#  define FG_NAMESPACES
#  define FG_HAVE_STD
#  define FG_CLASS_PARTIAL_SPECIALIZATION

#  define STL_ALGORITHM  <algorithm>
#  define STL_FUNCTIONAL <functional>
#  define STL_IOMANIP    <iomanip>
#  define STL_IOSTREAM   <iostream.h>
#  define STL_STDEXCEPT  <stdexcept>
#  define STL_STRING     <string>
#  define STL_STRSTREAM  <strstream>

#endif // Native SGI compilers


#if defined ( sun )
#  include <memory.h>
#  if defined ( __cplusplus )
     // typedef unsigned int size_t;
     extern "C" {
       extern void *memmove(void *, const void *, size_t);
     }
#  else
     extern void *memmove(void *, const void *, size_t);
#  endif // __cplusplus
#endif // sun

//
// No user modifiable definitions beyond here.
//

#ifdef FG_NEED_EXPLICIT
#  define explicit
#endif

#ifdef FG_NEED_TYPENAME
#  define typename
#endif

#ifdef FG_NEED_MUTABLE
#  define mutable
#endif

#ifdef FG_NEED_BOOL
   typedef int bool;
#  define true 1
#  define false 0
#endif

#ifdef FG_EXPLICIT_FUNCTION_TMPL_ARGS
#  define FG_NULL_TMPL_ARGS <>
#else
#  define FG_NULL_TMPL_ARGS
#endif

#ifdef FG_CLASS_PARTIAL_SPECIALIZATION
# define FG_TEMPLATE_NULL template<>
#else
# define FG_TEMPLATE_NULL
#endif

// FG_NO_NAMESPACES is a hook so that users can disable namespaces
// without having to edit library headers.
#if defined(FG_NAMESPACES) && !defined(FG_NO_NAMESPACES)
#   define FG_NAMESPACE(X) namespace X {
#   define FG_NAMESPACE_END }
#   define FG_USING_NAMESPACE(X) using namespace X
# else
#   define FG_NAMESPACE(X)
#   define FG_NAMESPACE_END
#   define FG_USING_NAMESPACE(X)
#endif

# ifdef FG_HAVE_STD
#  define FG_USING_STD(X) using std::X
#  define STD std
# else
#  define FG_USING_STD(X) 
#  define STD
# endif

// Additional <functional> implementation from SGI STL 3.11
// Adapter function objects: pointers to member functions
#ifdef FG_INCOMPLETE_FUNCTIONAL

template <class _Ret, class _Tp>
class const_mem_fun_ref_t
#ifndef __BORLANDC__
    : public unary_function<_Tp,_Ret>
#endif // __BORLANDC__
{
public:
  explicit const_mem_fun_ref_t(_Ret (_Tp::*__pf)() const) : _M_f(__pf) {}
  _Ret operator()(const _Tp& __r) const { return (__r.*_M_f)(); }
private:
  _Ret (_Tp::*_M_f)() const;
};

template <class _Ret, class _Tp>
inline const_mem_fun_ref_t<_Ret,_Tp> mem_fun_ref(_Ret (_Tp::*__f)() const)
  { return const_mem_fun_ref_t<_Ret,_Tp>(__f); }

#endif // FG_INCOMPLETE_FUNCTIONAL

#endif // _COMPILER_H

// $Log$
// Revision 1.9  1999/03/02 00:36:31  curt
// Tweaks for native SGI compilers.
//
// Revision 1.8  1999/02/26 22:07:10  curt
// Added initial support for native SGI compilers.
//
// Revision 1.7  1999/02/05 21:27:41  curt
// Tweaks for the solaris platform.
//
// Revision 1.6  1999/01/27 04:45:17  curt
// Tweak for solaris.
//
// Revision 1.5  1999/01/19 20:41:25  curt
// Added support for MacOS (Metrowerks)
//
// Revision 1.4  1999/01/06 21:47:37  curt
// renamed general.h to general.hxx
// More portability enhancements to compiler.h
//
// Revision 1.3  1998/11/06 14:04:09  curt
// More portability improvements by Bernie Bright.
//
// Revision 1.2  1998/11/02 18:28:08  curt
// Portability updates from Bernie Bright
//
// Revision 1.1  1998/10/16 00:49:04  curt
// fg_stl_config.h -> compiler.h, fg_stl_config.h maintained for backwards
// compatibility.
//
// Revision 1.3  1998/09/29 02:00:16  curt
// Start of some borland c support
//
// Revision 1.2  1998/09/10 19:07:04  curt
// /Simulator/Objects/fragment.hxx
//   Nested fgFACE inside fgFRAGMENT since its not used anywhere else.
//
// ./Simulator/Objects/material.cxx
// ./Simulator/Objects/material.hxx
//   Made fgMATERIAL and fgMATERIAL_MGR bona fide classes with private
//   data members - that should keep the rabble happy :)
//
// ./Simulator/Scenery/tilemgr.cxx
//   In viewable() delay evaluation of eye[0] and eye[1] in until they're
//   actually needed.
//   Change to fgTileMgrRender() to call fgMATERIAL_MGR::render_fragments()
//   method.
//
// ./Include/fg_stl_config.h
// ./Include/auto_ptr.hxx
//   Added support for g++ 2.7.
//   Further changes to other files are forthcoming.
//
// Brief summary of changes required for g++ 2.7.
//   operator->() not supported by iterators: use (*i).x instead of i->x
//   default template arguments not supported,
//   <functional> doesn't have mem_fun_ref() needed by callbacks.
//   some std include files have different names.
//   template member functions not supported.
//
// Revision 1.1  1998/08/30 14:13:49  curt
// Initial revision.  Contributed by Bernie Bright.
//
