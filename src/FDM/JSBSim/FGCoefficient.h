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
#else
#  include <string>
#  include <fstream>
#endif

/*******************************************************************************
DEFINES
*******************************************************************************/

using namespace std;

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
FORWARD DECLARATIONS
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

/*******************************************************************************
COMMENTS, REFERENCES,  and NOTES
*******************************************************************************/
/**
This class models the stability derivative coefficient lookup tables or 
equations. Note that the coefficients need not be calculated each delta-t.

The coefficient files are located in the axis subdirectory for each aircraft.
For instance, for the X-15, you would find subdirectories under the
aircraft/X-15/ directory named CLIFT, CDRAG, CSIDE, CROLL, CPITCH, CYAW. Under
each of these directories would be files named a, a0, q, and so on. The file
named "a" under the CLIFT directory would contain data for the stability
derivative modeling lift due to a change in alpha. See the FGAircraft.cpp file
for additional information. The coefficient files have the following format:

<name of coefficient>
<short description of coefficient with no embedded spaces>
<method used in calculating the coefficient: TABLE | EQUATION | VECTOR | VALUE>
  <parameter identifier for table row (if required)>
  <parameter identifier for table column (if required)>
<OR'ed list of parameter identifiers needed to turn this coefficient into a force>
<number of rows in table (if required)>
<number of columns in table (if required)>

<value of parameter indexing into the column of a table or vector - or value
  itself for a VALUE coefficient>
<values of parameter indexing into row of a table if TABLE type> <Value of
  coefficient at this row and column>

<... repeat above for each column of data in table ...>

As an example for the X-15, for the lift due to mach:
<PRE>

CLa0
Lift_at_zero_alpha
Table 8 3
16384
32768
16387

0.00
0.0 0.0
0.5 0.4
0.9 0.9
1.0 1.6
1.1 1.3
1.4 1.0
2.0 0.5
3.0 0.5

30000.00
0.0 0.0
0.5 0.5
0.9 1.0
1.0 1.7
1.1 1.4
1.4 1.1
2.0 0.6
3.0 0.6

70000.00
0.0 0.0
0.5 0.6
0.9 1.1
1.0 1.7
1.1 1.5
1.4 1.2
2.0 0.7
3.0 0.7
</PRE>

Note that the values in a row which index into the table must be the same value
for each column of data, so the first column of numbers for each altitude are
seen to be equal, and there are the same number of values for each altitude.

<PRE>
FG_QBAR         1
FG_WINGAREA     2
FG_WINGSPAN     4
FG_CBAR         8
FG_ALPHA       16
FG_ALPHADOT    32
FG_BETA        64
FG_BETADOT    128
FG_PITCHRATE  256
FG_ROLLRATE   512
FG_YAWRATE   1024
FG_ELEVATOR  2048
FG_AILERON   4096
FG_RUDDER    8192
FG_MACH     16384
FG_ALTITUDE 32768L
</PRE>
@author Jon S. Berndt
@memo This class models the stability derivative coefficient lookup tables or equations.
*/
/*******************************************************************************
CLASS DECLARATION
*******************************************************************************/

class FGCoefficient
{
public:
  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  FGCoefficient(FGFDMExec*, ifstream&);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  ~FGCoefficient(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  bool Allocate(int);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  bool Allocate(int, int);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float Value(float, float);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float Value(float);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float Value(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  float TotalValue(void);

  // ***************************************************************************
  /** @memo
      @param
      @return
  */
  enum Type {UNKNOWN, VALUE, VECTOR, TABLE, EQUATION};

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
  Type type;
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
