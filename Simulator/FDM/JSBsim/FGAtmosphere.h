/*******************************************************************************

 Header:       FGAtmosphere.h
 Author:       Jon Berndt
 Date started: 11/24/98

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
11/24/98   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGATMOSPHERE_H
#define FGATMOSPHERE_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include "FGModel.h"

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/
/**
The equation used in this model was determined by a third order curve fit using
Excel. The data is from the ICAO atmosphere model.
@memo Models the atmosphere.
@author Jon S. Berndt
*/
/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGAtmosphere : public FGModel
{
public:
  // ***************************************************************************
  /** @memo Constructor
      @param FGFDMExec* - a pointer to the "owning" FDM Executive
  */
  FGAtmosphere(FGFDMExec*);

  // ***************************************************************************
  /** @memo Destructor
  */
  ~FGAtmosphere(void);

  // ***************************************************************************
  /** This must be called for each dt to execute the model algorithm */
  bool Run(void);

  // ***************************************************************************
  /** @memo Returns the air density
      @return float air density in slugs/cubic foot
  */
  inline float Getrho(void) {return rho;}

protected:

private:
  float rho;
};

/******************************************************************************/
#endif
