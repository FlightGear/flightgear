// multiplay.hxx -- protocol object for multiplay in Flightgear
//
// Written by Diarmuid Tyson, started February 2003.
// diarmuid.tyson@airservicesaustralia.com
//
// With additions by Vivian Meazza, January 2006
//
// Copyright (C) 2003  Airservices Australia
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

#ifndef _FG_MULTIPLAY_HXX
#define _FG_MULTIPLAY_HXX

#define FG_MULTIPLAY_HID "$Id$"

#include <simgear/compiler.h>

#include <string>

#include <simgear/props/props.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Model/acmodel.hxx>
#include <MultiPlayer/multiplaymgr.hxx>

#include "protocol.hxx"

using std::string;


/****************************************************************
* @version $Id$
*
* Description: FGMultiplay is an FGProtocol object used as the basic
* interface for the multiplayer code into FlightGears generic IO
* subsystem.  It only implements the basic FGProtocol methods: open(),
* process() and close().  It does not use Sim Gear's IO channels, as
* the MultiplayMgrs creates their own sockets through plib.
*
* It will set up it's direction and rate protocol properties when
* created.  Subsequent calls to process will prompt the
* MultiplayMgr to either send or receive data over the network.
*
******************************************************************/

class FGMultiplay : public FGProtocol {
public:

    /** Constructor */
    FGMultiplay (const string &dir, const int rate, const string &host, const int port);

    /** Destructor. */
    ~FGMultiplay ();

    /** Enables the FGMultiplay object. */
    bool open();

    /** Tells the multiplayer_mgr to send/receive data.
    */
    bool process();

    /** Closes the multiplayer_mgr.
    */
    bool close();

    void setPropertiesChanged()
    {
      mPropertiesChanged = true;
    }
private:
  bool mPropertiesChanged;
  
  void findProperties();
  
  // Map between the property id's from the multiplayers network packets
  // and the property nodes
  typedef std::map<unsigned, SGSharedPtr<SGPropertyNode> > PropertyMap;
  PropertyMap mPropertyMap;
};


#endif // _FG_MULTIPLAY_HXX
