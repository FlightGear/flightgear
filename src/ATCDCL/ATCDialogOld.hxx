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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef ATC_DIALOG_HXX
#define ATC_DIALOG_HXX

#include <simgear/compiler.h>

#include <vector>
#include <map>

#include "ATC.hxx"

using std::vector;
using std::map;

class NewGUI;

// ATCMenuEntry - an encapsulation of an entry in the ATC dialog
struct ATCMenuEntry {

  string  stationid;	// ID of transmitting station
  //int     stationfr;	// ?
  string  transmission;	// Actual speech of transmission
  string  menuentry;	// Shortened version for display in the dialog
  int callback_code;    // This code is supplied by the registering station, and then
                        // returned to the registering station if that option is chosen.
                        // The actual value is only understood by the registering station - 
                        // FGATCDialog only stores it and returns it if appropriate.

  ATCMenuEntry();
  ~ATCMenuEntry();
};

typedef vector < ATCMenuEntry > atcmentry_vec_type;
typedef atcmentry_vec_type::iterator atcmentry_vec_iterator;
  
typedef map < string, atcmentry_vec_type > atcmentry_map_type;
typedef atcmentry_map_type::iterator atcmentry_map_iterator;

void atcUppercase(string &s);

//void ATCDialogInit();
//void ATCDoDialog(atc_type type);

class FGATCDialog {
	
public:

	FGATCDialog();
	~FGATCDialog();
	
	void Init();
	
	void Update(double dt);
	
	void PopupDialog();
	
	void PopupCallback(int);
	
	void add_entry( const string& station, const string& transmission, const string& menutext, atc_type type, int code);
	
	void remove_entry( const string &station, const string &trans, atc_type type );

	void remove_entry( const string &station, int code, atc_type type );
	
	// query the database whether the transmission is already registered; 
	bool trans_reg( const string &station, const string &trans, atc_type type );
	
	// query the database whether the transmission is already registered; 
	bool trans_reg( const string &station, int code, atc_type type );
	
	// Display a frequency search dialog for nearby stations
	void FreqDialog();
	
	// Display the comm ATC frequencies for airport ident
	// where ident is a valid ICAO code.
	void FreqDisplay(string& ident);

private:

	atcmentry_map_type available_dialog[ATC_NUM_TYPES];
  
	int  freq;
	bool reset;
	
	bool _callbackPending;
	double _callbackTimer;
	double _callbackWait;
	FGATC* _callbackPtr;
	int _callbackCode;

	NewGUI *_gui;
};
	
extern FGATCDialog *current_atcdialog;	

#endif	// ATC_DIALOG_HXX

