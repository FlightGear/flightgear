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
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "atcdialog.hxx"



static bool doATCDialog(const SGPropertyNode* arg) {
        cerr << "Running doATCDialog" << endl;
	current_atcdialog->PopupDialog();
	return(true);
}

FGATCDialogNew::FGATCDialogNew()
{
  dialogVisible = false;
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
	//globals->get_props()->setIntValue("/sim/atc/transmission-num", -1);
}

// Display the ATC popup dialog box with options relevant to the users current situation.
void FGATCDialogNew::PopupDialog() {
	const char *dialog_name = "atc-dialog";
        _gui = (NewGUI *)globals->get_subsystem("gui");
	SGPropertyNode_ptr dlg = _gui->getDialogProperties(dialog_name);
	if (!dlg)
		return;
        if (dialogVisible) {
	     _gui->closeDialog(dialog_name);
        } else {
	     _gui->showDialog(dialog_name);
        }
        dialogVisible = !dialogVisible;
	return;
}
