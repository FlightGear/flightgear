// atcdialog.cxx - classes to manage user interactions with AI traffic
// Written by Durk Talsma, started April 3, 2011
// Based on earlier code written by Alexander Kappes and David Luff
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "atcdialog.hxx"

#include <boost/foreach.hpp>

#include <simgear/constants.h>
#include <simgear/structure/commands.hxx>
#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <Main/fg_commands.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <GUI/gui.h>		// mkDialog
#include <GUI/new_gui.hxx>
#include <Airports/simple.hxx>
#include <ATC/CommStation.hxx>


static SGPropertyNode *getNamedNode(SGPropertyNode *prop, const char *name)
{
  SGPropertyNode* p;
  
  for (int i = 0; i < prop->nChildren(); i++)
    if ((p = getNamedNode(prop->getChild(i), name)))
      return p;
  
  if (!strcmp(prop->getStringValue("name"), name))
    return prop;
  return 0;
}

static void atcUppercase(string &s) {
	for(unsigned int i=0; i<s.size(); ++i) {
		s[i] = toupper(s[i]);
	}
}

class AirportsWithATC : public FGAirport::AirportFilter
{
public:
  virtual FGPositioned::Type maxType() const {
    return FGPositioned::SEAPORT;
  }
  
  virtual bool passAirport(FGAirport* aApt) const
  {
    return (!aApt->commStations().empty());
  }
};

void FGATCDialogNew::frequencySearch()
{
	const char *dialog_name = "atc-freq-search";
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;
  
	_gui->closeDialog(dialog_name);
  
	SGPropertyNode_ptr button_group = getNamedNode(dlg, "quick-buttons");
	// remove all dynamic airport/ATC buttons
	button_group->removeChildren("button", false);
  
  AirportsWithATC filt;
  FGPositioned::List results = FGPositioned::findWithinRange(globals->get_aircraft_position(), 50.0, &filt);
  FGPositioned::sortByRange(results, globals->get_aircraft_position());
  for (unsigned int r=0; (r<results.size()) && (r < 6); ++r) {
    
    SGPropertyNode *entry = button_group->getNode("button", r, true);
		copyProperties(button_group->getNode("button-template", true), entry);
		entry->removeChildren("enabled", true);
		entry->setStringValue("legend", results[r]->ident());
		entry->setStringValue("binding[0]/icao", results[r]->ident());
  }
  
	// (un)hide message saying no things in range
	SGPropertyNode_ptr range_error = getNamedNode(dlg, "no-atc-in-range");
	range_error->setBoolValue("visible", !results.empty());
  
	_gui->showDialog(dialog_name);
}

void FGATCDialogNew::frequencyDisplay(const std::string& ident)
{
	const char *dialog_name = "atc-freq-display";
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;
  
	_gui->closeDialog(dialog_name);
  
	SGPropertyNode_ptr freq_group = getNamedNode(dlg, "frequency-list");
	// remove all frequency entries
	freq_group->removeChildren("group", false);
  
  std::string uident(ident);
	atcUppercase(uident);
	string label;
  
	const FGAirport *a = fgFindAirportID(uident);
	if (!a) {
		label = "Airport " + ident + " not found in database.";
		mkDialog(label.c_str());
		return;
	}
  
	// set title
	label = uident + " Frequencies";
	dlg->setStringValue("text/label", label.c_str());
  
  const flightgear::CommStationList& comms(a->commStations());
  if (comms.empty()) {
    label = "No frequencies found for airport " + uident;
		mkDialog(label.c_str());
		return;
  }
  
  int n = 0;
  for (unsigned int c=0; c < comms.size(); ++c) {
    flightgear::CommStation* comm = comms[c];
    
    // add frequency line (modified copy of <group-template>)
		SGPropertyNode *entry = freq_group->getNode("group", n, true);
		copyProperties(freq_group->getNode("group-template", true), entry);
		entry->removeChildren("enabled", true);
    
    entry->setStringValue("text[0]/label", comm->ident());
    
		char buf[8];
		snprintf(buf, 8, "%.2f", comm->freqMHz());
		if(buf[5] == '3') buf[5] = '2';
		if(buf[5] == '8') buf[5] = '7';
		buf[7] = '\0';
    
		entry->setStringValue("text[1]/label", buf);
    ++n;
  }
  
	_gui->showDialog(dialog_name);
}

static bool doFrequencySearch( const SGPropertyNode* )
{
  FGATCDialogNew::instance()->frequencySearch();
  return true;
}

static bool doFrequencyDisplay( const SGPropertyNode* args )
{
  std::string icao = args->getStringValue("icao");
  if (icao.empty()) {
    icao = fgGetString("/sim/atc/freq-airport");
  }
  
  FGATCDialogNew::instance()->frequencyDisplay(icao);
  return true;
}

FGATCDialogNew * FGATCDialogNew::_instance = NULL;

FGATCDialogNew::FGATCDialogNew()
{
  dialogVisible = true;
}

FGATCDialogNew::~FGATCDialogNew()
{
}



void FGATCDialogNew::init() {
	// Add ATC-dialog to the command list
    globals->get_commands()->addCommand("ATC-dialog", FGATCDialogNew::popup );
	// Add ATC-freq-search to the command list
	globals->get_commands()->addCommand("ATC-freq-search", doFrequencySearch);
  globals->get_commands()->addCommand("ATC-freq-display", doFrequencyDisplay);
  
	// initialize properties polled in Update()
	//globals->get_props()->setStringValue("/sim/atc/freq-airport", "");
	globals->get_props()->setIntValue("/sim/atc/transmission-num", -1);
}



// Display the ATC popup dialog box with options relevant to the users current situation.
// 
// So, the first thing we need to do is check the frequency that our pilot's nav radio
// is currently tuned to. The dialog should always contain an option to to tune the 
// to a particular frequency,

// Okay, let's see, according to my interpretation of David Luff's original code,
// his ATC subsystem wrote properties to a named node, called "transmission-choice"


void FGATCDialogNew::addEntry(int nr, const std::string& txt) {
    commands.clear();
    commands.push_back(txt);
    commands.push_back(string("Toggle ground network visibility"));
}

void FGATCDialogNew::removeEntry(int nr) {
    commands.clear();
}



void FGATCDialogNew::PopupDialog() {
    dialogVisible = !dialogVisible;
    return;
}

void FGATCDialogNew::update(double dt) {
//  double onBoardRadioFreq0 =
//      fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
//  double onBoardRadioFreq1 =
//      fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");

    const char *dialog_name = "atc-dialog";
    _gui = (NewGUI *)globals->get_subsystem("gui");
    SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
    if (!dlg)
        return;

    _gui->closeDialog(dialog_name);
    SGPropertyNode_ptr button_group = getNamedNode(dlg, "transmission-choice");
    button_group->removeChildren("button", false);

    const int bufsize = 32;
    char buf[bufsize];
    int commandNr = 0;
    // loop over all entries that should fill up the dialog; use 10 items for now...
    BOOST_FOREACH(const std::string& i, commands) {
        snprintf(buf, bufsize, "/sim/atc/opt[%d]", commandNr);
            fgSetBool(buf, false);
        SGPropertyNode *entry = button_group->getNode("button", commandNr, true);
        copyProperties(button_group->getNode("button-template", true), entry);
	entry->removeChildren("enabled", true);
	entry->setStringValue("property", buf);
	entry->setIntValue("keynum", '1' + commandNr);
	if (commandNr == 0)
	    entry->setBoolValue("default", true);

	snprintf(buf, bufsize, "%d", 1 + commandNr);
	string legend = string(buf) + i; //"; // + current->menuentry;
	entry->setStringValue("legend", legend.c_str());
	entry->setIntValue("binding/value", commandNr);
        commandNr++;
	//current++;
    }

    if (dialogVisible) {
        _gui->closeDialog(dialog_name);
    } else {
        _gui->showDialog(dialog_name);
    }
    //dialogVisible = !dialogVisible;
    return;
    /*
    static SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
    int n = trans_num->getIntValue();
    if (n >= 0) {
        trans_num->setIntValue(-1);
           // PopupCallback(n);
        cerr << "Selected transmission message" << n << endl;
    } */
}
