
//// opengc.cxx - Network interface program to send sim data onto a LAN
//
// Created by: 	J. Wojnaroski  -- castle@mminternet.com
// Date:		21 Nov 2001 
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
//


#ifndef _FG_OPENGC_HXX
#define _FG_OPENGC_HXX

#include <simgear/compiler.h>

#include STL_STRING

#include <FDM/flight.hxx>

#include "protocol.hxx"
#include "opengc_data.hxx"

class FGOpenGC : public FGProtocol, public FGInterface, public FGEngInterface  {

    ogcFGData buf;
    int length;

public:

    FGOpenGC();
    ~FGOpenGC();

    // open hailing frequencies
    bool open();

    // process work for this port
    bool process();

    // close the channel
    bool close();

   
};

#endif // _FG_OPENGC_HXX



