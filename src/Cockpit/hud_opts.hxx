// hud_opts.hxx -- hud optimization tools
//
// Probably written by Norman Vine, started sometime in 1998 or 1999.
//
// Copyright (C) 1999  FlightGear Project
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _HUD_OPTS_HXX
#define _HUD_OPTS_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>


#if defined(i386)
#define USE_X86_ASM
#endif

#if defined(USE_X86_ASM)
static __inline__ int FloatToInt(float f)
{
   int r;
   __asm__ ("fistpl %0" : "=m" (r) : "t" (f) : "st");
   return r;
}
#elif  defined(__MSC__) && defined(__WIN32__)
static __inline int FloatToInt(float f)
{
   int r;
   _asm {
     fld f
     fistp r
    }
   return r;
}
#else
#define FloatToInt(F) ((int) ((F) < 0.0f ? (F)-0.5f : (F)+0.5f))
#endif


#endif // _HUD_OPTS_H
