// fg_random.h -- routines to handle random number generation
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _FG_RANDOM_H
#define _FG_RANDOM_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


// Seed the random number generater with time() so we don't see the
// same sequence every time
void fg_srandom(void);

// return a random number between [0.0, 1.0)
double fg_random(void);


#ifdef __cplusplus
}
#endif


#endif // _FG_RANDOM_H


// $Log$
// Revision 1.4  1998/11/07 19:07:04  curt
// Enable release builds using the --without-logging option to the configure
// script.  Also a couple log message cleanups, plus some C to C++ comment
// conversion.
//
// Revision 1.3  1998/04/21 17:03:48  curt
// Prepairing for C++ integration.
//
// Revision 1.2  1998/01/22 02:59:38  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.1  1997/07/30 16:04:09  curt
// Moved random routines from Utils/ to Math/
//
// Revision 1.2  1997/07/23 21:52:28  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.1  1997/07/19 22:31:57  curt
// Initial revision.
//
