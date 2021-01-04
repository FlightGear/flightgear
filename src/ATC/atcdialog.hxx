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
#include <simgear/compiler.h>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

class NewGUI;
class SGPropertyNode;

/**
 * despite looking like a subsystem, this is not, at the moment.
 * So don't assume it has subsystem semantics or behaviour.
 * For 'next' we're fixing this, but it's too complex a change for 2020.3
 */
class FGATCDialogNew
{
private:
     NewGUI *_gui;
     bool dialogVisible;
     string_list commands;

     static FGATCDialogNew *_instance;
public:

    FGATCDialogNew();
    ~FGATCDialogNew();

    void frequencySearch();
    void frequencyDisplay(const std::string& ident);
  
    void init();
    void shutdown();

    void update(double dt);
    void PopupDialog();
    void addEntry(int, const std::string&);
    void removeEntry(int);

    static bool popup( const SGPropertyNode *, SGPropertyNode * root) {
        instance()->PopupDialog();
        return true;
    }
  
    /**
     * hacky fix for various crashes on reset: give a way to reset instance
     * back to null-ptr, so that we get a new instance created. 
     * Without this, we crash in various exciting ways due to stale pointers
     * 
     */
    static void hackyReset();

    // Generate new instance of FGATCDialogNew if it hasn't yet been generated
	// Call constructor and init() functions
	// If it has been generated, will return that instance
    inline static FGATCDialogNew * instance() {
        if( _instance != NULL ) return _instance;
        _instance = new FGATCDialogNew();
        _instance->init();
        return _instance;
    }
};


#endif // _ATC_DIALOG_HXX_