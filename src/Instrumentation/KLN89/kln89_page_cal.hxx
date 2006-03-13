// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff:  daveluff --AT-- ntlworld --D0T-- com
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

#ifndef _KLN89_PAGE_CAL
#define _KLN89_PAGE_CAL

#include "kln89.hxx"

class KLN89CalPage : public KLN89Page {

public:
	KLN89CalPage(KLN89* parent);
	~KLN89CalPage();
	
	void Update(double dt);

	void CrsrPressed();
	void ClrPressed();
	void Knob2Left1();
	void Knob2Right1();
	
	void LooseFocus();
	
private:
	unsigned int _nFp0;	// flightplan no. displayed on page 1 (_subPage == 0).
	double _ground_speed_ms;	// Assumed ground speed for ete calc on page 1 (user can alter this).

    // CAL3 (alarm) page
    ClockTime _alarmTime;
    ClockTime _alarmIn;
	// _alarmAnnotate shows that the alarm has been set at least once
	// so the time should now be always annotated (there seems to be
	// no way to remove it once set once!).
    bool _alarmAnnotate;
	// _alarmSet indicates that the alarm has been changed by the user
	// and should be set in the main unit when the page looses focus
	// (I don't think the alarm goes off unless the user leaves the
	// CAL3 page after setting it).
	bool _alarmSet;

    // Calculate the alarm time based on the alarm-in value
    void CalcAlarmTime();
    // Calculate alarm-in based on the alarm time.
    void CalcAlarmIn();

    // Calculate the difference between 2 hr:min times.
    // It is assumed that the second time is always later than the first one
    // ie. that the day has wrapped if the second one is less,
    // but is limited to intervals of < 24hr.
    void TimeDiff(int hr1, int min1, int hr2, int min2, int &hrDiff, int &minDiff);
};


inline ostream& operator<< (ostream& out, const ClockTime& t) {
    return(out << t._hr << ':' << t._min);
}

#endif	// _KLN89_PAGE_CAL
