// fdm_shell.hxx -- encapsulate FDM implementations as well-behaved subsystems
//
// Written by James Turner, started June 2010.
//
// Copyright (C) 2010  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifndef FG_FDM_SHELL_HXX
#define FG_FDM_SHELL_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include "TankProperties.hxx"

// forward decls
class FGInterface;

/**
 * Wrap an FDM implementation in a subsystem with standard semantics
 * Notably, deal with the various cases in which update() should not
 * be called, such as replay or before scenery has loaded
 *
 * This class also provides the factory method which creates the
 * specific FDM class (createImplementation)
 */
class FDMShell : public SGSubsystem
{
public:
  FDMShell();
  virtual ~FDMShell();
  
  virtual void init();
  virtual void reinit();
  
  virtual void bind();
  virtual void unbind();
  
  virtual void update(double dt);

private:

  void createImplementation();
  
  TankPropertiesList _tankProperties;
  FGInterface* _impl;
  SGPropertyNode_ptr _props; // root property tree for this FDM instance
  bool _dataLogging;
  
  SGPropertyNode_ptr _wind_north, _wind_east,_wind_down;
  SGPropertyNode_ptr _control_fdm_atmo,_temp_degc,_pressure_inhg;
  SGPropertyNode_ptr _density_slugft, _data_logging, _replay_master;
};

#endif // of FG_FDM_SHELL_HXX
