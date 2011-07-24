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

#include <Main/fg_commands.hxx>
#include <Main/globals.hxx>

#include <simgear/constants.h>
#include <simgear/structure/commands.hxx>
#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "atcdialog.hxx"

FGATCDialogNew *currentATCDialog;

static bool doATCDialog(const SGPropertyNode* arg) {
        cerr << "Running doATCDialog" << endl;
	currentATCDialog->PopupDialog();
	return(true);
}

FGATCDialogNew::FGATCDialogNew()
{
  dialogVisible = true;
}

FGATCDialogNew::~FGATCDialogNew()
{
}



void FGATCDialogNew::init() {
	// Add ATC-dialog to the command list
	globals->get_commands()->addCommand("ATC-dialog", doATCDialog);
	// Add ATC-freq-search to the command list
	//globals->get_commands()->addCommand("ATC-freq-search", do_ATC_freq_search);

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

static SGPropertyNode *getNamedNode(SGPropertyNode *prop, const char *name) {
    SGPropertyNode* p;

    for (int i = 0; i < prop->nChildren(); i++)
        if ((p = getNamedNode(prop->getChild(i), name)))
            return p;

        if (!strcmp(prop->getStringValue("name"), name))
            return prop;
      return 0;
}

void FGATCDialogNew::addEntry(int nr, string txt) {
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
 double onBoardRadioFreq0 =
        fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
    double onBoardRadioFreq1 =
        fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");

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
    for (StringVecIterator i = commands.begin(); i != commands.end(); i++) {
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
	string legend = string(buf) + (*i); //"; // + current->menuentry;
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