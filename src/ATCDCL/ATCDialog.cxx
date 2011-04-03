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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>

#include <simgear/structure/commands.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/globals.hxx>
#include <GUI/gui.h>		// mkDialog
#include <GUI/new_gui.hxx>

#include "ATCDialog.hxx"
#include "ATC.hxx"
#include "ATCmgr.hxx"
#include "commlist.hxx"
#include "ATCutils.hxx"
#include <Airports/simple.hxx>

#include <sstream>

using std::ostringstream;

FGATCDialog *current_atcdialog;

// For the command manager - maybe eventually this should go in the built in command list
static bool do_ATC_dialog(const SGPropertyNode* arg) {
        cerr << "Running ATCDCL do_ATC_dialog" << endl;
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

void atcUppercase(string &s) {
	for(unsigned int i=0; i<s.size(); ++i) {
		s[i] = toupper(s[i]);
	}
}

// find child whose <name>...</name> entry matches 'name'
static SGPropertyNode *getNamedNode(SGPropertyNode *prop, const char *name) {
	SGPropertyNode* p;

	for (int i = 0; i < prop->nChildren(); i++)
		if ((p = getNamedNode(prop->getChild(i), name)))
			return p;

	if (!strcmp(prop->getStringValue("name"), name))
		return prop;

	return 0;
}


FGATCDialog::FGATCDialog() {
	_callbackPending = false;
	_callbackTimer = 0.0;
	_callbackWait = 0.0;
	_callbackPtr = NULL;
	_callbackCode = 0;
	_gui = (NewGUI *)globals->get_subsystem("gui");
}

FGATCDialog::~FGATCDialog() {
}

void FGATCDialog::Init() {
	// Add ATC-dialog to the command list
	//globals->get_commands()->addCommand("ATC-dialog", do_ATC_dialog);
	// Add ATC-freq-search to the command list
	//globals->get_commands()->addCommand("ATC-freq-search", do_ATC_freq_search);

	// initialize properties polled in Update()
	//globals->get_props()->setStringValue("/sim/atc/freq-airport", "");
	//globals->get_props()->setIntValue("/sim/atc/transmission-num", -1);
}

void FGATCDialog::Update(double dt) {
	//static SGPropertyNode_ptr airport = globals->get_props()->getNode("/sim/atc/freq-airport", true);
	//string s = airport->getStringValue();
	//if (!s.empty()) {
	//	airport->setStringValue("");
	//	FreqDisplay(s);
	//}

	//static SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
	//int n = trans_num->getIntValue();
	//if (n >= 0) {
	//	trans_num->setIntValue(-1);
	//	PopupCallback(n);
	//}

	//if(_callbackPending) {
	//	if(_callbackTimer > _callbackWait) {
	//		_callbackPtr->ReceiveUserCallback(_callbackCode);
	//		_callbackPtr->NotifyTransmissionFinished(fgGetString("/sim/user/callsign"));
	//		_callbackPending = false;
	//	} else {
	//		_callbackTimer += dt;
	//	}
	//}
}

// Add an entry
void FGATCDialog::add_entry(const string& station, const string& transmission, const string& menutext, atc_type type, int code) {

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
	const char *dialog_name = "atc-dialog";
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;

	_gui->closeDialog(dialog_name);

	SGPropertyNode_ptr button_group = getNamedNode(dlg, "transmission-choice");
	// remove all transmission buttons
	button_group->removeChildren("button", false);

	string label;
	FGATC* atcptr = globals->get_ATC_mgr()->GetComm1ATCPointer();	// Hardwired to comm1 at the moment

	if (!atcptr) {
		label = "Not currently tuned to any ATC service";
		mkDialog(label.c_str());
		return;
	}

	if(atcptr->GetType() == ATIS) {
		label = "Tuned to ATIS - no communication possible";
		mkDialog(label.c_str());
		return;
	}

	atcmentry_vec_type atcmlist = (available_dialog[atcptr->GetType()])[atcptr->get_ident()];
	atcmentry_vec_iterator current = atcmlist.begin();
	atcmentry_vec_iterator last = atcmlist.end();
	
	if(!atcmlist.size()) {
		label = "No transmission available";
		mkDialog(label.c_str());
		return;
	}

	const int bufsize = 32;
	char buf[bufsize];
	// loop over all entries in atcmentrylist
	for (int n = 0; n < 10; ++n) {
		snprintf(buf, bufsize, "/sim/atc/opt[%d]", n);
		fgSetBool(buf, false);

		if (current == last)
			continue;

		// add transmission button (modified copy of <button-template>)
		SGPropertyNode *entry = button_group->getNode("button", n, true);
		copyProperties(button_group->getNode("button-template", true), entry);
		entry->removeChildren("enabled", true);
		entry->setStringValue("property", buf);
		entry->setIntValue("keynum", '1' + n);
		if (n == 0)
			entry->setBoolValue("default", true);

		snprintf(buf, bufsize, "%d", n + 1);
		string legend = string(buf) + ". " + current->menuentry;
		entry->setStringValue("legend", legend.c_str());
		entry->setIntValue("binding/value", n);
		current++;
	}

	_gui->showDialog(dialog_name);
	return;
}

void FGATCDialog::PopupCallback(int num) {
	FGATC* atcptr = globals->get_ATC_mgr()->GetComm1ATCPointer();	// FIXME - Hardwired to comm1 at the moment

	if (!atcptr)
		return;

	if (atcptr->GetType() == TOWER) {
		//cout << "TOWER " << endl;
		//cout << "ident is " << atcptr->get_ident() << endl;
		atcmentry_vec_type atcmlist = (available_dialog[TOWER])[atcptr->get_ident()];
		int size = atcmlist.size();
		if(size && num < size) {
			//cout << "Doing callback...\n";
			ATCMenuEntry a = atcmlist[num];
			atcptr->SetFreqInUse();
			string pilot = atcptr->GenText(a.transmission, a.callback_code);
			fgSetString("/sim/messages/pilot", pilot.c_str());
			// This is the user's speech getting displayed.
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

// map() key data type (removes duplicates and sorts by distance)
struct atcdata {
	atcdata() {}
	atcdata(const string i, const string n, const double d) {
		id = i, name = n, distance = d;
	}
	bool operator<(const atcdata& a) const {
		return id != a.id && distance < a.distance;
	}
	bool operator==(const atcdata& a) const {
		return id == a.id && distance == a.distance;
	}
	string id;
	string name;
	double distance;
};

void FGATCDialog::FreqDialog() {
	const char *dialog_name = "atc-freq-search";
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;

	_gui->closeDialog(dialog_name);

	SGPropertyNode_ptr button_group = getNamedNode(dlg, "quick-buttons");
	// remove all dynamic airport/ATC buttons
	button_group->removeChildren("button", false);

	// Find the ATC stations within a reasonable range
	comm_list_type atc_stations;
	comm_list_iterator atc_stat_itr;

  SGGeod geod(SGGeod::fromDegFt(fgGetDouble("/position/longitude-deg"),
    fgGetDouble("/position/latitude-deg"), fgGetDouble("/position/altitude-ft")));
	SGVec3d aircraft = SGVec3d::fromGeod(geod);

	// search stations in range
	int num_stat = current_commlist->FindByPos(geod, 50.0, &atc_stations);
	if (num_stat != 0) {
		map<atcdata, bool> uniq;
		// fill map (sorts by distance and removes duplicates)
		comm_list_iterator itr = atc_stations.begin();
		for (; itr != atc_stations.end(); ++itr) {
			double distance = distSqr(aircraft, itr->cart);
			uniq[atcdata(itr->ident, itr->name, distance)] = true;
		}
		// create button per map entry (modified copy of <button-template>)
		map<atcdata, bool>::iterator uit = uniq.begin();
		for (int n = 0; uit != uniq.end() && n < 6; ++uit, ++n) { // max 6 buttons
			SGPropertyNode *entry = button_group->getNode("button", n, true);
			copyProperties(button_group->getNode("button-template", true), entry);
			entry->removeChildren("enabled", true);
			entry->setStringValue("legend", uit->first.id.c_str());
			entry->setStringValue("binding[0]/value", uit->first.id.c_str());
		}
	}

	// (un)hide message saying no things in range
	SGPropertyNode_ptr range_error = getNamedNode(dlg, "no-atc-in-range");
	range_error->setBoolValue("enabled", !num_stat);

	_gui->showDialog(dialog_name);
}

void FGATCDialog::FreqDisplay(string& ident) {
	const char *dialog_name = "atc-freq-display";
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;

	_gui->closeDialog(dialog_name);

	SGPropertyNode_ptr freq_group = getNamedNode(dlg, "frequency-list");
	// remove all frequency entries
	freq_group->removeChildren("group", false);

	atcUppercase(ident);
	string label;

	const FGAirport *a = fgFindAirportID(ident);
	if (!a) {
		label = "Airport " + ident + " not found in database.";
		mkDialog(label.c_str());
		return;
	}

	// set title
	label = ident + " Frequencies";
	dlg->setStringValue("text/label", label.c_str());

	int n = 0;	// Number of ATC frequencies at this airport

	comm_list_type stations;
	int found = current_commlist->FindByPos(a->geod(), 20.0, &stations);
	if(found) {
		comm_list_iterator itr = stations.begin();
		for (n = 0; itr != stations.end(); ++itr) {
			if(itr->ident != ident)
				continue;

			if(itr->type == INVALID)
				continue;

			// add frequency line (modified copy of <group-template>)
			SGPropertyNode *entry = freq_group->getNode("group", n, true);
			copyProperties(freq_group->getNode("group-template", true), entry);
			entry->removeChildren("enabled", true);

			ostringstream ostr;
			ostr << itr->type;
			entry->setStringValue("text[0]/label", ostr.str());

			char buf[8];
			snprintf(buf, 8, "%.2f", (itr->freq / 100.0));	// Convert from KHz to MHz
			if(buf[5] == '3') buf[5] = '2';
			if(buf[5] == '8') buf[5] = '7';
			buf[7] = '\0';

			entry->setStringValue("text[1]/label", buf);
			n++;
		}
	}
	if(n == 0) {
		label = "No frequencies found for airport " + ident;
		mkDialog(label.c_str());
		return;
	}

	_gui->showDialog(dialog_name);
}

