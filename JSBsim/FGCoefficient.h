/*******************************************************************************

 Header:       FGCoefficient.h
 Author:       Jon Berndt
 Date started: 12/28/98

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
12/28/98   JSB   Created

********************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGCOEFFICIENT_H
#define FGCOEFFICIENT_H

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
#define FG_QBAR         1
#define FG_WINGAREA     2
#define FG_WINGSPAN     4
#define FG_CBAR         8
#define FG_ALPHA       16
#define FG_ALPHADOT    32
#define FG_BETA        64
#define FG_BETADOT    128
#define FG_PITCHRATE  256
#define FG_ROLLRATE   512
#define FG_YAWRATE   1024
#define FG_ELEVATOR  2048
#define FG_AILERON   4096
#define FG_RUDDER    8192
#define FG_MACH     16384
#define FG_ALTITUDE 32768L

/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGFDMExec;
class FGState;
class FGAtmosphere;
class FGFCS;
class FGAircraft;
class FGTranslation;
class FGRotation;
class FGPosition;
class FGAuxiliary;
class FGOutput;

class FGCoefficient
{
public:
  FGCoefficient(FGFDMExec*);
  FGCoefficient(FGFDMExec*, int, int);
  FGCoefficient(FGFDMExec*, int);
  FGCoefficient(FGFDMExec*, string);
  ~FGCoefficient(void);

  bool Allocate(int);
  bool Allocate(int, int);

  float Value(float, float);
  float Value(float);
  float Value(void);

protected:

private:
  string filename;
  string description;
  string name;
  string method;
  float StaticValue;
  float *Table2D;
  float **Table3D;
  float LookupR, LookupC;
  long int mult_idx[10];
  int rows, columns;
  int type;
  int multipliers;
  int mult_count;

  float GetCoeffVal(int);

  FGFDMExec*      FDMExec;
  FGState*        State;
  FGAtmosphere*   Atmosphere;
  FGFCS*          FCS;
  FGAircraft*     Aircraft;
  FGTranslation*  Translation;
  FGRotation*     Rotation;
  FGPosition*     Position;
  FGAuxiliary*    Auxiliary;
  FGOutput*       Output;
};

/******************************************************************************/
#endif
