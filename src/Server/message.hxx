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


#ifndef _FG_MPS_MESSAGE_HXX
#define _FG_MPS_MESSAGE_HXX

#include <simgear/compiler.h>

#include STL_STRING
#include <stdexcept>
#include <map>

SG_USING_STD(string);
SG_USING_STD(invalid_argument);
SG_USING_STD(map);

#include "messagebuf.hxx"

typedef enum 
{
	fgmps_null	= 0x00,
	fgmps_uchar	= 0xf0,
	fgmps_uint	= 0xf1,
	fgmps_ulong	= 0xf2,
	fgmps_ulonglong	= 0xf3,
	fgmps_char	= 0xf4,
	fgmps_int	= 0xf5,
	fgmps_long	= 0xf6,
	fgmps_longlong	= 0xf7,
	fgmps_float	= 0xf8,
	fgmps_double	= 0xf9,
	fgmps_string	= 0xfa,
	fgmps_reserved	= 0xfb,
	fgmps_eom	= 0xfc,
	fgmps_som8	= 0xfd,
	fgmps_som16	= 0xfe,
	fgmps_esc	= 0xfe
} FGMPSMsgElementType;

typedef struct 
{
	FGMPSMsgElementType	type;
	void *			data;
} FGMPSMsgElementEntry;

#define FGMPSMsgElementArrayEnd {fgmps_null, 0}

class FGMPSMessage;

typedef FGMPSMessage* FGMPSMessagePtr;
typedef FGMPSMessagePtr (*FGMPSMsgInstanceFunc)(void);

typedef map<unsigned int, FGMPSMsgInstanceFunc> FGMPSInstanceFuncMap;

class FGMPSMessage
{
private:
	static FGMPSInstanceFuncMap	funcmap;
	FGMPSMsgElementEntry 		elements[1];
protected:
	FGMPSMessageBuf		buf;
	unsigned int		msgid;
public:
	static int registermsg(int msgid, FGMPSMsgInstanceFunc func)
	{
		funcmap[msgid] = func;
	}

	FGMPSMessage();
	~FGMPSMessage() {}

	string			encodemsg();
	static FGMPSMessage*	decodemsg(string msg);
	unsigned int		getmessageid() { return msgid; }

	virtual FGMPSMsgElementEntry*	getelements() { return elements; }
};

#endif
