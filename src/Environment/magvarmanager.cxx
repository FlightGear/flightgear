// magvarmanager.cxx -- Wraps the SimGear SGMagVar in a subsystem
//
// Copyright (C) 2012  James Turner - zakalawe@mac.com
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "magvarmanager.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

FGMagVarManager::FGMagVarManager() :
    _magVar(new SGMagVar)
{
}

FGMagVarManager::~FGMagVarManager()
{
}

void FGMagVarManager::init()
{
    update(0.0); // force an immediate update
}

void FGMagVarManager::bind()
{
  _magVarNode = fgGetNode("/environment/magnetic-variation-deg", true);
  _magDipNode = fgGetNode("/environment/magnetic-dip-deg", true);
}

void FGMagVarManager::unbind()
{
  _magVarNode = SGPropertyNode_ptr();
  _magDipNode = SGPropertyNode_ptr();
}

void FGMagVarManager::update(double dt)
{
  SG_UNUSED(dt);
    
  // update magvar model
  _magVar->update( globals->get_aircraft_position(),
                     globals->get_time_params()->getJD() );
  
  _magVarNode->setDoubleValue(_magVar->get_magvar() * SG_RADIANS_TO_DEGREES);
  _magDipNode->setDoubleValue(_magVar->get_magdip() * SG_RADIANS_TO_DEGREES);
    
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGMagVarManager> registrantFGMagVarManager;
