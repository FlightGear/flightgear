// message.hxx -- Multiplayer Client/Server message base class
//
// Written by John Barrett, started November 2003.
//
// Copyright (C) 2003  John R. Barrett - jbarrett@accesshosting.com
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


#ifndef _FG_MPS_MSG0001_HXX
#define _FG_MPS_MSG0001_HXX

#include "message.hxx"

#define FGMPSMsg0001HelloID 0x0001

class FGMPSMsg0001Hello: public FGMPSMessage
{
private:
	FGMPSMsgElementEntry	 	elements[5];
public:

	static void registerme() 
	{ 
		FGMPSMessage::registermsg(FGMPSMsg0001HelloID, &FGMPSMsg0001Hello::instance);
	}

	static FGMPSMessage* instance() { return (FGMPSMessage*) new FGMPSMsg0001Hello; }

	virtual FGMPSMsgElementEntry*	getelements() { return elements; }

	FGMPSMsg0001Hello();
	~FGMPSMsg0001Hello() {}

	unsigned int	vermajor;
	unsigned int	verminor;
	unsigned int	verpatch;
	string		servname;
};

#endif
