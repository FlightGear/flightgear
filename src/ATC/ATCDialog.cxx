// ATCDialog.cxx - Functions and classes to handle the pop-up ATC dialog
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

#include <simgear/compiler.h>

#include <simgear/structure/commands.hxx>

#include <Main/globals.hxx>
#include <GUI/gui.h>

#include "ATCDialog.hxx"
#include "ATC.hxx"
#include "ATCmgr.hxx"
#include "ATCdisplay.hxx"
#include "commlist.hxx"
#include "ATCutils.hxx"
#include <Airports/simple.hxx>

#include <sstream>

SG_USING_STD(ostringstream);

FGATCDialog *current_atcdialog;

// For the command manager - maybe eventually this should go in the built in command list
static bool do_ATC_dialog(const SGPropertyNode* arg) {
	current_atcdialog->PopupDialog();
	return(true);
}

static bool do_ATC_freq_search(const SGPropertyNode* arg) {
	current_atcdialog->FreqDialog();
	return(true);
}

ATCMenuEntry::ATCMenuEntry() {
  stationid    = "";
  //stationfr    = 0;
  transmission = "";
  menuentry    = "";
  callback_code = 0;
}

ATCMenuEntry::~ATCMenuEntry() {
}

static void atcUppercase(string &s) {
	for(unsigned int i=0; i<s.size(); ++i) {
		s[i] = toupper(s[i]);
	}
}

// ----------------------- Popup Dialog Statics------------------
static puDialogBox*     atcDialog;
static puFrame*         atcDialogFrame;
static puText*          atcDialogMessage;
static puOneShot*       atcDialogOkButton;
static puOneShot*       atcDialogCancelButton;
static puButtonBox*     atcDialogCommunicationOptions;
// --------------------------------------------------------------

// ----------------------- Freq Dialog Statics-------------------
static const int ATC_MAX_FREQ_DISPLAY = 20;		// Maximum number of frequencies that can be displayed for any one airport

static puDialogBox*     atcFreqDialog;
static puFrame*         atcFreqDialogFrame;
static puText*          atcFreqDialogMessage;
static puInput*         atcFreqDialogInput;
static puOneShot*       atcFreqDialogOkButton;
static puOneShot*       atcFreqDialogCancelButton;

static puDialogBox*     atcFreqDisplay;
static puFrame*         atcFreqDisplayFrame;
static puText*          atcFreqDisplayMessage;
static puOneShot*       atcFreqDisplayOkButton;
static puText*          atcFreqDisplayText[ATC_MAX_FREQ_DISPLAY];
// --------------------------------------------------------------

//////////////// Popup callbacks ///////////////////
static void ATCDialogOK(puObject*) {
	current_atcdialog->PopupCallback();
	FG_POP_PUI_DIALOG( atcDialog );
}

static void ATCDialogCancel(puObject*) {
    FG_POP_PUI_DIALOG( atcDialog );
}
//////////////////////////////////////////////////


///////////////// Freq search callbacks ///////////
static void FreqDialogCancel(puObject*) {
	FG_POP_PUI_DIALOG(atcFreqDialog);
}

static void FreqDialogOK(puObject*) {
	string tmp = atcFreqDialogInput->getStringValue();
	FG_POP_PUI_DIALOG(atcFreqDialog);
	current_atcdialog->FreqDisplay(tmp);
}

static void FreqDisplayOK(puObject*) {
	FG_POP_PUI_DIALOG(atcFreqDisplay);
}
//////////////////////////////////////////////////


FGATCDialog::FGATCDialog() {
	_callbackPending = false;
	_callbackTimer = 0.0;
	_callbackWait = 0.0;
	_callbackPtr = NULL;
	_callbackCode = 0;
}

FGATCDialog::~FGATCDialog() {
	if(atcDialog) puDeleteObject(atcDialog);
	if(atcFreqDialog) puDeleteObject(atcFreqDialog);
	if(atcFreqDisplay) puDeleteObject(atcFreqDisplay);
}

void FGATCDialog::Init() {
	// Add ATC-dialog to the command list
	globals->get_commands()->addCommand("ATC-dialog", do_ATC_dialog);
	// Add ATC-freq-search to the command list
	globals->get_commands()->addCommand("ATC-freq-search", do_ATC_freq_search);
	
	int w;
	int h;
	int x;
	int y;
	
	// Init the freq-search dialog
	w = 300;
	h = 150;
	x = (fgGetInt("/sim/startup/xsize") / 2) - (w / 2);
	y = 50;
	char *s;
	atcFreqDialog = new puDialogBox (x, y);
	{
		atcFreqDialogFrame = new puFrame (0, 0, w, h);
		atcFreqDialogMessage = new puText          (40, (h - 30));
		atcFreqDialogMessage->setDefaultValue ("Enter airport identifier:");
		atcFreqDialogMessage->getDefaultValue (&s);
		atcFreqDialogMessage->setLabel(s);
	
		atcFreqDialogInput = new puInput (50, (h - 75), 150, (h - 45));
			
		atcFreqDialogOkButton     =  new puOneShot         (50, 10, 110, 50);
		atcFreqDialogOkButton     ->     setLegend         (gui_msg_OK);
		atcFreqDialogOkButton     ->     makeReturnDefault (TRUE);
		atcFreqDialogOkButton     ->     setCallback       (FreqDialogOK);
			
		atcFreqDialogCancelButton =  new puOneShot         (140, 10, 210, 50);
		atcFreqDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
		atcFreqDialogCancelButton ->     setCallback       (FreqDialogCancel);
		
		atcFreqDialogInput->acceptInput();
	}
	
	FG_FINALIZE_PUI_DIALOG(atcFreqDialog);
	
	// Init the freq-display dialog
	w = 400;
	h = 100;
	x = (fgGetInt("/sim/startup/xsize") / 2) - (w / 2);
	y = 50;
	atcFreqDisplay = new puDialogBox (x, y);
	{
		atcFreqDisplayFrame   = new puFrame (0, 0, w, h);
		
		atcFreqDisplayMessage = new puText          (40, (h - 30));
		atcFreqDisplayMessage    -> setDefaultValue ("No freqencies found");
		atcFreqDisplayMessage    -> getDefaultValue (&s);
		atcFreqDisplayMessage    -> setLabel        (s);
		
		for(int i=0; i<ATC_MAX_FREQ_DISPLAY; ++i) {
			atcFreqDisplayText[i] = new puText(40, h - 65 - (30 * i));
			atcFreqDisplayText[i]->setDefaultValue("");
			atcFreqDisplayText[i]-> getDefaultValue (&s);
			atcFreqDisplayText[i]-> setLabel        (s);
			atcFreqDisplayText[i]->hide();
		}
		
		atcFreqDisplayOkButton     =  new puOneShot         (50, 10, 110, 50);
		atcFreqDisplayOkButton     ->     setLegend         (gui_msg_OK);
		atcFreqDisplayOkButton     ->     makeReturnDefault (TRUE);
		atcFreqDisplayOkButton     ->     setCallback       (FreqDisplayOK);
	}
	FG_FINALIZE_PUI_DIALOG(atcFreqDisplay);
	
	// Init AK's interactive ATC menus
	w = 500;
	h = 110;
	x = (fgGetInt("/sim/startup/xsize") / 2) - (w / 2);
	//y = (fgGetInt("/sim/startup/ysize") / 2) - (h / 2);
	y = 50;
	atcDialog = new puDialogBox (x, y);
	{
		atcDialogFrame = new puFrame (0,0,w,h);
		atcDialogMessage = new puText (w / 2, h - 30);
		atcDialogMessage -> setLabel( "No transmission available" );
		atcDialogMessage -> setLabelPlace(PUPLACE_TOP_CENTERED);
		atcDialogCommunicationOptions = new puButtonBox (50, 60, 450, 50, NULL, true);
		atcDialogCommunicationOptions -> hide();
		atcDialogOkButton     =  new puOneShot         ((w/2)-85, 10, (w/2)-25, 50);
		atcDialogOkButton     ->     setLegend         (gui_msg_OK);
		atcDialogOkButton     ->     makeReturnDefault (TRUE);
		atcDialogOkButton     ->     setCallback       (ATCDialogOK);
		
		atcDialogCancelButton =  new puOneShot         ((w/2)+25, 10, (w/2)+85, 50);
		atcDialogCancelButton ->     setLegend         (gui_msg_CANCEL);
		atcDialogCancelButton ->     setCallback       (ATCDialogCancel);
	}
	FG_FINALIZE_PUI_DIALOG(atcDialog);
}

void FGATCDialog::Update(double dt) {
	if(_callbackPending) {
		if(_callbackTimer > _callbackWait) {
			_callbackPtr->ReceiveUserCallback(_callbackCode);
			_callbackPtr->NotifyTransmissionFinished(fgGetString("/sim/user/callsign"));
			_callbackPending = false;
		} else {
			_callbackTimer += dt;
		}
	}
}

// Add an entry
void FGATCDialog::add_entry(string station, string transmission, string menutext, atc_type type, int code) {

  ATCMenuEntry a;

  a.stationid = station;
  a.transmission = transmission;
  a.menuentry = menutext;
  a.callback_code = code;

  (available_dialog[type])[station.c_str()].push_back(a);

}

void FGATCDialog::remove_entry( const string &station, const string &trans, atc_type type ) {
  atcmentry_vec_type* p = &((available_dialog[type])[station]);
  atcmentry_vec_iterator current = p->begin();  
  while(current != p->end()) {
    if(current->transmission == trans) current = p->erase(current);
	else ++current;
  }
}

void FGATCDialog::remove_entry( const string &station, int code, atc_type type ) {
  atcmentry_vec_type* p = &((available_dialog[type])[station]);
  atcmentry_vec_iterator current = p->begin();
  while(current != p->end()) {
    if(current->callback_code == code) current = p->erase(current);
	else ++current;
  }
}

// query the database whether the transmission is already registered; 
bool FGATCDialog::trans_reg( const string &station, const string &trans, atc_type type ) {
  atcmentry_vec_type* p = &((available_dialog[type])[station]);
  atcmentry_vec_iterator current = p->begin();
  for ( ; current != p->end() ; ++current ) {
    if ( current->transmission == trans ) return true;
  }
  return false;
}

// query the database whether the transmission is already registered; 
bool FGATCDialog::trans_reg( const string &station, int code, atc_type type ) {
  atcmentry_vec_type* p = &((available_dialog[type])[station]);
  atcmentry_vec_iterator current = p->begin();
  for ( ; current != p->end() ; ++current ) {
    if ( current->callback_code == code ) return true;
  }
  return false;
}

// Display the ATC popup dialog box with options relevant to the users current situation.
void FGATCDialog::PopupDialog() {
	
	static string mentry[10];
	static string mtrans[10];
	char   buf[10];
	TransPar TPar;
	FGATC* atcptr = globals->get_ATC_mgr()->GetComm1ATCPointer();	// Hardwired to comm1 at the moment
	
	int w = 500;
	int h = 100;
	if(atcptr) {
		if(atcptr->GetType() == ATIS) {
			atcDialogCommunicationOptions->hide();
			atcDialogMessage -> setLabel( "Tuned to ATIS - no communication possible" );
			atcDialogFrame->setSize(w, h);
			atcDialogMessage -> setPosition(w / 2, h - 30);
		} else {
			
			atcmentry_vec_type atcmlist = (available_dialog[atcptr->GetType()])[atcptr->get_ident()];
			atcmentry_vec_iterator current = atcmlist.begin();
			atcmentry_vec_iterator last = atcmlist.end();
			
			// Set all opt flags to false before displaying box
			fgSetBool("/sim/atc/opt0",false);
			fgSetBool("/sim/atc/opt1",false);
			fgSetBool("/sim/atc/opt2",false);
			fgSetBool("/sim/atc/opt3",false);
			fgSetBool("/sim/atc/opt4",false);
			fgSetBool("/sim/atc/opt5",false);
			fgSetBool("/sim/atc/opt6",false);
			fgSetBool("/sim/atc/opt7",false);
			fgSetBool("/sim/atc/opt8",false);
			fgSetBool("/sim/atc/opt9",false);
			
			int k = atcmlist.size();
			h += k * 25;
			//cout << "k = " << k << '\n';
			
			atcDialogFrame->setSize(w, h); 
			
			if(k) { 
				// loop over all entries in atcmentrylist
				char** optList = new char*[k+1];
				int kk = 0;
				for ( ; current != last ; ++current ) {
					string dum;
					sprintf( buf, "%i", kk+1 );
					buf[1] = '\0';
					dum = buf;
					mentry[kk] = dum + ". " + current->menuentry;
					optList[kk] = new char[strlen(mentry[kk].c_str()) + 1];
					strcpy(optList[kk], mentry[kk].c_str());
					//cout << "optList[" << kk << "] = " << optList[kk] << endl; 
					mtrans[kk] =              current->transmission;
					++kk;
				} 
				optList[k] = NULL;
				atcDialogCommunicationOptions->newList(optList);
				atcDialogCommunicationOptions->setSize(w-100, h-90);
				atcDialogCommunicationOptions->reveal();
				atcDialogMessage -> setLabel( "ATC Menu" );
				atcDialogMessage -> setPosition(w / 2, h - 30);
			} else {
				atcDialogCommunicationOptions->hide();
				atcDialogMessage -> setLabel( "No transmission available" );
				atcDialogMessage -> setPosition(w / 2, h - 30);
			}
		}
	} else {
		atcDialogCommunicationOptions->hide();
		atcDialogMessage -> setLabel( "Not currently tuned to any ATC service" );
		atcDialogFrame->setSize(w, h);
		atcDialogMessage -> setPosition(w / 2, h - 30);
	}
		
	FG_PUSH_PUI_DIALOG(atcDialog);
}

void FGATCDialog::PopupCallback() {
	FGATC* atcptr = globals->get_ATC_mgr()->GetComm1ATCPointer();	// FIXME - Hardwired to comm1 at the moment

	if(atcptr) {
		if(atcptr->GetType() == APPROACH) {
			switch(atcDialogCommunicationOptions->getValue()) {
			case 0:
				fgSetBool("/sim/atc/opt0",true);
				break;
			case 1:
				fgSetBool("/sim/atc/opt1",true);
				break;
			case 2:
				fgSetBool("/sim/atc/opt2",true);
				break;
			case 3:
				fgSetBool("/sim/atc/opt3",true);
				break;
			default:
				break;
			}
		} else if(atcptr->GetType() == TOWER) {
			//cout << "TOWER " << endl;
			//cout << "ident is " << atcptr->get_ident() << endl;
			atcmentry_vec_type atcmlist = (available_dialog[TOWER])[atcptr->get_ident()];
			if(atcmlist.size()) {
				//cout << "Doing callback...\n";
				ATCMenuEntry a = atcmlist[atcDialogCommunicationOptions->getValue()];
				atcptr->SetFreqInUse();
				// This is the user's speech getting displayed.
				globals->get_ATC_display()->RegisterSingleMessage(atcptr->GenText(a.transmission, a.callback_code));
				_callbackPending = true;
				_callbackTimer = 0.0;
				_callbackWait = 5.0;
				_callbackPtr = atcptr;
				_callbackCode = a.callback_code;
			} else {
				//cout << "No options available...\n";
			}
			//cout << "Donded" << endl;
		}
	}
}

void FGATCDialog::FreqDialog() {

	// Find the ATC stations within a reasonable range (about 40 miles?)
	//comm_list_type atc_stations;
	//comm_list_iterator atc_stat_itr;
	
	//double lon = fgGetDouble("/position/longitude-deg");
	//double lat = fgGetDouble("/position/latitude-deg");
	//double elev = fgGetDouble("/position/altitude-ft");
	
	/*
	// search stations in range
	int num_stat = current_commlist->FindByPos(lon, lat, elev, 40.0, &atc_stations);
	if (num_stat != 0) {
	} else {
		// Make up a message saying no things in range
	}
	*/
	
	// TODO - it would be nice to display a drop-down list of airports within the general vicinity of the user
	//        in addition to the general input box (started above).
	
	atcFreqDialogInput->setValue("");
	atcFreqDialogInput->acceptInput();
	FG_PUSH_PUI_DIALOG(atcFreqDialog);
}

void FGATCDialog::FreqDisplay(string ident) {

	atcUppercase(ident);
	
	string label;
	char *s;
	
	int n = 0;	// Number of ATC frequencies at this airport
	string freqs[ATC_MAX_FREQ_DISPLAY];
	char buf[8];

    FGAirport a;
    if ( dclFindAirportID( ident, &a ) ) {
		comm_list_type stations;
		int found = current_commlist->FindByPos(a._longitude, a._latitude, a._elevation, 20.0, &stations);
		if(found) {
			ostringstream ostr;
			comm_list_iterator itr = stations.begin();
			while(itr != stations.end()) {
				if((*itr).ident == ident) {
					if((*itr).type != INVALID) {
						ostr << (*itr).type;
						freqs[n] = ostr.str();
                                                freqs[n].append("     -     ");
						sprintf(buf, "%.2f", ((*itr).freq / 100.0));	// Convert from KHz to MHz
						// Hack alert!
						if(buf[5] == '3') buf[5] = '2';
						if(buf[5] == '8') buf[5] = '7';
						freqs[n] += buf;
						ostr.seekp(0);
						n++;
					}
				}
				++itr;
			}
		}
		if(n == 0) {
			label = "No frequencies found for airport ";
			label += ident;
		} else {
			label = "Frequencies for airport ";
			label += ident;
			label += ":";
		}
    } else {
		label = "Airport ";
		label += ident;
		label += " not found in database.";
	}

	int hsize = 105 + (n * 30);
	
	atcFreqDisplayFrame->setSize(400, hsize);
	
	atcFreqDisplayMessage -> setPosition (40, (hsize - 30));
	atcFreqDisplayMessage -> setValue    (label.c_str());
	atcFreqDisplayMessage -> getValue    (&s);
	atcFreqDisplayMessage -> setLabel    (s);
	
	for(int i=0; i<n; ++i) {
		atcFreqDisplayText[i] -> setPosition(40, hsize - 65 - (30 * i));
		atcFreqDisplayText[i] -> setValue(freqs[i].c_str());
		atcFreqDisplayText[i] -> getValue (&s);
		atcFreqDisplayText[i] -> setLabel (s);
		atcFreqDisplayText[i] -> reveal();
	}
	for(int j=n; j<ATC_MAX_FREQ_DISPLAY; ++j) {
		atcFreqDisplayText[j] -> hide();
	}
	
	FG_PUSH_PUI_DIALOG(atcFreqDisplay);
	
}

