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

HISTORY
--------------------------------------------------------------------------------
12/28/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include "FGCoefficient.h"
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

FGCoefficient::FGCoefficient(FGFDMExec* fdex, ifstream& coeffDefFile)
{
  int r, c;
  float ftrashcan;
  string strashcan;

  FDMExec     = fdex;
  State       = FDMExec->GetState();
  Atmosphere  = FDMExec->GetAtmosphere();
  FCS         = FDMExec->GetFCS();
  Aircraft    = FDMExec->GetAircraft();
  Translation = FDMExec->GetTranslation();
  Rotation    = FDMExec->GetRotation();
  Position    = FDMExec->GetPosition();
  Auxiliary   = FDMExec->GetAuxiliary();
  Output      = FDMExec->GetOutput();

  if (coeffDefFile) {
    if (!coeffDefFile.fail()) {
      coeffDefFile >> name;
      cout << "   " << name << endl;
      coeffDefFile >> strashcan;
      coeffDefFile >> description;
      cout << "   " << description << endl;
      coeffDefFile >> method;
      cout << "   " << method << endl;

      if      (method == "EQUATION") type = EQUATION;
      else if (method == "TABLE")    type = TABLE;
      else if (method == "VECTOR")   type = VECTOR;
      else if (method == "VALUE")    type = VALUE;
      else                           type = UNKNOWN;

      if (type == VECTOR || type == TABLE) {
        coeffDefFile >> rows;
        cout << "   Rows: " << rows << " ";
        if (type == TABLE) {
          coeffDefFile >> columns;
          cout << "Cols: " << columns;
        }
        coeffDefFile >> LookupR;
        cout << endl;
        cout << "   Row indexing parameter: " << LookupR << endl;
      }

      if (type == TABLE) {
        coeffDefFile >> LookupC;
        cout << "   Column indexing parameter: " << LookupC << endl;
      }

      coeffDefFile >> multipliers;
      cout << "   Non-Dimensionalized by: ";
      
      mult_count = 0;
      if (multipliers & FG_QBAR) {
        mult_idx[mult_count] = FG_QBAR;
        mult_count++;
        cout << "qbar ";
      }
      if (multipliers & FG_WINGAREA) {
        mult_idx[mult_count] = FG_WINGAREA;
        mult_count++;
        cout << "S ";
      }
      if (multipliers & FG_WINGSPAN) {
        mult_idx[mult_count] = FG_WINGSPAN;
        mult_count++;
        cout << "b ";
      }
      if (multipliers & FG_CBAR) {
        mult_idx[mult_count] = FG_CBAR;
        mult_count++;
        cout << "c ";
      }
      if (multipliers & FG_ALPHA) {
        mult_idx[mult_count] = FG_ALPHA;
        mult_count++;
        cout << "alpha ";
      }
      if (multipliers & FG_ALPHADOT) {
        mult_idx[mult_count] = FG_ALPHADOT;
        mult_count++;
        cout << "alphadot ";
      }
      if (multipliers & FG_BETA) {
        mult_idx[mult_count] = FG_BETA;
        mult_count++;
        cout << "beta ";
      }
      if (multipliers & FG_BETADOT) {
        mult_idx[mult_count] = FG_BETADOT;
        mult_count++;
        cout << "betadot ";
      }
      if (multipliers & FG_PITCHRATE) {
        mult_idx[mult_count] = FG_PITCHRATE;
        mult_count++;
        cout << "q ";
      }
      if (multipliers & FG_ROLLRATE) {
        mult_idx[mult_count] = FG_ROLLRATE;
        mult_count++;
        cout << "p ";
      }
      if (multipliers & FG_YAWRATE) {
        mult_idx[mult_count] = FG_YAWRATE;
        mult_count++;
        cout << "r ";
      }
      if (multipliers & FG_ELEVATOR) {
        mult_idx[mult_count] = FG_ELEVATOR;
        mult_count++;
        cout << "De ";
      }
      if (multipliers & FG_AILERON) {
        mult_idx[mult_count] = FG_AILERON;
        mult_count++;
        cout << "Da ";
      }
      if (multipliers & FG_RUDDER) {
        mult_idx[mult_count] = FG_RUDDER;
        mult_count++;
        cout << "Dr ";
      }
      if (multipliers & FG_MACH) {
        mult_idx[mult_count] = FG_MACH;
        mult_count++;
        cout << "Mach ";
      }
      if (multipliers & FG_ALTITUDE) {
        mult_idx[mult_count] = FG_ALTITUDE;
        mult_count++;
        cout << "h ";
      }
			cout << endl;
			
      switch(type) {
      case VALUE:
        coeffDefFile >> StaticValue;
        break;
      case VECTOR:
        Allocate(rows,2);

        for (r=1;r<=rows;r++) {
          coeffDefFile >> Table3D[r][0];
          coeffDefFile >> Table3D[r][1];
        }

        for (r=0;r<=rows;r++) {
        	cout << "	";
        	for (c=0;c<=columns;c++) {
        		cout << Table3D[r][c] << "	";
        	}
        	cout << endl;
        }

        break;
      case TABLE:
        Allocate(rows, columns);

        for (c=1;c<=columns;c++) {
          coeffDefFile >> Table3D[0][c];
          for (r=1;r<=rows;r++) {
            if ( c==1 ) coeffDefFile >> Table3D[r][0];
            else coeffDefFile >> ftrashcan;
            coeffDefFile >> Table3D[r][c];
          }
        }

        for (r=0;r<=rows;r++) {
        	cout << "	";
        	for (c=0;c<=columns;c++) {
        		cout << Table3D[r][c] << "	";
        	}
        	cout << endl;
        }
        
        break;
      }
    } else {
      cerr << "Empty file" << endl;
    }
  }
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
  
//cout << "Value for " << description << " is " << Value;
 
  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

//cout << " after multipliers it is: " << Value << endl;

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

// cout << "Value for " << description << " is " << Value;
 
  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

//cout << " after multipliers it is: " << Value << endl;

  return Value;
}


float FGCoefficient::Value(void)
{
	float Value;
	int midx;
	
	Value = StaticValue;

// cout << "Value for " << description << " is " << Value << endl;
 
  for (midx=0;midx<mult_count;midx++) {
    Value *= GetCoeffVal(mult_idx[midx]);
  }

// cout << " after multipliers it is: " << Value << endl;

  return Value;
}

float FGCoefficient::TotalValue()
{
  switch(type) {
  case 0:
    return -1;
  case 1:
    return (Value());
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
//cout << "Qbar: " << State->Getqbar() << endl;
    return State->Getqbar();
  case FG_WINGAREA:
//cout << "S: " << Aircraft->GetWingArea() << endl;  
    return Aircraft->GetWingArea();
  case FG_WINGSPAN:
//cout << "b: " << Aircraft->GetWingSpan() << endl;
    return Aircraft->GetWingSpan();
  case FG_CBAR:
//cout << "Cbar: " << Aircraft->Getcbar() << endl;
    return Aircraft->Getcbar();
  case FG_ALPHA:
//cout << "Alpha: " << Translation->Getalpha() << endl;
    return Translation->Getalpha();
  case FG_ALPHADOT:
//cout << "Adot: " << State->Getadot() << endl;
    return State->Getadot();
  case FG_BETA:
//cout << "Beta: " << Translation->Getbeta() << endl;
    return Translation->Getbeta();
  case FG_BETADOT:
//cout << "Bdot: " << State->Getbdot() << endl;
    return State->Getbdot();
  case FG_PITCHRATE:
//cout << "Q: " << Rotation->GetQ() << endl;
    return Rotation->GetQ();
  case FG_ROLLRATE:
//cout << "P: " << Rotation->GetP() << endl;
    return Rotation->GetP();
  case FG_YAWRATE:
//cout << "R: " << Rotation->GetR() << endl;
    return Rotation->GetR();
  case FG_ELEVATOR:
//cout << "De: " << FCS->GetDe() << endl;
    return FCS->GetDe();
  case FG_AILERON:
//cout << "Da: " << FCS->GetDa() << endl;
    return FCS->GetDa();
  case FG_RUDDER:
//cout << "Dr: " << FCS->GetDr() << endl;
    return FCS->GetDr();
  case FG_MACH:
//cout << "Mach: " << State->GetMach() << endl;
    return State->GetMach();
  case FG_ALTITUDE:
//cout << "h: " << State->Geth() << endl;
    return State->Geth();
  }
  return 0;
}
