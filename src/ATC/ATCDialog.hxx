// ATCDialog.hxx - Functions and classes to handle the pop-up ATC dialog
//
// Written by Alexander Kappes and David Luff, started February 2003.
//
// Copyright (C) 2003  Alexander Kappes and David Luff
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

#ifndef ATC_DIALOG_HXX
#define ATC_DIALOG_HXX

#include <simgear/compiler.h>

#include "ATC.hxx"

// ATCMenuEntry - an encapsulation of an entry in the ATC dialog
struct ATCMenuEntry {

  string  stationid;	// ID of transmitting station
  int     stationfr;	// ?
  string  transmission;	// Actual speech of transmission
  string  menuentry;	// Shortened version for display in the dialog

  ATCMenuEntry();
  ~ATCMenuEntry();
};

// convenience types
typedef vector < ATCMenuEntry > atcmentry_list_type;
typedef atcmentry_list_type::iterator atcmentry_list_iterator;
typedef atcmentry_list_type::const_iterator atcmentry_list_const_iterator;
  
// typedef map < string, atcmentry_list_type, less<string> > atcmentry_map_type;
typedef map < string, atcmentry_list_type > atcmentry_map_type;
typedef atcmentry_map_type::iterator atcmentry_map_iterator;
typedef atcmentry_map_type::const_iterator atcmentry_map_const_iterator;

void ATCDialogInit();

void ATCDoDialog(atc_type type);

class FGATCDialog {
	
public:

	void Init();
	
	void DoDialog();
	
	void add_entry( string station, string transmission, string menutext );
	
	bool trans_reg( const string &station, const string &trans );
	
	// Display a frequency search dialog for nearby stations
	void FreqDialog();
	
	// Display the comm ATC frequencies for airport ident
	// where ident is a valid ICAO code.
	void FreqDisplay(string ident);

private:

	atcmentry_map_type atcmentrylist_station;
  
	int  freq;
	bool reset;
	
};
	
extern FGATCDialog *current_atcdialog;	

#endif	// ATC_DIALOG_HXX

