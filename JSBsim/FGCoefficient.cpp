/*******************************************************************************

 Module:       FGCoefficient.cpp
 Author:       Jon S. Berndt
 Date started: 12/28/98
 Purpose:      Encapsulates the stability derivative class FGCoefficient;
 Called by:    FGAircraft

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

Note that the values in a row which index into the table must be the same value
for each column of data, so the first column of numbers for each altitude are
seen to be equal, and there are the same number of values for each altitude.

See the header file FGCoefficient.h for the values of the identifiers.

ARGUMENTS
--------------------------------------------------------------------------------


HISTORY
--------------------------------------------------------------------------------
12/28/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

class FGCoefficient;
#include <stdio.h>
#include <stdlib.h>
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGCoefficient.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGCoefficient::FGCoefficient(void)
{
  rows = columns = 0;
}


FGCoefficient::FGCoefficient(char* fname)
{
  int r, c;
  float ftrashcan;

  ifstream coeffDefFile(fname);

  if (coeffDefFile) {
    if (!coeffDefFile.fail()) {
      coeffDefFile >> name;
      coeffDefFile >> description;
      coeffDefFile >> method;

      if (strcmp(method,"EQUATION") == 0) type = 4;
      else if (strcmp(method,"TABLE") == 0) type = 3;
      else if (strcmp(method,"VECTOR") == 0) type = 2;
      else if (strcmp(method,"VALUE") == 0) type = 1;
      else type = 0;

      if (type == 2 || type == 3) {
        coeffDefFile >> rows;
        if (type == 3) {
          coeffDefFile >> columns;
        }
        coeffDefFile >> LookupR;
      }

      if (type == 3) {
        coeffDefFile >> LookupC;
      }

      coeffDefFile >> multipliers;

      mult_count = 0;
      if (multipliers & FG_QBAR) {
        mult_idx[mult_count] = FG_QBAR;
        mult_count++;
      }
      if (multipliers & FG_WINGAREA) {
        mult_idx[mult_count] = FG_WINGAREA;
        mult_count++;
      }
      if (multipliers & FG_WINGSPAN) {
        mult_idx[mult_count] = FG_WINGSPAN;
        mult_count++;
      }
      if (multipliers & FG_CBAR) {
        mult_idx[mult_count] = FG_CBAR;
        mult_count++;
      }
      if (multipliers & FG_ALPHA) {
        mult_idx[mult_count] = FG_ALPHA;
        mult_count++;
      }
      if (multipliers & FG_ALPHADOT) {
        mult_idx[mult_count] = FG_ALPHADOT;
        mult_count++;
      }
      if (multipliers & FG_BETA) {
        mult_idx[mult_count] = FG_BETA;
        mult_count++;
      }
      if (multipliers & FG_BETADOT) {
        mult_idx[mult_count] = FG_BETADOT;
        mult_count++;
      }
      if (multipliers & FG_PITCHRATE) {
        mult_idx[mult_count] = FG_PITCHRATE;
        mult_count++;
      }
      if (multipliers & FG_ROLLRATE) {
        mult_idx[mult_count] = FG_ROLLRATE;
        mult_count++;
      }
      if (multipliers & FG_YAWRATE) {
        mult_idx[mult_count] = FG_YAWRATE;
        mult_count++;
      }
      if (multipliers & FG_ELEVATOR) {
        mult_idx[mult_count] = FG_ELEVATOR;
        mult_count++;
      }
      if (multipliers & FG_AILERON) {
        mult_idx[mult_count] = FG_AILERON;
        mult_count++;
      }
      if (multipliers & FG_RUDDER) {
        mult_idx[mult_count] = FG_RUDDER;
        mult_count++;
      }
      if (multipliers & FG_MACH) {
        mult_idx[mult_count] = FG_MACH;
        mult_count++;
      }
      if (multipliers & FG_ALTITUDE) {
        mult_idx[mult_count] = FG_ALTITUDE;
        mult_count++;
      }

      switch(type) {
      case 1:
        coeffDefFile >> StaticValue;
        break;
      case 2:
        Allocate(rows,2);

        for (r=1;r<=rows;r++) {
          coeffDefFile >> Table3D[r][0];
          coeffDefFile >> Table3D[r][1];
        }
        break;
      case 3:
        Allocate(rows, columns);

        for (c=1;c<=columns;c++) {
          coeffDefFile >> Table3D[0][c];
          for (r=1;r<=rows;r++) {
            if ( c==1 ) coeffDefFile >> Table3D[r][0];
            else coeffDefFile >> ftrashcan;
            coeffDefFile >> Table3D[r][c];
          }
        }
        break;
      }
    } else {
      cerr << "Empty file" << endl;
    }
    coeffDefFile.close();
  }
}


FGCoefficient::FGCoefficient(int r, int c)
{
  rows = r;
  columns = c;
  Allocate(r,c);
}


FGCoefficient::FGCoefficient(int n)
{
  rows = n;
  columns = 0;
  Allocate(n);
}


FGCoefficient::~FGCoefficient(void)
{
}


bool FGCoefficient::Allocate(int r, int c)
{
  rows = r;
  columns = c;
  Table3D = new float*[r+1];
  for (int i=0;i<=r;i++) Table3D[i] = new float[c+1];
  return true;
}


bool FGCoefficient::Allocate(int n)
{
  rows = n;
  columns = 0;
  Table2D = new float[n+1];
  return true;
}


float FGCoefficient::Value(float rVal, float cVal)
{
  float rFactor, cFactor, col1temp, col2temp, Value;
  int r, c, midx;

  if (rows < 2 || columns < 2) return 0.0;

  for (r=1;r<=rows;r++) if (Table3D[r][0] >= rVal) break;
  for (c=1;c<=columns;c++) if (Table3D[0][c] >= cVal) break;

  c = c < 2 ? 2 : (c > columns ? columns : c);
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  rFactor = (rVal - Table3D[r-1][0]) / (Table3D[r][0] - Table3D[r-1][0]);
  cFactor = (cVal - Table3D[0][c-1]) / (Table3D[0][c] - Table3D[0][c-1]);

  col1temp = rFactor*(Table3D[r][c-1] - Table3D[r-1][c-1]) + Table3D[r-1][c-1];
  col2temp = rFactor*(Table3D[r][c] - Table3D[r-1][c]) + Table3D[r-1][c];

  Value = col1temp + cFactor*(col2temp - col1temp);

  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

  return Value;
}


float FGCoefficient::Value(float Val)
{
  float Factor, Value;
  int r, midx;

  if (rows < 2) return 0.0;
  
  for (r=1;r<=rows;r++) if (Table3D[r][0] >= Val) break;
  r = r < 2 ? 2 : (r > rows    ? rows    : r);

  // make sure denominator below does not go to zero.
  if (Table3D[r][0] != Table3D[r-1][0]) {
    Factor = (Val - Table3D[r-1][0]) / (Table3D[r][0] - Table3D[r-1][0]);
  } else {
    Factor = 1.0;
  }
  Value = Factor*(Table3D[r][1] - Table3D[r-1][1]) + Table3D[r-1][1];

  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

  return Value;
}


float FGCoefficient::Value()
{
  switch(type) {
  case 0:
    return -1;
  case 1:
    return (StaticValue);
  case 2:
    return (Value(GetCoeffVal(LookupR)));
  case 3:
    return (Value(GetCoeffVal(LookupR),GetCoeffVal(LookupC)));
  case 4:
    return 0.0;
  }
  return 0;
}

float FGCoefficient::GetCoeffVal(int val_idx)
{
  switch(val_idx) {
  case FG_QBAR:
    return State->Getqbar();
  case FG_WINGAREA:
    return Aircraft->GetWingArea();
  case FG_WINGSPAN:
    return Aircraft->GetWingSpan();
  case FG_CBAR:
    return Aircraft->Getcbar();
  case FG_ALPHA:
    return State->Getalpha();
  case FG_ALPHADOT:
    return State->Getadot();
  case FG_BETA:
    return State->Getbeta();
  case FG_BETADOT:
    return State->Getbdot();
  case FG_PITCHRATE:
    return State->GetQ();
  case FG_ROLLRATE:
    return State->GetP();
  case FG_YAWRATE:
    return State->GetR();
  case FG_ELEVATOR:
    return FCS->GetDe();
  case FG_AILERON:
    return FCS->GetDa();
  case FG_RUDDER:
    return FCS->GetDr();
  case FG_MACH:
    return State->GetMach();
  case FG_ALTITUDE:
    return State->Geth();
  }
  return 0;
}
