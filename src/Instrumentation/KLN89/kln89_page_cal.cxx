// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff - david.luff@nottingham.ac.uk
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

#include "kln89_page_cal.hxx"

KLN89CalPage::KLN89CalPage(KLN89* parent)
: KLN89Page(parent) {
	_nSubPages = 8;
	_subPage = 0;
	_name = "CAL";
	_nFp0 = 0;
	_ground_speed_ms = 110 * 0.514444444444;
}

KLN89CalPage::~KLN89CalPage() {
}

void KLN89CalPage::Update(double dt) {
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
		_kln89->DrawText("Alarm at:", 2, 0, 2);
		_kln89->DrawText("in:", 2, 6, 1);
		_kln89->DrawText("Elapsed", 2, 0, 0);
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
