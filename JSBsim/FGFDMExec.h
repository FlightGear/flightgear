/*******************************************************************************

 Header:       FGFDMExec.h
 Author:       Jon Berndt
 Date started: 11/17/98

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

HISTORY
--------------------------------------------------------------------------------
11/17/98   JSB   Created
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGFDMEXEC_HEADER_H
#define FGFDMEXEC_HEADER_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"
#include "FGOutput.h"

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFDMExec
{
public:
   FGFDMExec::FGFDMExec(void);
   FGFDMExec::~FGFDMExec(void);

   FGModel* FirstModel;

   bool Initialize(void);
   int  Schedule(FGModel* model, int rate);
   bool Run(void);
   bool Freeze(void);
   bool Resume(void);

private:
   bool freeze;
   bool terminate;

protected:
};

#ifndef FDM_MAIN
extern FGFDMExec* FDMExec;
#else
FGFDMExec* FDMExec;
#endif

/******************************************************************************/
#endif
