/*******************************************************************************

 Header:       FGTank.h
 Author:       Jon S. Berndt
 Date started: 01/21/99

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

Based on Flightgear code, which is based on LaRCSim. This class simulates
a generic Tank.

HISTORY
--------------------------------------------------------------------------------
01/21/99   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGTank_H
#define FGTank_H

/*******************************************************************************
INCLUDES
*******************************************************************************/
#ifdef FGFS
#  include <Include/compiler.h>
#  include STL_STRING
#  ifdef FG_HAVE_STD_INCLUDES
#    include <fstream>
#  else
#    include <fstream.h>
#  endif
   FG_USING_STD(string);
#  ifdef FG_HAVE_NATIVE_SGI_COMPILERS
     FG_USING_NAMESPACE(std);
#  endif
#else
#  include <string>
#  include <fstream>
#endif

/*******************************************************************************
DEFINES
*******************************************************************************/

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGTank
{
public:
  FGTank(ifstream&);
  ~FGTank(void);

  float Reduce(float);
  int GetType(void) {return Type;}
  bool GetSelected(void) {return Selected;}
  float GetPctFull(void) {return PctFull;}
  float GetContents(void) {return Contents;}

  enum TankType {ttUNKNOWN, ttFUEL, ttOXIDIZER};
  
private:
  TankType Type;
  float X, Y, Z;
  float Capacity;
  float Radius;
  float PctFull;
  float Contents;
  bool  Selected;

protected:
};

/******************************************************************************/
#endif
