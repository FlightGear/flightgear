// atis.hxx -- ATIS class
//
// Written by David Luff, started October 2001.
// Based on nav.hxx by Curtis Olson, started April 2000.
//
// Copyright (C) 2001  David C. Luff - david.luff@nottingham.ac.uk
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


#ifndef _FG_ATIS_HXX
#define _FG_ATIS_HXX

#include <string>
#include <iosfwd>

#include <simgear/compiler.h>
#include <simgear/timing/sg_time.hxx>

#include "ATC.hxx"

//DCL - a complete guess for now.
#define FG_ATIS_DEFAULT_RANGE 30
	
typedef std::map<std::string,std::string> MSS;

class FGATIS : public FGATC {
	
	//atc_type type;

	// The actual ATIS transmission
	// This is generated from the prevailing conditions when required.
	// This is the version with markup, suitable for voice synthesis:
	std::string transmission;
	
	// Same as above, but in a form more readable as text.
	std::string transmission_readable;

	// for failure modeling
	std::string trans_ident;		// transmitted ident
	double old_volume;
	bool atis_failed;		// atis failed?
	time_t msg_time;		// for moderating error messages
	time_t cur_time;
	int msg_OK;
	int attention;
	
	bool _prev_display;		// Previous value of _display flag
	MSS _remap;                     // abbreviations to be expanded

	std::string _commbase;

	// Aircraft position
	// ATIS is actually a special case in that unlike other ATC eg.tower it doesn't actually know about
	// or the whereabouts of the aircraft it is transmitting to.  However, to ensure consistancy of
	// operation with the other ATC classes the ATIS class must calculate range to the aircraft in order
	// to decide whether to render the transmission - hence the users plane details must be stored.
	//SGPropertyNode_ptr airplane_lon_node; 
	//SGPropertyNode_ptr airplane_lat_node;
	//SGPropertyNode_ptr airplane_elev_node; 
	
public:
	
	FGATIS(const std::string& commbase);
	~FGATIS(void);
	virtual void Init();
	void attend (int);

	//run the ATIS instance
	void Update(double dt);
	
	//inline void set_type(const atc_type tp) {type = tp;}
	inline const std::string& get_trans_ident() { return trans_ident; }
	
protected:
    virtual FGATCVoice* GetVoicePointer();

private:

	// Generate the ATIS transmission text:
	int GenTransmission(const int regen, const int special);
	
	// Put the text into the property tree
	// (and in debug mode, print it on the console):
	void TreeOut(int msgOK);

	friend std::istream& operator>> ( std::istream&, FGATIS& );

};

typedef int (FGATIS::*int_getter)() const;

#endif // _FG_ATIS_HXX
