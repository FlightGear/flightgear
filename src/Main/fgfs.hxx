// fgfs.hxx -- top level include file for FlightGear.
//
// Written by David Megginson, started 2000-12
//
// Copyright (C) 2000  David Megginson, david@megginson.com
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


#ifndef __FGFS_HXX
#define __FGFS_HXX 1


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#  include <float.h>                    
#endif



/**
 * Basic interface for all FlightGear subsystems.
 */
class FGSubsystem
{
public:
  virtual ~FGSubsystem ();

  virtual void init () = 0;
  virtual void bind () = 0;
  virtual void unbind () = 0;
  virtual void update () = 0;
};


#endif // __FGFS_HXX

// end of fgfs.hxx
