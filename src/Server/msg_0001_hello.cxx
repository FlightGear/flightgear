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


#include "msg_0001_hello.hxx"

FGMPSMsg0001Hello::FGMPSMsg0001Hello() 
{
	msgid = 0x0001;
	elements[0].type = fgmps_uint;		elements[0].data = &this->vermajor;	
	elements[1].type = fgmps_uint;		elements[0].data = &this->verminor;	
	elements[2].type = fgmps_uint;		elements[0].data = &this->verpatch;	
	elements[3].type = fgmps_string;	elements[0].data = &this->servname;	
	elements[4].type = fgmps_null;		elements[4].data = NULL;	
}




