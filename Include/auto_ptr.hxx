/**************************************************************************
 * auto_ptr.hxx -- A simple auto_ptr definition.
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

#ifndef _AUTO_PTR_HXX
#define _AUTO_PTR_HXX

#include "fg_stl_config.h"

//-----------------------------------------------------------------------------
//
// auto_ptr is initialised with a pointer obtained via new and deletes that
// object when it itself is destroyed (such as when leaving block scope).
// auto_ptr can be used in any way that a normal pointer can be.
//
// This class is only required when the STL doesn't supply one.
//
template <class X> class auto_ptr {
private:
    X* ptr;
    mutable bool owns;

public:
    typedef X element_type;

    explicit auto_ptr(X* p = 0) : ptr(p), owns(p) {}

    auto_ptr(const auto_ptr& a) : ptr(a.ptr), owns(a.owns) {
	a.owns = 0;
    }

#ifdef _FG_MEMBER_TEMPLATES
    template <class T> auto_ptr(const auto_ptr<T>& a)
	: ptr(a.ptr), owns(a.owns) {
	a.owns = 0;
    }
#endif

    auto_ptr& operator = (const auto_ptr& a) {
	if (&a != this) {
	    if (owns)
		delete ptr;
	    owns = a.owns;
	    ptr = a.ptr;
	    a.owns = 0;
	}
    }

#ifdef _FG_MEMBER_TEMPLATES
    template <class T> auto_ptr& operator = (const auto_ptr<T>& a) {
	if (&a != this) {
	    if (owns)
		delete ptr;
	    owns = a.owns;
	    ptr = a.ptr;
	    a.owns = 0;
	}
    }
#endif

    ~auto_ptr() {
	if (owns)
	    delete ptr;
    }

    X& operator*() const { return *ptr; }
    X* operator->() const { return ptr; }
    X* get() const { return ptr; }
    X* release() const { owns = false; return ptr; }
};

#endif /* _AUTO_PTR_HXX */

// $Log$
// Revision 1.1  1999/04/05 21:32:41  curt
// Initial revision
//
// Revision 1.2  1998/09/10 19:07:03  curt
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
// Revision 1.1  1998/08/30 14:12:45  curt
// Initial revision.  Contributed by Bernie Bright.
//
