/*******************************************************************************

 Module:       FGOutput.cpp
 Author:       Jon Berndt
 Date started: 12/02/98
 Purpose:      Manage output of sim parameters to file or stdout
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
This is the place where you create output routines to dump data for perusal
later. Some machines may not support the ncurses console output. Borland is one
of those environments which does not, so the ncurses stuff is commented out.

HISTORY
--------------------------------------------------------------------------------
12/02/98   JSB   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#ifdef FGFS
#  include <Include/compiler.h>
#  ifdef FG_HAVE_STD_INCLUDES
#    include <iostream>
#  else
#    include <iostream.h>
#  endif
#else
#  include <iostream>
#endif

#ifdef HAVE_CURSES
  #include <ncurses.h>
#endif

#include "FGOutput.h"
#include "FGState.h"
#include "FGFDMExec.h"
#include "FGAtmosphere.h"
#include "FGFCS.h"
#include "FGAircraft.h"
#include "FGTranslation.h"
#include "FGRotation.h"
#include "FGPosition.h"
#include "FGAuxiliary.h"

/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

FGOutput::FGOutput(FGFDMExec* fdmex) : FGModel(fdmex)
{
  Name = "FGOutput";
  FirstPass = true;
#ifdef HAVE_CURSES
  initscr();
  cbreak();
  noecho();
#endif
}


FGOutput::~FGOutput(void)
{
}


bool FGOutput::Run(void)
{
  if (!FGModel::Run()) {
    DelimitedOutput();
//    ConsoleOutput();
  } else {
  }
  return false;
}


void FGOutput::ConsoleOutput(void)
{
#ifdef HAVE_CURSES
  string buffer;

  clear();
  move(1,1);  insstr("Quaternions");
  move(2,5);  insstr("Q0");
  move(2,16); insstr("Q1");
  move(2,27); insstr("Q2");
  move(2,38); insstr("Q3");

  move(3,1);  buffer = Rotation->GetQ0(); insstr(buffer.c_str());
  move(3,12); buffer = Rotation->GetQ1(); insstr(buffer.c_str());
  move(3,23); buffer = Rotation->GetQ2(); insstr(buffer.c_str());
  move(3,34); buffer = Rotation->GetQ3(); insstr(buffer.c_str());

  move(0,0); insstr("Time: ");
  move(0,6); insstr(gcvt(State->Getsim_time(),6,buffer));

  move(2,46); insstr("Phi");
  move(2,55); insstr("Tht");
  move(2,64); insstr("Psi");

  move(3,45); buffer = Rotation->Getphi(); insstr(buffer.c_str());
  move(3,54); buffer = Rotation->Gettht(); insstr(buffer.c_str());
  move(3,63); buffer = Rotation->Getpsi(); insstr(buffer.c_str());

  move(5,47); insstr("U");
  move(5,56); insstr("V");
  move(5,65); insstr("W");

  move(6,45); buffer = Translation->GetU(); insstr(buffer.c_str());
  move(6,54); buffer = Translation->GetV(); insstr(buffer.c_str());
  move(6,63); buffer = Translation->GetW(); insstr(buffer.c_str());

  move(8,47); insstr("Fx");
  move(8,56); insstr("Fy");
  move(8,65); insstr("Fz");

  move(9,45); buffer = Aircraft->GetFx(); insstr(buffer.c_str());
  move(9,54); buffer = Aircraft->GetFy(); insstr(buffer.c_str());
  move(9,63); buffer = Aircraft->GetFz(); insstr(buffer.c_str());

  move(11,47); insstr("Fn");
  move(11,56); insstr("Fe");
  move(11,65); insstr("Fd");

  move(12,45); buffer = Position->GetFn(); insstr(buffer.c_str());
  move(12,54); buffer = Position->GetFe(); insstr(buffer.c_str());
  move(12,63); buffer = Position->GetFd(); insstr(buffer.c_str());

  move(14,47); insstr("Latitude");
  move(14,57); insstr("Longitude");
  move(14,67); insstr("Altitude");

  move(15,47); buffer = State->Getlatitude(); insstr(buffer.c_str());
  move(15,57); buffer = State->Getlongitude(); insstr(buffer.c_str());
  move(15,67); buffer = State->Geth(); insstr(buffer.c_str());

  refresh();

  move(LINES-1,1);
  refresh();
#endif
}


void FGOutput::DelimitedOutput(void)
{
  if (FirstPass) {
    cout << "Time,";
    cout << "Altitude,";
    cout << "Phi,";
    cout << "Tht,";
    cout << "Psi,";
    cout << "Rho,";
    cout << "Vtotal,";
    cout << "U,";
    cout << "V,";
    cout << "W,";
    cout << "Vn,";
    cout << "Ve,";
    cout << "Vd,";
    cout << "Udot,";
    cout << "Vdot,";
    cout << "Wdot,";
    cout << "Fx,";
    cout << "Fy,";
    cout << "Fz,";
    cout << "Latitude,";
    cout << "Longitude,";
    cout << "QBar,";
    cout << "Alpha";
    cout << "L";
    cout << "M";
    cout << "N";
    cout << endl;
    FirstPass = false;
  } else {
    cout << State->Getsim_time() << ",";
    cout << State->Geth() << ",";
    cout << Rotation->Getphi() << ",";
    cout << Rotation->Gettht() << ",";
    cout << Rotation->Getpsi() << ",";
    cout << Atmosphere->Getrho() << ",";
    cout << State->GetVt() << ",";
    cout << Translation->GetU() << ",";
    cout << Translation->GetV() << ",";
    cout << Translation->GetW() << ",";
    cout << Position->GetVn() << ",";
    cout << Position->GetVe() << ",";
    cout << Position->GetVd() << ",";
    cout << Translation->GetUdot() << ",";
    cout << Translation->GetVdot() << ",";
    cout << Translation->GetWdot() << ",";
    cout << Aircraft->GetFx() << ",";
    cout << Aircraft->GetFy() << ",";
    cout << Aircraft->GetFz() << ",";
    cout << State->Getlatitude() << ",";
    cout << State->Getlongitude() << ",";
    cout << State->Getqbar() << ",";
    cout << Translation->Getalpha() << ",";
		cout << Aircraft->GetL() << ",";
		cout << Aircraft->GetM() << ",";
		cout << Aircraft->GetN() << "";
    cout << endl;
  }
}
