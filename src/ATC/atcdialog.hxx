// atcdialog.hxx - classes to manage user interactions with AI traffic
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


#ifndef _ATC_DIALOG_HXX_
#define _ATC_DIALOG_HXX_


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/constants.h>
#include <simgear/structure/commands.hxx>
#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/SGSharedPtr.hxx>


#include <GUI/gui.h>		// mkDialog
#include <GUI/new_gui.hxx>

typedef vector<string> StringVec;
typedef vector<string>:: iterator StringVecIterator;

static bool doATCDialog(const SGPropertyNode* arg);

class FGATCDialogNew {
private:
     NewGUI *_gui;
     bool dialogVisible;
     StringVec commands;
public:

    FGATCDialogNew();
    ~FGATCDialogNew();

    void init();

    void update(double dt);
    void PopupDialog();
    void addEntry(int, string);
};

extern FGATCDialogNew *currentATCDialog;

#endif // _ATC_DIALOG_HXX_