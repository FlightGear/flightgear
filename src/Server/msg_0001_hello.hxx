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

class FGMPSMsg0001Hello
{
private:
	FGMPSMsgElementEntry	 	elements[5];
	unsigned int			msgid;
public:
	FGMPSMsg0001Hello();
	~FGMPSMsg0001Hello() {}

	virtual string			encodemsg() {}
	virtual FGMPSMessage*		decodemsg(string msg) {}
	virtual FGMPSMsgElementEntry*	getelements() { return elements; }
	virtual unsigned int		getmessageid() { return msgid; }

	unsigned int	vermajor;
	unsigned int	verminor;
	unsigned int	verpatch;
	string		servname;
};

#endif
