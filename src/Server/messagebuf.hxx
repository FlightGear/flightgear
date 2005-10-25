// messagebuf.hxx -- Multiplayer Client/Server message buffer class
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


#ifndef _FG_MPS_MESSAGEBUF_HXX
#define _FG_MPS_MESSAGEBUF_HXX

#include <simgear/compiler.h>

#include STL_STRING
#include <stdexcept>

SG_USING_STD(string);
SG_USING_STD(invalid_argument);

class FGMPSDataException : public invalid_argument
{
public:
    FGMPSDataException( const string& what_string )
	: invalid_argument(what_string) {}
};

class FGMPSMessageBuf 
{

private:
	string		buf;
	size_t		pos;
	unsigned char	tmp[16];

public:

	FGMPSMessageBuf()		{ init(""); }
	FGMPSMessageBuf(const string& data)	{ init(data); }

	~FGMPSMessageBuf() {}

	void init(const string& data)
	{
		buf = data;
		pos = 0;
	}

	void	clr()			{ buf = ""; }
	void	add(const string& data)	{ buf += data; }
	void	set(const string& data)	{ buf = data; }
	const string&	str()		{ return buf; }
	size_t	ofs()			{ return pos; }
	void	ofs(size_t val)		{ pos = val; }

	unsigned char	get(bool raw = false);
	unsigned char	peek(size_t ofs = 0) { return buf[pos+ofs]; }
	void		put(unsigned char byte, bool raw = false);

	void write1(void* data);
	void write2(void* data);
	void write4(void* data);
	void write8(void* data);
	void writef(float data);
	void writed(double data);
	void writes(string data);

	void*	read1();
	void*	read2();
	void*	read4();
	void*	read8();
	float	readf();
	double	readd();
	string	reads(size_t length);

	const string&	buffer() { return buf; }
};

#endif
