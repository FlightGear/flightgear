/*******************************************************************************

 Module:       FGFDMExec.cpp
 Author:       Jon S. Berndt
 Date started: 11/17/98
 Purpose:      Schedules and runs the model routines.
 Called by:    The GUI.

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

This class wraps up the simulation scheduling routines.


ARGUMENTS
--------------------------------------------------------------------------------


HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGFDMExec.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <time.h>

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/


// Constructor

FGFDMExec::FGFDMExec(void)
{
  FirstModel = 0L;

  State       = new FGState();
  Atmosphere  = new FGAtmosphere();
  FCS         = new FGFCS();
  Aircraft    = new FGAircraft();
  Translation = new FGTranslation();
  Rotation    = new FGRotation();
  Position    = new FGPosition();
  Auxiliary   = new FGAuxiliary();
  Output      = new FGOutput();

  Schedule(Atmosphere,  5);
  Schedule(FCS,         1);
  Schedule(Aircraft,    1);
  Schedule(Rotation,    1);
  Schedule(Translation, 1);
  Schedule(Position,    1);
  Schedule(Auxiliary,   1);
  Schedule(Output,      5);

  terminate = false;
  freeze = false;
}


FGFDMExec::~FGFDMExec(void)
{
}


int FGFDMExec::Schedule(FGModel* model, int rate)
{
  FGModel* model_iterator;

  model_iterator = FirstModel;

  if (model_iterator == 0L) {                  // this is the first model

    FirstModel = model;
    FirstModel->NextModel = 0L;
    FirstModel->SetRate(rate);

  } else {                                     // subsequent model

    while (model_iterator->NextModel != 0L) {
      model_iterator = model_iterator->NextModel;
    }
    model_iterator->NextModel = model;
    model_iterator->NextModel->SetRate(rate);

  }
  return 0;
}


bool FGFDMExec::Run(void)
{
  FGModel* model_iterator;

  model_iterator = FirstModel;
  if (model_iterator == 0L) return false;

  while (!model_iterator->Run())
  {
    model_iterator = model_iterator->NextModel;
    if (model_iterator == 0L) break;
  }

  return true;
}

