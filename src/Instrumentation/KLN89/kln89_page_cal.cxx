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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>

#include <Main/fg_props.hxx>
#include "kln89_page_cal.hxx"

using std::string;

KLN89CalPage::KLN89CalPage(KLN89* parent)
: KLN89Page(parent) {
	_nSubPages = 8;
	_subPage = 0;
	_name = "CAL";
	_nFp0 = 0;
	_ground_speed_ms = 110 * 0.514444444444;
	_alarmAnnotate = false;
    _alarmSet = false;
}

KLN89CalPage::~KLN89CalPage() {
}

void KLN89CalPage::Update(double dt) {
	bool crsr = (_kln89->_mode == KLN89_MODE_CRSR);
	bool blink = _kln89->_blink;
	if(_subPage == 0) {
		if(1) { // TODO - fix this hardwiring!
			// Flightplan calc
			_kln89->DrawText(">Fpl:", 2, 0, 3);
			_kln89->DrawText("0", 2, 6, 3);// TODO - fix this hardwiring!
			GPSFlightPlan* fp = _kln89->_flightPlans[_nFp0];
			if(fp) {
				unsigned int n = fp->waypoints.size();
				if(n < 2) {
					// TODO - check that this is what really happens
					_kln89->DrawText("----", 2, 9, 3);
					_kln89->DrawText("----", 2, 9, 2);
				} else {
					_kln89->DrawText(fp->waypoints[0]->id, 2, 9, 3);
					_kln89->DrawText(fp->waypoints[n-1]->id, 2, 9, 2);
					double cum_tot_m = 0.0;
					for(unsigned int i = 1; i < fp->waypoints.size(); ++i) {
						cum_tot_m += _kln89->GetGreatCircleDistance(fp->waypoints[i-1]->lat, fp->waypoints[i-1]->lon,
						                                    fp->waypoints[i]->lat, fp->waypoints[i]->lon);
					}
					double ete = (cum_tot_m * SG_NM_TO_METER) / _ground_speed_ms;
					_kln89->DrawDist(cum_tot_m, 2, 5, 1);
					_kln89->DrawSpeed(_ground_speed_ms / 0.5144444444, 2, 5, 0);
					_kln89->DrawTime(ete, 2, 14, 0);
				}
			} else {
				_kln89->DrawText("----", 2, 9, 3);
				_kln89->DrawText("----", 2, 9, 2);
			}
		} else {
			_kln89->DrawText(">Wpt:", 2, 0, 3);
		}
		_kln89->DrawText("To", 2, 6, 2);
		_kln89->DrawText("ESA ----'", 2, 7, 1);	// TODO - implement an ESA calc
		_kln89->DrawText("ETE", 2, 7, 0);
	} else if(_subPage == 1) {
		_kln89->DrawText(">Fpl: 0", 2, 0, 3);	// TODO - fix this hardwiring!
		_kln89->DrawText("FF:", 2, 0, 2);
		_kln89->DrawText("Res:", 2, 7, 1);
		_kln89->DrawText("Fuel Req", 2, 0, 0);
	} else if(_subPage == 2) {
        _kln89->DrawText("Time:", 2, 0, 3);
		// TODO - hardwired to UTC at the moment
		if(!(_uLinePos == 1 && crsr && blink)) { _kln89->DrawText("UTC", 2, 6, 3); }
		if(_uLinePos == 1 && crsr) { _kln89->Underline(2, 6, 3, 3); }
		string th = fgGetString("/instrumentation/clock/indicated-hour");
		string tm = fgGetString("/instrumentation/clock/indicated-min");
		ClockTime t(atoi(th.c_str()), atoi(tm.c_str()));
		if(th.size() == 1) th = "0" + th;
		if(tm.size() == 1) tm = "0" + tm;
		_kln89->DrawText(th + tm, 2, 11, 3);
		
		char buf[6];
		_kln89->DrawText("Alarm at:", 2, 0, 2);
        _kln89->DrawText("in:", 2, 6, 1);
        if(_alarmAnnotate) {
            _alarmIn = _alarmTime - t;
            snprintf(buf, 5, "%02i", _alarmTime.hr());
            if(!(_uLinePos == 2 && crsr && blink)) { _kln89->DrawText((string)buf, 2, 11, 2); }
			snprintf(buf, 5, "%02i", _alarmTime.min());
			if(!(_uLinePos == 3 && crsr && blink)) { _kln89->DrawText((string)buf, 2, 13, 2); }
		} else {
			if(!(_uLinePos == 2 && crsr && blink)) { _kln89->DrawText("--", 2, 11, 2); }
			if(!(_uLinePos == 3 && crsr && blink)) { _kln89->DrawText("--", 2, 13, 2); }
		}
		if(_alarmAnnotate && _alarmIn.hr() < 10) {
            sprintf(buf, "%01i", _alarmIn.hr());
			if(!(_uLinePos == 4 && crsr && blink)) { _kln89->DrawText((string)buf, 2, 11, 1); }
			sprintf(buf, "%02i", _alarmIn.min());
			if(!(_uLinePos == 5 && crsr && blink)) { _kln89->DrawText((string)buf, 2, 13, 1); }
        } else {
			if(!(_uLinePos == 4 && crsr && blink)) { _kln89->DrawText("-", 2, 11, 1); }
			if(!(_uLinePos == 5 && crsr && blink)) { _kln89->DrawText("--", 2, 13, 1); }
        }
		_kln89->DrawText(":", 2, 12, 1);
		if(crsr) {
			if(_uLinePos == 2) { _kln89->Underline(2, 11, 2, 2); }
			if(_uLinePos == 3) { _kln89->Underline(2, 13, 2, 2); }
			if(_uLinePos == 4) { _kln89->Underline(2, 11, 1, 1); }
			if(_uLinePos == 5) { _kln89->Underline(2, 13, 1, 2); }
		}
        
        // TODO
		_kln89->DrawText("Elapsed", 2, 0, 0);
		ClockTime te = t - _kln89->_powerOnTime;
		// There is the possibility that when we reset it we may end up a minute ahead of the
		// alarm time for a second, so treat 23:59 as 0.00
		if(te.hr() == 23 && te.min() == 59) {
			te.set_hr(0);
			te.set_min(0);
		}
		if(!(_uLinePos == 6 && crsr && blink)) {
			if(te.hr() > 9) {
				// The elapsed time blanks out
				// when past 9:59 on the kln89 sim.
				_kln89->DrawText("-:--", 2, 11, 0);
			} else {
				snprintf(buf, 5, "%01i:%02i", te.hr(), te.min());
				_kln89->DrawText((string)buf, 2, 11, 0);
			}
		}
		if(_uLinePos == 6 && crsr) { _kln89->Underline(2, 11, 0, 4); }
	} else if(_subPage == 3) {
		_kln89->DrawText("PRESSURE ALT", 2, 1, 3);
		_kln89->DrawText("Ind:", 2, 0, 2);
		_kln89->DrawText("Baro:", 2, 0, 1);
		_kln89->DrawText("Prs", 2, 0, 0);
	} else if(_subPage == 4) {
		_kln89->DrawText("DENSITY ALT", 2, 1, 3);
		_kln89->DrawText("Prs:", 2, 0, 2);
		_kln89->DrawText("Temp:", 2, 0, 1);
		_kln89->DrawText("Den", 2, 0, 0);
	} else if(_subPage == 5) {
		_kln89->DrawText("CAS:", 2, 0, 3);
		_kln89->DrawText("Prs:", 2, 0, 2);
		_kln89->DrawText("Temp:", 2, 0, 1);
		_kln89->DrawText("TAS", 2, 0, 0);
	} else if(_subPage == 6) {
		_kln89->DrawText("TAS:", 2, 0, 3);
		_kln89->DrawText("Hdg:", 2, 0, 2);
		_kln89->DrawText("Headwind:", 2, 0, 1);
		_kln89->DrawText("True", 2, 4, 0);
	} else {
		_kln89->DrawText("SUNRISE", 2, 0, 1);
		_kln89->DrawText("SUNSET", 2, 0, 0);
	}

	KLN89Page::Update(dt);
}

void KLN89CalPage::CrsrPressed() {
	if(_kln89->_obsMode) {
		_uLinePos = 0;
	} else {
		_uLinePos = 1;
	}
    if(_subPage == 2) {
        _maxULinePos = 6;
    }
}

void KLN89CalPage::ClrPressed() {
	if(_kln89->_mode != KLN89_MODE_CRSR) {
		KLN89Page::ClrPressed();
	}
    if(_subPage == 2 && _uLinePos == 6) {
		_kln89->ResetPowerOnTimer();
    } else {
		KLN89Page::ClrPressed();
	}
}

void KLN89CalPage::Knob2Left1() {
	if(_kln89->_mode != KLN89_MODE_CRSR) {
		KLN89Page::Knob2Left1();
		return;
	}
	
	if(_subPage == 2) {
		if(_uLinePos == 1) {
			// TODO - allow time zone to be changed
		} else if(_uLinePos == 2) {
			ClockTime t(1,0);
			if(_alarmAnnotate) {
				_alarmTime = _alarmTime - t;
			} else {
				_alarmTime.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
				_alarmTime.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
				_alarmTime = _alarmTime - t;
				_alarmAnnotate = true;
			}
			_alarmSet = true;
		} else if(_uLinePos == 3) {
			ClockTime t(0,1);
			if(_alarmAnnotate) {
				_alarmTime = _alarmTime - t;
			} else {
				_alarmTime.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
				_alarmTime.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
				_alarmTime = _alarmTime - t;
				_alarmAnnotate = true;
			}
			_alarmSet = true;
		}  else if(_uLinePos == 4) {
			ClockTime t(1,0);
			// If the _alarmIn time is dashed out due to being > 9:59
			// then changing it starts from zero again.
			if(_alarmAnnotate && _alarmIn.hr() < 10) {
				_alarmIn = _alarmIn - t;
				if(_alarmIn.hr() > 9) { _alarmIn.set_hr(9); }
			} else {
				_alarmIn.set_hr(9);
				_alarmIn.set_min(0);
				_alarmAnnotate = true;
			}
			_alarmSet = true;
			t.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
			t.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
			_alarmTime = t + _alarmIn;
		} else if(_uLinePos == 5) {
			ClockTime t(0,1);
			if(_alarmAnnotate && _alarmIn.hr() < 10) {
				_alarmIn = _alarmIn - t;
				if(_alarmIn.hr() > 9) { _alarmIn.set_hr(9); }
			} else {
				_alarmIn.set_hr(9);
				_alarmIn.set_min(59);
				_alarmAnnotate = true;
			}
			_alarmSet = true;
			t.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
			t.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
			_alarmTime = t + _alarmIn;
		}
	}
}

void KLN89CalPage::Knob2Right1() {
	if(_kln89->_mode != KLN89_MODE_CRSR) {
		KLN89Page::Knob2Right1();
		return;
	}
	
	if(_subPage == 2) {
		if(_uLinePos == 1) {
			// TODO - allow time zone to be changed
		} else if(_uLinePos == 2) {
			ClockTime t(1,0);
			if(_alarmAnnotate) {
				_alarmTime = _alarmTime + t;
			} else {
				_alarmTime.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
				_alarmTime.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
				_alarmTime = _alarmTime + t;
				_alarmAnnotate = true;
			}
			_alarmSet = true;
		} else if(_uLinePos == 3) {
			ClockTime t(0,1);
			if(_alarmAnnotate) {
				_alarmTime = _alarmTime + t;
			} else {
				_alarmTime.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
				_alarmTime.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
				_alarmTime = _alarmTime + t;
				_alarmAnnotate = true;
			}
			_alarmSet = true;
		}  else if(_uLinePos == 4) {
			ClockTime t(1,0);
			if(_alarmAnnotate && _alarmIn.hr() < 10) {
				_alarmIn = _alarmIn + t;
				if(_alarmIn.hr() > 9) { _alarmIn.set_hr(0); }
			} else {
				_alarmIn.set_hr(1);
				_alarmIn.set_min(0);
				_alarmAnnotate = true;
			}
			_alarmSet = true;
			t.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
			t.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
			_alarmTime = t + _alarmIn;
		} else if(_uLinePos == 5) {
			ClockTime t(0,1);
			if(_alarmAnnotate && _alarmIn.hr() < 10) {
				_alarmIn = _alarmIn + t;
				if(_alarmIn.hr() > 9) { _alarmIn.set_hr(0); }
			} else {
				_alarmIn.set_hr(0);
				_alarmIn.set_min(1);
				_alarmAnnotate = true;
			}
			_alarmSet = true;
			t.set_hr(atoi(fgGetString("/instrumentation/clock/indicated-hour")));
			t.set_min(atoi(fgGetString("/instrumentation/clock/indicated-min")));
			_alarmTime = t + _alarmIn;
		}
	}
}

void KLN89CalPage::LooseFocus() {
	if(_alarmSet) {
		_kln89->SetAlarm(_alarmTime.hr(), _alarmTime.min());
		_alarmSet = false;
	}
}
