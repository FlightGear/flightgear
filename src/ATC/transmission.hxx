// transmission.hxx -- Transmission class
//
// Written by Alexander Kappes, started March 2002.
// Based on nav.hxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2001  David C. Luff - david.luff@nottingham.ac.uk
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef _FG_TRANSMISSION_HXX
#define _FG_TRANSMISSION_HXX

#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <Main/fg_props.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#include <iomanip>
#elif defined( SG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ )
#  include <iostream>
#else
#  include <istream.h>
#include <iomanip.h>
#endif

#include "ATC.hxx"

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif

SG_USING_STD(string);

struct TransCode {
  int c1;
  int c2;
  int c3;
};

// TransPar - a representation of the logic of a parsed speech transmission
struct TransPar {
  string  station;
  string  callsign;
  string  airport;
  string  intention;      // landing, crossing
  string  intid;          // (airport) ID for intention
  bool    request;        // is the transmission a request or an answer?
  int     tdir;           // turning direction: 1=left, 2=right
  double  heading;
  int     VDir;           // vertical direction: 1=descent, 2=maintain, 3=climb
  double  alt;
  double  miles;
  string  runway;
  double  freq;
  double  time;
};

// FGTransmission - a class to encapsulate a speech transmission
class FGTransmission {

  //int       StationType;    // Type of ATC station: 1 Approach
  atc_type  StationType;
  TransCode Code;           // DCL - no idea what this is.
  string    TransText;      // The text of the spoken transmission
  string    MenuText;       // An abbreviated version of the text for the menu entry

public:

  FGTransmission(void);
  ~FGTransmission(void);

  void Init();

  inline atc_type  get_station()   const { return StationType; }
  inline const TransCode& get_code()      { return Code; }
  inline const string&    get_transtext() { return TransText; }
  inline const string&    get_menutext()  { return MenuText; }

  // Return the parsed logic of the transmission  
  TransPar Parse();

private:

  friend istream& operator>> ( istream&, FGTransmission& );

};


inline istream&
operator >> ( istream& in, FGTransmission& a ) {
	char ch;
	int tmp;
	
	static bool first_time = true;
	static double julian_date = 0;
	static const double MJD0    = 2415020.0;
	if ( first_time ) {
		julian_date = sgTimeCurrentMJD(0, 0) + MJD0;
		first_time = false;
	}
	// Ugly hack alert - eventually we'll use xml format for the transmissions file
	in >> tmp;
	if(tmp == 1) {
		a.StationType = APPROACH;
	} else {
		a.StationType = INVALID;
	}
	in >> a.Code.c1;
	in >> a.Code.c2;
	in >> a.Code.c3;
	a.TransText = "";
	in >> ch;
	if ( ch != '"' ) a.TransText += ch;
	while(1) {
		//in >> noskipws
		in.unsetf(ios::skipws);
		in >> ch;
		if ( ch != '"' ) a.TransText += ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
	}
	in.setf(ios::skipws);
	
	a.MenuText = "";
	in >> ch;
	if ( ch != '"' ) a.MenuText += ch;
	while(1) {
		//in >> noskipws
		in.unsetf(ios::skipws);
		in >> ch;
		if ( ch != '"' ) a.MenuText += ch;
		if((ch == '"') || (ch == 0x0A)) {
			break;
		}   // we shouldn't need the 0x0A but it makes a nice safely in case someone leaves off the "
	}
	in.setf(ios::skipws);
	
	//cout << "Code = " << a.Code << "   Transmission text = " << a.TransText 
	//     << "   Menu text = " << a.MenuText << endl;
	
	return in >> skipeol;
}


#endif // _FG_TRANSMISSION_HXX
