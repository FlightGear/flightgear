// fg_memory.h -- memcpy/bcopy portability declarations
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


#ifndef _FG_MEMORY_H
#define _FG_MEMORY_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_MEMCPY

#  ifdef HAVE_MEMORY_H
#    include <memory.h>
#  endif

#  define fgmemcmp            memcmp
#  define fgmemcpy            memcpy
#  define fgmemzero(dest,len) memset(dest,0,len)

#elif defined(HAVE_BCOPY)

#  define fgmemcmp              bcmp
#  define fgmemcpy(dest,src,n)  bcopy(src,dest,n)
#  define fgmemzero             bzero

#else

/* 
 * Neither memcpy() or bcopy() available.
 * Use substitutes provided be zlib.
 */

#  include <zlib/zutil.h>
#  define fgmemcmp zmemcmp
#  define fgmemcpy zmemcpy
#  define fgmemzero zmemzero

#endif

#endif // _FG_MEMORY_H


// $Log$
// Revision 1.2  1998/12/09 18:47:39  curt
// Use C++ style comments.
//
// Revision 1.1  1998/12/07 21:07:25  curt
// Memory related portability improvements.
//
