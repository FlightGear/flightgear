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


#include "message.hxx"

FGMPSInstanceFuncMap	FGMPSMessage::funcmap;

FGMPSMessage::FGMPSMessage() 
{
	msgid = 0x0000;
	elements[0].type = fgmps_null;
	elements[0].data = NULL;	
	
}

string FGMPSMessage::encodemsg()
{
	FGMPSMsgElementEntry* ptr = getelements();

	buf.clr();
	if (msgid > 255) {
		buf.put(fgmps_som16, true);
		buf.write2(&msgid);
	} else {
		buf.put(fgmps_som8, true);
		buf.write1(&msgid);
	}
	while (ptr->type != fgmps_null) {
		buf.put(ptr->type, true);
		//printf ("adding %02x\n", ptr->type);
		switch (ptr->type) {
		case fgmps_uchar:
		case fgmps_char:
			buf.write1(ptr->data);
			break;
		case fgmps_uint:
		case fgmps_int:
			buf.write2(ptr->data);
			break;
		case fgmps_ulong:
		case fgmps_long:
			buf.write4(ptr->data);
			break;
		case fgmps_float:
			buf.writef(*(float*)ptr->data);
			break;
		case fgmps_double:
			buf.writed(*(double*)ptr->data);
			break;
		case fgmps_string:
			buf.writes(*(string*)ptr->data);
			break;
		}
		ptr++;
	}

	buf.put(fgmps_eom, true);

	return buf.str();
}

FGMPSMessage* FGMPSMessage::decodemsg(string msg)
{
	FGMPSMessageBuf			buf;
	unsigned char			ch;
	unsigned int			mid;
	FGMPSInstanceFuncMap::iterator	fmitr;

	buf.set(msg);
	buf.ofs(0);
	ch = buf.get(true);
	if (ch != fgmps_som8 && ch != fgmps_som16) {
		throw FGMPSDataException("Invalid start of message");
	}
	if (ch == fgmps_som8) {
		ch = buf.get();
		mid = ch;
	} else {
		mid = *(unsigned int *)buf.read2();
	}
	
	fmitr = funcmap.find(mid);
	if (fmitr == funcmap.end()) {
		throw FGMPSDataException("MessageID has no registered Message Class");
	}
	FGMPSMessage* msgclass = (fmitr->second)();
	FGMPSMsgElementEntry* elements = msgclass->getelements();
	while ((ch = buf.get()) != fgmps_eom) {
		//printf("dump: ");
		//for (int i=-1; i<16; i++) printf("%02x ", buf.peek(i));
		//printf("\n");

		if (ch != elements->type) {
			delete msgclass;
			throw FGMPSDataException("Decode: Message Structure Error");
		}
		switch (ch) {
		case fgmps_uchar:
		case fgmps_char:
			memcpy(elements->data, buf.read1(), 1);
			break;
		case fgmps_uint:
		case fgmps_int:
			memcpy(elements->data, buf.read2(), 2);
			break;
		case fgmps_ulong:
		case fgmps_long:
			memcpy(elements->data, buf.read4(), 4);
			break;
		case fgmps_float:
			*(float*)elements->data = buf.readf();
			break;
		case fgmps_double:
			*(double*)elements->data = buf.readd();
			break;
		case fgmps_string:
			ch = buf.get();
			*(string*)elements->data = buf.reads(ch);
			break;
		default:
			delete msgclass;
			throw FGMPSDataException("Decode: Unknown data type");
			break;
		}
		elements++;
	}
	return msgclass;
}


