/*******************************************************************************

 Header:       FGAuxiliary.h
 Author:       Jon Berndt
 Date started: 01/26/99

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
11/22/98   JSB   Created
*******************************************************************************/

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGAUXILIARY_H
#define FGAUXILIARY_H

/*******************************************************************************
INCLUDES
*******************************************************************************/
#include "FGModel.h"

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAuxiliary : public FGModel
{
public:
  FGAuxiliary(void);
  ~FGAuxiliary(void);
   
  bool Run(void);

protected:

private:

};

#ifndef FDM_MAIN
extern FGAuxiliary* Auxiliary;
#else
FGAuxiliary* Auxiliary;
#endif

/******************************************************************************/
#endif
