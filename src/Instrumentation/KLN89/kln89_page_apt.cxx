// kln89_page_*.[ch]xx - this file is one of the "pages" that
//                       are used in the KLN89 GPS unit simulation. 
//
// Written by David Luff, started 2005.
//
// Copyright (C) 2005 - David C Luff - daveluff AT ntlworld.com
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

#include "kln89_page_apt.hxx"

#include <simgear/structure/exception.hxx>
#include <cassert>

#include <ATC/CommStation.hxx>
#include <Main/globals.hxx>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>

KLN89AptPage::KLN89AptPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 8;
	_subPage = 0;
	_name = "APT";
	_apt_id = "KHWD";
	// Make sure that _last_apt_id doesn't match at startup to force airport data to be fetched on first update.
	_last_apt_id = "XXXX";
	_nRwyPages = 1;
	_curRwyPage = 0;
	_nFreqPages = 1;
	_curFreqPage = 0;
	ap = NULL;
	_iapStart = 0;
	_iafStart = 0;
	_fStart = 0;
	_iaps.clear();
	_iafDialog = false;
	_addDialog = false;
	_replaceDialog = false;
	_curIap = 0;
	_curIaf = 0;
}

KLN89AptPage::~KLN89AptPage() {
}

void KLN89AptPage::Update(double dt) {
	bool actPage = (_kln89->_activePage->GetName() == "ACT" ? true : false);
	bool multi;  // Not set by FindFirst...
	bool exact = false;
	if(_apt_id.size() == 4) exact = true;
	// TODO - move this search out to where the button is pressed, and cache the result!
	if(_apt_id != _last_apt_id || ap == NULL) ap = _kln89->FindFirstAptById(_apt_id, multi, exact);
	//if(np == NULL) cout << "NULL... ";
	//if(b == false) cout << "false...\n";
	/*
	if(np && b) {
		cout << "VOR FOUND!\n";
	} else {
		cout << ":-(\n";
	}
	*/
	
	if(ap) {
		//cout << "Valid airport found! id = " << ap->getId() << ", elev = " << ap->getElevation() << '\n';
		if(_apt_id != _last_apt_id) {
			UpdateAirport(ap->getId());
			_last_apt_id = _apt_id;
			_curFreqPage = 0;
			_curRwyPage = 0;
		}
		_apt_id = ap->getId();
		if(_kln89->GetActiveWaypoint()) {
			if(_apt_id == _kln89->GetActiveWaypoint()->id) {
				if(!(_kln89->_waypointAlert && _kln89->_blink)) {
					// Active waypoint arrow
					_kln89->DrawSpecialChar(4, 2, 0, 3);
				}
			}
		}
		if(_kln89->_mode != KLN89_MODE_CRSR) {
			if(!(_subPage == 7 && (_iafDialog || _addDialog || _replaceDialog))) {	// Don't draw the airport name when the IAP dialogs are active
				if(!_entInvert) {
					if(!actPage) {
						_kln89->DrawText(ap->getId(), 2, 1, 3);
					} else {
						// If it's the ACT page, The ID is shifted slightly right to make space for the waypoint index.
						_kln89->DrawText(ap->getId(), 2, 4, 3);
						char buf[3];
						int n = snprintf(buf, 3, "%i", _kln89->GetActiveWaypointIndex() + 1);
						_kln89->DrawText((string)buf, 2, 3 - n, 3);
					}
				} else {
					if(!_kln89->_blink) {
						_kln89->DrawText(ap->getId(), 2, 1, 3, false, 99);
						_kln89->DrawEnt();
					}
				}
			}
		}
		if(_subPage == 0) {
			// Name
			_kln89->DrawText(ap->getName(), 2, 0, 2);
			// Elevation
			_kln89->DrawText(_kln89->_altUnits == GPS_ALT_UNITS_FT ? "ft" : "m", 2, 14, 3);
			char buf[6];
			int n = snprintf(buf, 5, "%i", (_kln89->_altUnits == GPS_ALT_UNITS_FT ? (int)(ap->getElevation()) : (int)((double)ap->getElevation() * SG_FEET_TO_METER)));
			_kln89->DrawText((string)buf, 2, 14 - n, 3);
			// Town
			airport_id_str_map_iterator itr = _kln89->_airportTowns.find(_apt_id);
			if(itr != _kln89->_airportTowns.end()) {
				_kln89->DrawText(itr->second, 2, 0, 1);
			}
			// State / Province / Country
			itr = _kln89->_airportStates.find(_apt_id);
			if(itr != _kln89->_airportStates.end()) {
				_kln89->DrawText(itr->second, 2, 0, 0);
			}
		} else if(_subPage == 1) {
			_kln89->DrawLatitude(ap->getLatitude(), 2, 3, 2);
			_kln89->DrawLongitude(ap->getLongitude(), 2, 3, 1);
			_kln89->DrawDirDistField(ap->getLatitude() * SG_DEGREES_TO_RADIANS, ap->getLongitude() * SG_DEGREES_TO_RADIANS, 
			                         2, 0, 0, _to_flag, (_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == 5 ? true : false));
		} else if(_subPage == 2) {
			// Try and calculate a realistic difference from UTC based on longitude
			// Since 0 longitude is the middle of UTC, the boundaries will be at 7.5, 22.5, 37.5 etc.
			int hrDiff = ((int)((fabs(ap->getLongitude())) + 7.5)) / 15;
			_kln89->DrawText("UTC", 2, 0, 2);
			if(hrDiff != 0) {
				_kln89->DrawText(ap->getLongitude() >= 0.0 ? "+" : "-", 2, 3, 2);
				char buf[3];
				snprintf(buf, 3, "%02i", hrDiff);
				_kln89->DrawText((string)buf, 2, 4, 2);
				_kln89->DrawText("(   DT)", 2, 6, 2);
				if(ap->getLongitude() >= 0.0) {
					hrDiff++;
				} else {
					hrDiff--;
				}
				_kln89->DrawText(ap->getLongitude() >= 0.0 ? "+" : "-", 2, 7, 2);
				snprintf(buf, 3, "%02i", hrDiff);
				_kln89->DrawText((string)buf, 2, 8, 2);
			}
			// I guess we can make a heuristic guess as to fuel availability from the runway sizes
			// For now assume that airports with asphalt or concrete runways will have at least 100L,
			// and that runways over 4000ft will have JET.
			if(_aptRwys[0]->surface() <= 2) {
				if(_aptRwys[0]->lengthFt() >= 4000) {
					_kln89->DrawText("JET 100L", 2, 0, 1);
				} else {
					_kln89->DrawText("100L", 2, 0, 1);
				}
			}
			if(_iaps.empty()) {
				_kln89->DrawText("NO APR", 2, 0, 0);
			} else {
				// TODO - output proper differentiation of ILS and NP APR and NP APR type eg GPS(R)
				_kln89->DrawText("NP APR", 2, 0, 0);
			}
		} else if(_subPage == 3) {
			if(_nRwyPages > 1) {
				_kln89->DrawChar('+', 1, 3, 0);
			}
			unsigned int i = _curRwyPage * 2;
			string s;
			if(i < _aptRwys.size()) {
				// Rwy No.
				string s = _aptRwys[i]->ident();
				_kln89->DrawText(s, 2, 9, 3);
				_kln89->DrawText("/", 2, 12, 3);
                string recipIdent = _aptRwys[i]->reciprocalRunway()->ident();
				_kln89->DrawText(recipIdent, 2, 13, 3);
				// Length
				s = GPSitoa(int(float(_aptRwys[i]->lengthFt()) * (_kln89->_altUnits == GPS_ALT_UNITS_FT ? 1.0 : SG_FEET_TO_METER) + 0.5));
				_kln89->DrawText(s, 2, 5 - s.size(), 2);
				_kln89->DrawText((_kln89->_altUnits == GPS_ALT_UNITS_FT ? "ft" : "m"), 2, 5, 2);
				// Surface
				// TODO - why not store these strings as an array?
				switch(_aptRwys[i]->surface()) {
				case 1:
					// Asphalt - fall through
				case 2:
					// Concrete
					_kln89->DrawText("HRD", 2, 9, 2);
					break;
				case 3:
				case 8:
					// Turf / Turf helipad
					_kln89->DrawText("TRF", 2, 9, 2);
					break;
				case 4:
				case 9:
					// Dirt / Dirt helipad
					_kln89->DrawText("DRT", 2, 9, 2);
					break;
				case 5:
					// Gravel
					_kln89->DrawText("GRV", 2, 9, 2);
					break;
				case 6:
					// Asphalt helipad - fall through
				case 7:
					// Concrete helipad
					_kln89->DrawText("HRD", 2, 9, 2);
					break;
				case 12:
					// Lakebed
					_kln89->DrawText("CLY", 2, 9, 2);
				default:
					// erm? ...
					_kln89->DrawText("MAT", 2, 9, 2);
				}
			}
			i++;
			if(i < _aptRwys.size()) {
				// Rwy No.
				string s = _aptRwys[i]->ident();
				_kln89->DrawText(s, 2, 9, 1);
				_kln89->DrawText("/", 2, 12, 1);
                string recip = _aptRwys[i]->reciprocalRunway()->ident();
				_kln89->DrawText(recip, 2, 13, 1);
				// Length
				s = GPSitoa(int(float(_aptRwys[i]->lengthFt()) * (_kln89->_altUnits == GPS_ALT_UNITS_FT ? 1.0 : SG_FEET_TO_METER) + 0.5));
				_kln89->DrawText(s, 2, 5 - s.size(), 0);
				_kln89->DrawText((_kln89->_altUnits == GPS_ALT_UNITS_FT ? "ft" : "m"), 2, 5, 0);
				// Surface
				// TODO - why not store these strings as an array?
				switch(_aptRwys[i]->surface()) {
				case 1:
					// Asphalt - fall through
				case 2:
					// Concrete
					_kln89->DrawText("HRD", 2, 9, 0);
					break;
				case 3:
				case 8:
					// Turf / Turf helipad
					_kln89->DrawText("TRF", 2, 9, 0);
					break;
				case 4:
				case 9:
					// Dirt / Dirt helipad
					_kln89->DrawText("DRT", 2, 9, 0);
					break;
				case 5:
					// Gravel
					_kln89->DrawText("GRV", 2, 9, 0);
					break;
				case 6:
					// Asphalt helipad - fall through
				case 7:
					// Concrete helipad
					_kln89->DrawText("HRD", 2, 9, 0);
					break;
				case 12:
					// Lakebed
					_kln89->DrawText("CLY", 2, 9, 0);
				default:
					// erm? ...
					_kln89->DrawText("MAT", 2, 9, 0);
				}
			}
		} else if(_subPage == 4) {
			if(_nFreqPages > 1) {
				_kln89->DrawChar('+', 1, 3, 0);
			}
			unsigned int i = _curFreqPage * 3;
			if(i < _aptFreqs.size()) {
				_kln89->DrawText(_aptFreqs[i].service, 2, 0, 2);
				_kln89->DrawFreq(_aptFreqs[i].freq, 2, 7, 2);
			}
			i++;
			if(i < _aptFreqs.size()) {
				_kln89->DrawText(_aptFreqs[i].service, 2, 0, 1);
				_kln89->DrawFreq(_aptFreqs[i].freq, 2, 7, 1);
			}
			i++;
			if(i < _aptFreqs.size()) {
				_kln89->DrawText(_aptFreqs[i].service, 2, 0, 0);
				_kln89->DrawFreq(_aptFreqs[i].freq, 2, 7, 0);
			}
		} else if(_subPage == 5) {
			// TODO - user ought to be allowed to leave persistent remarks
			_kln89->DrawText("[Remarks]", 2, 2, 2);
		} else if(_subPage == 6) {
			// We don't have SID/STAR database yet
			// TODO
			_kln89->DrawText("No SID/STAR", 2, 3, 2);
			_kln89->DrawText("In Data Base", 2, 2, 1);
			_kln89->DrawText("For This Airport", 2, 0, 0);
		} else if(_subPage == 7) {
			if(_iaps.empty()) {
				_kln89->DrawText("IAP", 2, 11, 3);
				_kln89->DrawText("No Approach", 2, 3, 2);
				_kln89->DrawText("In Data Base", 2, 2, 1);
				_kln89->DrawText("For This Airport", 2, 0, 0);
			} else {
				if(_iafDialog) {
					_kln89->DrawText(_iaps[_curIap]->_ident, 2, 1, 3);
					_kln89->DrawText(_iaps[_curIap]->_rwyStr, 2, 7, 3);
					_kln89->DrawText(_iaps[_curIap]->_aptIdent, 2, 12, 3);
					_kln89->DrawText("IAF", 2, 2, 2);
					unsigned int line = 0;
					for(unsigned int i=_iafStart; i<_approachRoutes.size(); ++i) {
						if(line == 2) {
							i = _approachRoutes.size() - 1;
						}
						// Assume that the IAF number is always single digit!
						_kln89->DrawText(GPSitoa(i+1), 2, 6, 2-line);
						if(!(_kln89->_mode == KLN89_MODE_CRSR && _kln89->_blink && _uLinePos == (line + 1))) {
							_kln89->DrawText(_approachRoutes[i]->waypoints[0]->id, 2, 8, 2-line);
						}
						if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos == (line + 1) && !(_kln89->_blink )) {
							_kln89->Underline(2, 8, 2-line, 5);
						}
						++line;
					}
					if(_uLinePos > 0 && !(_kln89->_blink)) {
						_kln89->DrawEnt();
					}
				} else if(_addDialog) {
					_kln89->DrawText(_iaps[_curIap]->_ident, 2, 1, 3);
					_kln89->DrawText(_iaps[_curIap]->_rwyStr, 2, 7, 3);
					_kln89->DrawText(_iaps[_curIap]->_aptIdent, 2, 12, 3);
					string s = GPSitoa(_fStart + 1);
					_kln89->DrawText(s, 2, 2-s.size(), 2);
					s = GPSitoa(_kln89->_approachFP->waypoints.size());
					_kln89->DrawText(s, 2, 2-s.size(), 1);
					if(!(_uLinePos == _fStart+1 && _kln89->_blink)) {
						_kln89->DrawText(_kln89->_approachFP->waypoints[_fStart]->id, 2, 4, 2);
						if(_uLinePos == _fStart+1) _kln89->Underline(2, 4, 2, 6);
					}
					if(!(_uLinePos == _maxULinePos-1 && _kln89->_blink)) {
						_kln89->DrawText(_kln89->_approachFP->waypoints[_kln89->_approachFP->waypoints.size()-1]->id, 2, 4, 1);
						if(_uLinePos == _maxULinePos-1) _kln89->Underline(2, 4, 1, 6);
					}
					if(!(_uLinePos > _kln89->_approachFP->waypoints.size() && _kln89->_blink)) {
						_kln89->DrawText("ADD TO FPL 0?", 2, 2, 0);
						if(_uLinePos > _kln89->_approachFP->waypoints.size()) {
							_kln89->Underline(2, 2, 0, 13);
							_kln89->DrawEnt();
						}
					}
				} else if(_replaceDialog) {
					_kln89->DrawText(_iaps[_curIap]->_ident, 2, 1, 3);
					_kln89->DrawText(_iaps[_curIap]->_rwyStr, 2, 7, 3);
					_kln89->DrawText(_iaps[_curIap]->_aptIdent, 2, 12, 3);
					_kln89->DrawText("Replace Existing", 2, 0, 2);
					_kln89->DrawText("Approach", 2, 4, 1);
					if(_uLinePos > 0 && !(_kln89->_blink)) {
						_kln89->DrawText("APPROVE?", 2, 4, 0);
						_kln89->Underline(2, 4, 0, 8);
						_kln89->DrawEnt();
					}
				} else {
					_kln89->DrawText("IAP", 2, 11, 3);
					bool selApp = false;
					if(_kln89->_mode == KLN89_MODE_CRSR && _uLinePos > 4) {
						selApp = true;
						if(!_kln89->_blink) _kln89->DrawEnt();
					}
					// _maxULine pos should be 4 + iaps.size() at this point.
					// Draw a maximum of 3 IAPs.
					// If there are more than 3 IAPs for this airport, then we need to offset the start
					// of the list if _uLinePos is pointing at the 4th or later IAP.
					unsigned int offset = 0;
					unsigned int index;
					if(_uLinePos > 7) {
						offset = _uLinePos - 7;
					}
					for(unsigned int i=0; i<3; ++i) {
						index = offset + i;
						if(index < _iaps.size()) {
							string s = GPSitoa(index+1);
							_kln89->DrawText(s, 2, 2 - s.size(), 2-i);
							if(!(selApp && _uLinePos == index+5 && _kln89->_blink)) {
								_kln89->DrawText(_iaps[index]->_ident, 2, 3, 2-i);
								_kln89->DrawText(_iaps[index]->_rwyStr, 2, 9, 2-i);
							}
							if(selApp && _uLinePos == index+5 && !_kln89->_blink) {
								_kln89->Underline(2, 3, 2-i, 9);
							}
						} else {
							break;
						}
					}
				}
			}
		}
	} else {
		if(_kln89->_mode != KLN89_MODE_CRSR) _kln89->DrawText(_apt_id, 2, 1, 3);
		if(_subPage == 0) {
			/*
			_kln89->DrawText("----.-", 2, 9, 3);
			_kln89->DrawText("--------------", 2, 0, 2);
			_kln89->DrawText("- -- --.--'", 2, 3, 1);
			_kln89->DrawText("---- --.--'", 2, 3, 0);
			_kln89->DrawSpecialChar(0, 2, 7, 1);
			_kln89->DrawSpecialChar(0, 2, 7, 0);
			*/
		}
	}
	
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(!(_subPage == 7 && (_iafDialog || _addDialog || _replaceDialog))) {
			if(_uLinePos > 0 && _uLinePos < 5) {
				// TODO - blink as well
				_kln89->Underline(2, _uLinePos, 3, 1);
			}
			for(unsigned int i = 0; i < _apt_id.size(); ++i) {
				if(_uLinePos != (i + 1)) {
					_kln89->DrawChar(_apt_id[i], 2, i + 1, 3);
				} else {
					if(!_kln89->_blink) _kln89->DrawChar(_apt_id[i], 2, i + 1, 3);
				}
			}
		}
	}
	
	_id = _apt_id;

	KLN89Page::Update(dt);
}

void KLN89AptPage::SetId(const string& s) {
	if(s != _apt_id || s != _last_apt_id) {
		UpdateAirport(s);	// If we don't do this here we break things if s is the same as the current ID since the update wouldn't get called then.
		/*
			DCL: Hmmm - I wrote the comment above, but I don't quite understand it!
			I'm not quite sure why I don't simply set _apt_id here (and NOT _last_apt_id)
			and let the logic in Update(...) handle the airport details cache update.
		*/
	}
	_last_apt_id = _apt_id;
	_save_apt_id = _apt_id;
	_apt_id = s;
}

// Update the cached airport details
void KLN89AptPage::UpdateAirport(const string& id) {
	// Frequencies
	_aptFreqs.clear();	
	
    const FGAirport* apt = fgFindAirportID(id);
    if (!apt) {
        throw sg_exception("UpdateAirport: unknown airport id " + id);
    }
	
    for (unsigned int c=0; c<apt->commStations().size(); ++c) {
        flightgear::CommStation* comm = apt->commStations()[c];
        AptFreq aq;
        aq.freq = comm->freqKHz();
        switch (comm->type()) {
        case FGPositioned::FREQ_ATIS:
            aq.service = "ATIS*"; break;
        case FGPositioned::FREQ_GROUND:
            aq.service = "GRND*"; break;
        case FGPositioned::FREQ_TOWER:
            aq.service = "TWR *"; break;
        case FGPositioned::FREQ_APP_DEP:
            aq.service = "APR *"; break;
        default:
            continue;
        }
    }

	_nFreqPages = (unsigned int)ceil((float(_aptFreqs.size())) / 3.0f);
	
	// Runways
	_aptRwys.clear();
  
  // build local array, longest runway first
  for (unsigned int r=0; r<apt->numRunways(); ++r) {
    FGRunway* rwy(apt->getRunwayByIndex(r));
    if ((r > 0) && (rwy->lengthFt() > _aptRwys.front()->lengthFt())) {
      _aptRwys.insert(_aptRwys.begin(), rwy);
    } else {
      _aptRwys.push_back(rwy);
    }
  }
  
	_nRwyPages = (_aptRwys.size() + 1) / 2;	// 2 runways per page.
	if(_nFreqPages < 1) _nFreqPages = 1;
	if(_nRwyPages < 1) _nRwyPages = 1;
	
	// Instrument approaches
	// Only non-precision for now - TODO - handle precision approaches if necessary
	_iaps.clear();
	iap_map_iterator itr = _kln89->_np_iap.find(id);
	if(itr != _kln89->_np_iap.end()) {
		_iaps = itr->second;
	}
	if(_subPage == 7) { 
		if(_iafDialog || _addDialog || _replaceDialog) {
			// Eek - major logic error if an airport details cache update occurs
			// with one of these dialogs active.
			// TODO - output a warning.
			//cout << "HELP!!!!!!!!!!\n";
		} else {
			_maxULinePos = 4 + _iaps.size();	// We shouldn't need to check the crsr for out-of-bounds here since we only update the airport details when the airport code is changed - ie. _uLinePos <= 4!
		}
	}
}

void KLN89AptPage::CrsrPressed() {
	if(_kln89->_mode == KLN89_MODE_DISP) {
		if(_subPage == 7) {
			// Pressing crsr jumps back to vanilla IAP page.
			_iafDialog = false;
			_addDialog = false;
			_replaceDialog = false;
		}
		return;
	}
	if(_kln89->_obsMode) {
		_uLinePos = 0;
	} else {
		_uLinePos = 1;
	}
	if(_subPage == 0) {
		_maxULinePos = 32;
	} else if(_subPage == 7) {
		// Don't *think* we need some of this since some of it we can only get to by pressing ENT, not CRSR.
		if(_iafDialog) {
			_maxULinePos = _approachRoutes.size();
			_uLinePos = 1;
		} else if(_addDialog) {
			_maxULinePos = 1;
			_uLinePos = 1;
		} else if(_replaceDialog) {
			_maxULinePos = 1;
			_uLinePos = 1;
		} else {
			_maxULinePos = 4 + _iaps.size();
			if(_iaps.empty()) {
				_uLinePos = 1;
			} else {
				_uLinePos = 5;
			}
		}
	} else {
		_maxULinePos = 5;
	}
}

void KLN89AptPage::ClrPressed() {
	if(_subPage == 1 && _uLinePos == 5) {
		_to_flag = !_to_flag;
	} else if(_subPage == 7) {
		// Clear backs out IAP selection one step at a time
		if(_iafDialog) {
			_iafDialog = false;
			_maxULinePos = 4 + _iaps.size();
			if(_iaps.empty()) {
				_uLinePos = 1;
			} else {
				_uLinePos = 5;
			}
		} else if(_addDialog) {
			_addDialog = false;
			if(_approachRoutes.size() > 1) {
				_iafDialog = true;
				_maxULinePos = 1;
				// Don't reset _curIaf since it is remembed.
				_uLinePos = 1 + _curIaf;	// TODO - make this robust to more than 3 IAF
			} else {
				_maxULinePos = 4 + _iaps.size();
				if(_iaps.empty()) {
					_uLinePos = 1;
				} else {
					_uLinePos = 5;
				}
			}			
		} else if(_replaceDialog) {
			_replaceDialog = false;
			_addDialog = true;
			_maxULinePos = 1;
			_uLinePos = 1;
		}
	}
}

void KLN89AptPage::EntPressed() {
	if(_entInvert) {
		_entInvert = false;
		if(_kln89->_dtoReview) {
			_kln89->DtoInitiate(_apt_id);
		} else {
			_last_apt_id = _apt_id;
			_apt_id = _save_apt_id;
		}
	} else if(_subPage == 7 && _kln89->_mode == KLN89_MODE_CRSR && _uLinePos > 0) {
		// We are selecting an approach
		if(_iafDialog) {
			if(_uLinePos > 0) {
				// Record the IAF that was picked
				if(_uLinePos == 3) {
					_curIaf = _approachRoutes.size() - 1;
				} else {
					_curIaf = _uLinePos - 1 + _iafStart;
				}
				//cout << "_curIaf = " << _curIaf << '\n';
				// TODO - delete the waypoints inside _approachFP before clearing them!!!!!!!
				_kln89->_approachFP->waypoints.clear();
				GPSWaypoint* wp = new GPSWaypoint;
				*wp = *(_approachRoutes[_curIaf]->waypoints[0]);	// Need to make copies here since we're going to alter ID and type sometimes
				string iafid = wp->id;
				_kln89->_approachFP->waypoints.push_back(wp);
				for(unsigned int i=0; i<_IAP.size(); ++i) {
					if(_IAP[i]->id != iafid) {	// Don't duplicate waypoints that are part of the initial fix list and the approach procedure list.
                        // FIXME - allow the same waypoint to be both the IAF and the FAF in some
                        // approaches that have a procedure turn eg. KDLL
                        // Also allow MAF to be the same as IAF!
						wp = new GPSWaypoint;
						*wp = *_IAP[i];
						//cout << "Adding waypoint " << wp->id << ", type is " << wp->appType << '\n';
						//if(wp->appType == GPS_FAF) wp->id += 'f';
						//if(wp->appType == GPS_MAP) wp->id += 'm';
						//cout << "New id = " << wp->id << '\n';
						_kln89->_approachFP->waypoints.push_back(wp);
					}
				}
				_iafDialog = false;
				_addDialog = true;
				_maxULinePos = _kln89->_approachFP->waypoints.size() + 1;
				_uLinePos = _maxULinePos;
			}
		} else if(_addDialog) {
			if(_uLinePos == _maxULinePos) {
				_addDialog = false;
				if(_kln89->ApproachLoaded()) {
					_replaceDialog = true;
					_uLinePos = 1;
					_maxULinePos = 1;
				} else {
                    // Now load the approach into the active flightplan.
                    // As far as I can tell, the rules are this:
                    // If the airport of the approach is in the flightplan, insert it prior to this.  (Not sure what happens if airport has already been passed).
                    // If the airport is not in the flightplan, append the approach to the flightplan, even if it is closer than the current active leg,
                    // in which case reorientate to flightplan might put us on the approach, but unable to activate it.
                    // However, it appears from the sim as if this can indeed happen if the user is not carefull.
                    bool added = false;
                    for(unsigned int i=0; i<_kln89->_activeFP->waypoints.size(); ++i) {
                        if(_kln89->_activeFP->waypoints[i]->id == _apt_id) {
                            _kln89->_activeFP->waypoints.insert(_kln89->_activeFP->waypoints.begin()+i, _kln89->_approachFP->waypoints.begin(), _kln89->_approachFP->waypoints.end());
                            added = true;
                            break;
                        }
                    }
                    if(!added) {
                        _kln89->_activeFP->waypoints.insert(_kln89->_activeFP->waypoints.end(), _kln89->_approachFP->waypoints.begin(), _kln89->_approachFP->waypoints.end());
                    }
					_kln89->_approachID = _apt_id;
					_kln89->_approachAbbrev = _iaps[_curIap]->_ident;
					_kln89->_approachRwyStr = _iaps[_curIap]->_rwyStr;
					_kln89->_approachLoaded = true;
					//_kln89->_messageStack.push_back("*Press ALT To Set Baro");
					// Actually - this message is only sent when we go into appraoch-arm mode.
					// TODO - check the flightplan for consistency
					_kln89->OrientateToActiveFlightPlan();
					_kln89->_mode = KLN89_MODE_DISP;
					_kln89->_curPage = 7;
					_kln89->_activePage = _kln89->_pages[7];	// Do we need to clean up here at all before jumping?
				}
			}
		} else if(_replaceDialog) {
			// TODO - load the approach!
		} else if(_uLinePos > 4) {
			_approachRoutes.clear();
			_IAP.clear();
			_curIaf = 0;
			_approachRoutes = ((FGNPIAP*)(_iaps[_uLinePos-5]))->_approachRoutes;
			_IAP = ((FGNPIAP*)(_iaps[_uLinePos-5]))->_IAP;
			_curIap = _uLinePos - 5;	// TODO - handle the start of list ! no. 1, and the end of list not sequential!
			_uLinePos = 1;
			if(_approachRoutes.size() > 1) {
				// More than 1 IAF - display the selection dialog
				_iafDialog = true;
				_maxULinePos = _approachRoutes.size();
			} else {
				// There is only 1 IAF, so load the waypoints into the approach flightplan here.
				// TODO - there is nasty code duplication loading the approach FP between the case here where we have only one
				// IAF and the case where we must choose the IAF from a list.  Try to tidy this after it is all working properly.
				_kln89->_approachFP->waypoints.clear();
				GPSWaypoint* wp = new GPSWaypoint;
				*wp = *(_approachRoutes[0]->waypoints[0]);	// Need to make copies here since we're going to alter ID and type sometimes
				string iafid = wp->id;
				_kln89->_approachFP->waypoints.push_back(wp);
				for(unsigned int i=0; i<_IAP.size(); ++i) {
					if(_IAP[i]->id != iafid) {	// Don't duplicate waypoints that are part of the initial fix list and the approach procedure list.
						// FIXME - allow the same waypoint to be both the IAF and the FAF in some
						// approaches that have a procedure turn eg. KDLL
						// Also allow MAF to be the same as IAF!
						wp = new GPSWaypoint;
						*wp = *_IAP[i];
						_kln89->_approachFP->waypoints.push_back(wp);
					}
				}
				_addDialog = true;
				_maxULinePos = 1;
			}
		}
	}
}

void KLN89AptPage::Knob1Left1() {
	if(_kln89->_mode == KLN89_MODE_CRSR && _subPage == 7 && _addDialog) {
		if(_uLinePos == _maxULinePos) {
			_uLinePos--;
			if(_kln89->_approachFP->waypoints.size() > 1) _fStart = _kln89->_approachFP->waypoints.size() - 2;
		} else if(_uLinePos == _maxULinePos - 1) {
			_uLinePos--;
		} else if(_uLinePos > 0) {
			if(_fStart == 0) {
				_uLinePos--;
			} else {
				_uLinePos--;
				_fStart--;
			}
		}
	} else {
		KLN89Page::Knob1Left1();
	}
}

void KLN89AptPage::Knob1Right1() {
	if(_kln89->_mode == KLN89_MODE_CRSR && _subPage == 7 && _addDialog) {
		if(_uLinePos == _maxULinePos) {
			// no-op
		} else if(_uLinePos == _maxULinePos - 1) {
			_uLinePos++;
			_fStart = 0;
		} else if(_uLinePos > 0) {
			if(_fStart >= _kln89->_approachFP->waypoints.size() - 2) {
				_uLinePos++;
			} else {
				_uLinePos++;
				_fStart++;
			}
		} else if(_uLinePos == 0) {
			_uLinePos++;
			_fStart = 0;
		}
	} else {
		KLN89Page::Knob1Right1();
	}
}

void KLN89AptPage::Knob2Left1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		if(_uLinePos == 0 && _kln89->_mode == KLN89_MODE_CRSR && _kln89->_obsMode) {
			KLN89Page::Knob2Left1();
		} else if(_subPage == 5) {
			_subPage = 4;
			_curFreqPage = _nFreqPages - 1;
		} else if(_subPage == 4) {
			// Freqency pages
			if(_curFreqPage == 0) {
				_subPage = 3;
				_curRwyPage = _nRwyPages - 1;
			} else {
				_curFreqPage--;
			}
		} else if(_subPage == 3) {
			if(_curRwyPage == 0) {
				KLN89Page::Knob2Left1();
			} else {
				_curRwyPage--;
			}
		} else if(_subPage == 0) {
			_subPage = 7;
			// We have to set _uLinePos here even though the cursor isn't pressed, to
			// ensure that the list displays properly.
			if(_iaps.empty()) {
				_uLinePos = 1;
			} else {
				_uLinePos = 5;
			}
		} else {
			KLN89Page::Knob2Left1();
		}
	} else {
		if(_uLinePos < 5 && !(_subPage == 7 && (_iafDialog || _addDialog || _replaceDialog))) {
			// Same logic for all pages - set the ID
			_apt_id = _apt_id.substr(0, _uLinePos);
			// ASSERT(_uLinePos > 0);
			if(_uLinePos == (_apt_id.size() + 1)) {
				_apt_id += '9';
			} else {
				_apt_id[_uLinePos - 1] = _kln89->DecChar(_apt_id[_uLinePos - 1], (_uLinePos == 1 ? false : true));
			}
		} else {
			if(_subPage == 0) {
				// TODO - set by name
			} else {
				// NO-OP - to/fr is cycled by clr button
			}
		}
	}
}

void KLN89AptPage::Knob2Right1() {
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		if(_uLinePos == 0 && _kln89->_mode == KLN89_MODE_CRSR && _kln89->_obsMode) {
			KLN89Page::Knob2Right1();
		} else if(_subPage == 2) {
			_subPage = 3;
			_curRwyPage = 0;
		} else if(_subPage == 3) {
			if(_curRwyPage == _nRwyPages - 1) {
				_subPage = 4;
				_curFreqPage = 0;
			} else {
				_curRwyPage++;
			}
		} else if(_subPage == 4) {
			if(_curFreqPage == _nFreqPages - 1) {
				_subPage = 5;
			} else {
				_curFreqPage++;
			}
		} else if(_subPage == 6) {
			_subPage = 7;
			// We have to set _uLinePos here even though the cursor isn't pressed, to
			// ensure that the list displays properly.
			if(_iaps.empty()) {
				_uLinePos = 1;
			} else {
				_uLinePos = 5;
			}
		} else {
			KLN89Page::Knob2Right1();
		}
	} else {
		if(_uLinePos < 5 && !(_subPage == 7 && (_iafDialog || _addDialog || _replaceDialog))) {
			// Same logic for all pages - set the ID
			_apt_id = _apt_id.substr(0, _uLinePos);
			// ASSERT(_uLinePos > 0);
			if(_uLinePos == (_apt_id.size() + 1)) {
				_apt_id += 'A';
			} else {
				_apt_id[_uLinePos - 1] = _kln89->IncChar(_apt_id[_uLinePos - 1], (_uLinePos == 1 ? false : true));
			}
		} else {
			if(_subPage == 0) {
				// TODO - set by name
			} else {
				// NO-OP - to/fr is cycled by clr button
			}
		}
	}
}
