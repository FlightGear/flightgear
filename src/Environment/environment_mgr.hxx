// environment-mgr.hxx -- manager for natural environment information.
//
// Written by David Megginson, started February 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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

#ifndef _ENVIRONMENT_MGR_HXX
#define _ENVIRONMENT_MGR_HXX

#include <simgear/compiler.h>

#include <Main/fgfs.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

class FGEnvironment;
class FGEnvironmentCtrl;


/**
 * Manage environment information.
 */
class FGEnvironmentMgr : public FGSubsystem
{

public:

  FGEnvironmentMgr ();
  virtual ~FGEnvironmentMgr ();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);

  /**
   * Get the environment information for the plane's current position.
   */
  virtual FGEnvironment getEnvironment () const;


  /**
   * Get the environment information for another location.
   */
  virtual FGEnvironment getEnvironment (double lat, double lon,
					double alt) const;

private:

  FGEnvironment * _environment;	// always the same, for now
  FGEnvironmentCtrl * _controller; // always the same, for now

};

#endif // _ENVIRONMENT_MGR_HXX
