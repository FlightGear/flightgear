/*******************************************************************************

 Module:       FGAtmosphere.cpp
 Author:       Jon Berndt
 Date started: 11/24/98
 Purpose:      Models the atmosphere
 Called by:    FGSimExec

 ------------- Copyright (C) 1999  Jon S. Berndt (jsb@hal-pc.org) -------------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------
Models the atmosphere. The equation used below was determined by a third order
curve fit using Excel. The data is from the ICAO atmosphere model.

HISTORY
--------------------------------------------------------------------------------
11/24/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGAtmosphere.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGAtmosphere::FGAtmosphere(FGFDMExec* fdmex) : FGModel(fdmex)
{
  strcpy(Name,"FGAtmosphere");
}


FGAtmosphere::~FGAtmosphere()
{
}


bool FGAtmosphere::Run(void)
{
  if (!FGModel::Run()) {                 // if false then execute this Run()
    rho = 0.002377 - 7.0E-08*State->Geth()
        + 7.0E-13*State->Geth()*State->Geth()
        - 2.0E-18*State->Geth()*State->Geth()*State->Geth();

    State->SetMach(State->GetVt()/State->Geta()); 
  } else {                               // skip Run() execution this time
  }
  return false;
}
