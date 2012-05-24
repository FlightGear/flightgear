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

#include <cstdlib>

#include "kln89_page_nav.hxx"
#include <Main/fg_props.hxx>

using std::string;

KLN89NavPage::KLN89NavPage(KLN89* parent) 
: KLN89Page(parent) {
	_nSubPages = 4;
	_subPage = 0;
	_name = "NAV";
	_posFormat = 0;		// Check - should this default to ref from waypoint?
	_vnv = 0;
	_nav4DataSnippet = 0;
	_cdiFormat = 0;
	_menuActive = false;
	_menuPos = 0;
	_suspendAVS = false;
	_scanWpSet = false;
	_scanWpIndex = -1;
}

KLN89NavPage::~KLN89NavPage() {
}

void KLN89NavPage::Update(double dt) {
	GPSFlightPlan* fp = _kln89->_activeFP;
	GPSWaypoint* awp = _kln89->GetActiveWaypoint();
	// Scan-pull out on nav4 page switches off the cursor
	if(3 == _subPage && fgGetBool("/instrumentation/kln89/scan-pull")) { _kln89->_mode = KLN89_MODE_DISP; }
	bool crsr = (_kln89->_mode == KLN89_MODE_CRSR);
	bool blink = _kln89->_blink;
	double lat = _kln89->_gpsLat * SG_RADIANS_TO_DEGREES;
	double lon = _kln89->_gpsLon * SG_RADIANS_TO_DEGREES;
	
	if(_subPage != 3) { _scanWpSet = false; }
	
	if(0 == _subPage) {
		if(_kln89->_navFlagged) {
			_kln89->DrawText(">    F L A G", 2, 0, 2);
			_kln89->DrawText("DTK ---  TK ---", 2, 0, 1);
			_kln89->DrawText(">--- To    --:--", 2, 0, 0);
			_kln89->DrawSpecialChar(0, 2, 7, 1);
			_kln89->DrawSpecialChar(0, 2, 15, 1);
			_kln89->DrawSpecialChar(0, 2, 4, 0);
			_kln89->DrawSpecialChar(1, 2, 3, 2);
			_kln89->DrawSpecialChar(1, 2, 4, 2);
			_kln89->DrawSpecialChar(1, 2, 6, 2);
			_kln89->DrawSpecialChar(1, 2, 10, 2);
			_kln89->DrawSpecialChar(1, 2, 12, 2);
			_kln89->DrawSpecialChar(1, 2, 13, 2);
		} else {
			if(_kln89->_dto) {
				_kln89->DrawDTO(2, 7, 3);
			} else {
				if(!(_kln89->_waypointAlert && _kln89->_blink)) {
					_kln89->DrawSpecialChar(3, 2, 8, 3);
				}
			}
			_kln89->DrawText(awp->id, 2, 10, 3);
			if(!_kln89->_dto && !_kln89->_obsMode && !_kln89->_fromWaypoint.id.empty()) {
				if(_kln89->_fromWaypoint.type != GPS_WP_VIRT) {		// Don't draw the virtual waypoint names
					_kln89->DrawText(_kln89->_fromWaypoint.id, 2, 1, 3);
				}
			}
			if(!(crsr && blink && _uLinePos == 1)) {
				if(_cdiFormat == 0) {
					_kln89->DrawCDI();
				} else if(_cdiFormat == 1) {
					_kln89->DrawText("Fly", 2, 2, 2);
					double x = _kln89->CalcCrossTrackDeviation();
					// TODO - check the R/L from sign of x below - I *think* it holds but not sure!
					// Note also that we're setting Fly R or L based on the aircraft
					// position only, not the heading.  Not sure if this is correct or not.
					_kln89->DrawText(x < 0.0 ? "R" : "L", 2, 6, 2);
					char buf[6];
					int n;
					x = fabs(x * (_kln89->_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001));
					if(x < 1.0) {
						n = snprintf(buf, 6, "%0.2f", x);
					} else if(x < 100.0) {
						n = snprintf(buf, 6, "%0.1f", x);
					} else {
						n = snprintf(buf, 6, "%i", (int)(x+0.5));
					}
					_kln89->DrawText((string)buf, 2, 13-n, 2);
					_kln89->DrawText(_kln89->_distUnits == GPS_DIST_UNITS_NM ? "nm" : "km", 2, 13, 2);
				} else {
					_kln89->DrawText("CDI Scale:", 2, 1, 2);
					double d = _kln89->_cdiScales[_kln89->_currentCdiScaleIndex] * (_kln89->_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001);
					char buf[5];
					string s;
					if(d >= 1.0) {
						snprintf(buf, 4, "%2.1f", d);
						s = buf;
					} else {
						snprintf(buf, 5, "%2.2f", d);
						// trim the leading zero
						s = buf;
						s = s.substr(1, s.size() - 1);
					}
					_kln89->DrawText(s, 2, 11, 2);
					_kln89->DrawText(_kln89->_distUnits == GPS_DIST_UNITS_NM ? "nm" : "km", 2, 14, 2);
				}
			}
			_kln89->DrawChar('>', 2, 0, 2);
			_kln89->DrawChar('>', 2, 0, 0);
			if(crsr) {
				if(_uLinePos == 1) _kln89->Underline(2, 1, 2, 15);
				else if(_uLinePos == 2) _kln89->Underline(2, 1, 0, 9);
			}
			// Desired and actual magnetic track
			if(!_kln89->_obsMode) {
				_kln89->DrawText("DTK", 2, 0, 1);
				_kln89->DrawHeading((int)_kln89->_dtkMag, 2, 7, 1);
			}
			_kln89->DrawText("TK", 2, 9, 1);
			if(_kln89->_groundSpeed_ms > 3) {	// about 6 knots, don't know exactly what value to disable track
				// The trouble with relying on FG gps's track value is we don't know when it's valid.
				_kln89->DrawHeading((int)_kln89->_magTrackDeg, 2, 15, 1);
			} else {
				_kln89->DrawText("---", 2, 12, 1);
				_kln89->DrawSpecialChar(0, 2, 15, 1);
			}
			
			// Radial to/from active waypoint.
			// TODO - Not sure if this either is or should be true or mag!!!!!!!
			if(!(crsr && blink && _uLinePos == 2)) {
				if(0 == _vnv) {
					_kln89->DrawHeading((int)_kln89->GetHeadingToActiveWaypoint(), 2, 4, 0);
					_kln89->DrawText("To", 2, 5, 0);
				} else if(1 == _vnv) {
					_kln89->DrawHeading((int)_kln89->GetHeadingFromActiveWaypoint(), 2, 4, 0);
					_kln89->DrawText("Fr", 2, 5, 0);
				} else {
					_kln89->DrawText("Vnv Off", 2, 1, 0);
				}
			}
			// It seems that the floating point groundspeed must be at least 30kt
			// for an ETA to be calculated.  Note that this means that (integer) 30kt
			// can appear in the frame 1 display both with and without an ETA displayed.
			// TODO - need to switch off track (and heading bug change) based on instantaneous speed as well
			// since the long gps lag filter means we can still be displaying when stopped on ground.
			if(_kln89->_groundSpeed_kts > 30.0) {
				// Assuming eta display is always hh:mm
				// Does it ever switch to seconds when close?
				if(_kln89->_eta / 3600.0 > 100.0) {
					// More that 100 hours ! - Doesn't fit.
					_kln89->DrawText("--:--", 2, 11, 0);
				} else {
					_kln89->DrawTime(_kln89->_eta, 2, 15, 0);
				}
			} else {
				_kln89->DrawText("--:--", 2, 11, 0);
			}
		}
	} else if(1 == _subPage) {
		// Present position
		_kln89->DrawChar('>', 2, 1, 3);
		if(!(crsr && blink && _uLinePos == 1)) _kln89->DrawText("PRESENT POSN", 2, 2, 3);
		if(crsr && _uLinePos == 1) _kln89->Underline(2, 2, 3, 12);
		if(0 == _posFormat) {
			// Lat/lon
			_kln89->DrawLatitude(lat, 2, 3, 1);
			_kln89->DrawLongitude(lon, 2, 3, 0);
		} else {
			// Ref from wp - defaults to nearest vor (and resets to default when page left and re-entered).
		}
	} else if(2 == _subPage) {
		_kln89->DrawText("Time", 2, 0, 3);
		// TODO - hardwired to UTC at the moment
		_kln89->DrawText("UTC", 2, 6, 3);
		string th = fgGetString("/instrumentation/clock/indicated-hour");
		string tm = fgGetString("/instrumentation/clock/indicated-min");
		if(th.size() == 1) th = "0" + th;
		if(tm.size() == 1) tm = "0" + tm;
		_kln89->DrawText(th + tm, 2, 11, 3);
		_kln89->DrawText("Depart", 2, 0, 2);
		_kln89->DrawText(_kln89->_departureTimeString, 2, 11, 2);
		_kln89->DrawText("ETA", 2, 0, 1);
		if(_kln89->_departed) {
			/* Rules of ETA waypoint are:
			 If the active waypoint is part of the active flightplan, then display
			 the ETA to the final (destination) waypoint of the active flightplan.
			 If the active waypoint is not part of the active flightplan, then
			 display the ETA to the active waypoint. */
			// TODO - implement the above properly - we haven't below!
			string wid = "";
			if(fp->waypoints.size()) {
				wid = fp->waypoints[fp->waypoints.size() - 1]->id;
			} else if(awp) {
				wid = awp->id;
			}
			if(!wid.empty()) {
				_kln89->DrawText(wid, 2, 4, 1);
				double tsec = _kln89->GetTimeToWaypoint(wid);
				if(tsec < 0.0) {
					_kln89->DrawText("----", 2, 11, 1);
				} else {
					int etah = (int)tsec / 3600;
					int etam = ((int)tsec - etah * 3600) / 60;
					etah += atoi(fgGetString("/instrumentation/clock/indicated-hour"));
					etam += atoi(fgGetString("/instrumentation/clock/indicated-min"));
					while(etam > 59) {
						etam -= 60;
						etah += 1;
					}
					while(etah > 23) {
						etah -= 24;
					}
					char buf[6];
					int n = snprintf(buf, 6, "%02i%02i", etah, etam);
					_kln89->DrawText((string)buf, 2, 15-n, 1);
				}
			} else {
				_kln89->DrawText("----", 2, 11, 1);
			}
		} else {
			_kln89->DrawText("----", 2, 11, 1);
		}
		_kln89->DrawText("Flight", 2, 0, 0);
		if(_kln89->_departed) {
			int eh = (int)_kln89->_elapsedTime / 3600;
			int em = ((int)_kln89->_elapsedTime - eh * 3600) / 60;
			char buf[6];
			int n = snprintf(buf, 6, "%i:%02i", eh, em);
			_kln89->DrawText((string)buf, 2, 15-n, 0);
		} else {
			_kln89->DrawText("-:--", 2, 11, 0);
		}
	} else {	// if(3 == _subPage)
		//
		// Switch the cursor off if scan-pull is out on this page.
		//
		if(fgGetBool("/instrumentation/kln89/scan-pull")) { _kln89->_mode = KLN89_MODE_DISP; } 
		
		//
		// Draw the moving map if valid.
		// We call the core KLN89 class to do this.
		//
		if(_kln89->_mapOrientation == 2 && _kln89->_groundSpeed_kts < 2) {
			// Don't draw it if in track up mode and groundspeed < 2kts, as per real-life unit.
		} else {
			_kln89->DrawMap(!_suspendAVS);
		}
		
		//
		// Now that the map has been drawn, add the annotation (scale, etc).
		//
		int scale = KLN89MapScales[_kln89->_mapScaleUnits][_kln89->_mapScaleIndex];
		string scle_str = GPSitoa(scale);
		if(crsr) {
			if(_menuActive) {
				// Draw a background quad to encompass on/off for the first three at 'off' length
				_kln89->DrawMapQuad(28, 9, 48, 36, true);
				_kln89->DrawMapText("SUA:", 1, 27, true);
				if(!(_menuPos == 0 && _kln89->_blink)) _kln89->DrawMapText((_kln89->_drawSUA ? "on" : "off"), 29, 27, true);
				if(_menuPos == 0) _kln89->DrawLine(28, 27, 48, 27);
				_kln89->DrawMapText("VOR:", 1, 18, true);
				if(!(_menuPos == 1 && _kln89->_blink)) _kln89->DrawMapText((_kln89->_drawVOR ? "on" : "off"), 29, 18, true);
				if(_menuPos == 1) _kln89->DrawLine(28, 18, 48, 18);
				_kln89->DrawMapText("APT:", 1, 9, true);
				if(!(_menuPos == 2 && _kln89->_blink)) _kln89->DrawMapText((_kln89->_drawApt ? "on" : "off"), 29, 9, true);
				if(_menuPos == 2) _kln89->DrawLine(28, 9, 48, 9);
				_kln89->DrawMapQuad(0, 0, 27, 8, true);
				if(!(_menuPos == 3 && _kln89->_blink)) {
					if(_kln89->_mapOrientation == 0) {
						_kln89->DrawMapText("N", 1, 0, true);
						_kln89->DrawMapUpArrow(7, 1);
					} else if(_kln89->_mapOrientation == 1) {
						_kln89->DrawMapText("DTK", 1, 0, true);
						_kln89->DrawMapUpArrow(21, 1);
					} else {
						// Don't bother with heading up for now!
						_kln89->DrawMapText("TK", 1, 0, true);
						_kln89->DrawMapUpArrow(14, 1);
					}
				}
				if(_menuPos == 3) _kln89->DrawLine(0, 0, 27, 0);
			} else {
				if(_uLinePos == 2) {
					if(!_kln89->_blink) {
						_kln89->DrawMapText("Menu?", 1, 9, true);
						_kln89->DrawEnt();
						_kln89->DrawLine(0, 9, 34, 9);
					} else {
						_kln89->DrawMapQuad(0, 9, 34, 17, true);
					}
				} else {
					_kln89->DrawMapText("Menu?", 1, 9, true);
				}
				// right-justify the scale when _uLinePos == 3
				if(!(_uLinePos == 3 && _kln89->_blink)) _kln89->DrawMapText(scle_str, (_uLinePos == 3 ? 29 - (scle_str.size() * 7) : 1), 0, true);
				if(_uLinePos == 3) _kln89->DrawLine(0, 0, 27, 0);
			}
		} else {
			// Just draw the scale
			_kln89->DrawMapText(scle_str, 1, 0, true);
		}
		// If the scan-pull knob is out, draw one of the waypoints (if applicable).
		if(fgGetBool("/instrumentation/kln89/scan-pull")) {
			if(_kln89->_activeFP->waypoints.size()) {
				//cout << "Need to draw a waypoint!\n";
				_kln89->DrawLine(70, 0, 111, 0);
				if(!_kln89->_blink) {
					//_kln89->DrawMapQuad(45, 0, 97, 8, true);
					if(!_scanWpSet) {
						_scanWpIndex = _kln89->GetActiveWaypointIndex();
						_scanWpSet = true;
					}
					_kln89->DrawMapText(_kln89->_activeFP->waypoints[_scanWpIndex]->id, 71, 0, true);
				}		
			}
		}
		
		//
		// Do part of the field 1 update, since NAV 4 is a special case for the last line.
		//
		_kln89->DrawChar('>', 1, 0, 0);
		if(crsr && _uLinePos == 1) _kln89->Underline(1, 1, 0, 5);
		if(!(crsr && _uLinePos == 1 && _kln89->_blink)) {
			if(_kln89->_obsMode && _nav4DataSnippet == 0) _nav4DataSnippet = 1;
			double tsec;
			switch(_nav4DataSnippet) {
			case 0:
				// DTK
				_kln89->DrawLabel("DTK", -39, 6); 
				// TODO - check we have an active FP / dtk and draw dashes if not.
				char buf0[4];
				snprintf(buf0, 4, "%03i", (int)(_kln89->_dtkMag));
				_kln89->DrawText((string)buf0, 1, 3, 0);
				break;
			case 1:
				// groundspeed
				_kln89->DrawSpeed(_kln89->_groundSpeed_kts, 1, 5, 0);
				break;
			case 2:
				// ETE
				tsec = _kln89->GetETE();
				if(tsec < 0.0) {
					_kln89->DrawText("--:--", 1, 1, 0);
				} else {
					int eteh = (int)tsec / 3600;
					int etem = ((int)tsec - eteh * 3600) / 60;
					char buf[6];
					int n = snprintf(buf, 6, "%02i:%02i", eteh, etem);
					_kln89->DrawText((string)buf, 1, 6-n, 0);
				}
				break;
			case 3:
				// Cross-track correction
				double x = _kln89->CalcCrossTrackDeviation();
				if(x < 0.0) {
					_kln89->DrawSpecialChar(3, 1, 5, 0);
				} else {
					_kln89->DrawSpecialChar(7, 1, 5, 0);
				}
				char buf3[6];
				int n;
				x = fabs(x * (_kln89->_distUnits == GPS_DIST_UNITS_NM ? 1.0 : SG_NM_TO_METER * 0.001));
				if(x < 1.0) {
					n = snprintf(buf3, 6, "%0.2f", x);
				} else if(x < 100.0) {
					n = snprintf(buf3, 6, "%0.1f", x);
				} else {
					n = snprintf(buf3, 6, "%i", (int)(x+0.5));
				}
				_kln89->DrawText((string)buf3, 1, 5-n, 0);
				break;
			}
		}
	}
	
	KLN89Page::Update(dt);
}

// Returns the id string of the selected waypoint on NAV4 if valid, else returns an empty string.
string KLN89NavPage::GetNav4WpId() {
	if(3 == _subPage) {
		if(fgGetBool("/instrumentation/kln89/scan-pull")) {
			if(_kln89->_activeFP->waypoints.size()) {
				if(!_scanWpSet) {
					return(_kln89->_activeWaypoint.id);
				} else {
					return(_kln89->_activeFP->waypoints[_scanWpIndex]->id);
				}		
			}
		}
	}
	return("");
}

void KLN89NavPage::LooseFocus() {
	_suspendAVS = false;
	_scanWpSet = false;
}

void KLN89NavPage::CrsrPressed() {
	if(_kln89->_mode == KLN89_MODE_DISP) {
		// Crsr just switched off
		_menuActive = false;
	} else {
		// Crsr just switched on
		if(_subPage < 3) {
			_uLinePos = 1;
		} else {
			_uLinePos = 3;
		}
	}
}

void KLN89NavPage::EntPressed() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_subPage == 3 && _uLinePos == 2 && !_menuActive) {
			_menuActive = true;
			_menuPos = 0;
			_suspendAVS = false;
		}
	}
}

void KLN89NavPage::ClrPressed() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_subPage == 0) {
			if(_uLinePos == 1) {
				_cdiFormat++;
				if(_cdiFormat > 2) _cdiFormat = 0;
			} else if(_uLinePos == 2) {
				_vnv++;
				if(_vnv > 2) _vnv = 0;
			}
		}
		if(_subPage == 3) {
			if(_uLinePos > 1) {
				_suspendAVS = !_suspendAVS;
				_menuActive = false;
			} else if(_uLinePos == 1) {
				_nav4DataSnippet++;
				if(_nav4DataSnippet > 3) _nav4DataSnippet = 0;
			}
		}
	} else {
		if(_subPage == 3) {
			_suspendAVS = !_suspendAVS;
		}
	}
}

void KLN89NavPage::Knob1Left1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(!(_subPage == 3 && _menuActive)) {
			if(_uLinePos > 0) _uLinePos--;
		} else {
			if(_menuPos > 0) _menuPos--;
		}
	}
}

void KLN89NavPage::Knob1Right1() {
	if(_kln89->_mode == KLN89_MODE_CRSR) {
		if(_subPage < 2) {
			if(_uLinePos < 2) _uLinePos++;
		} else if(_subPage == 2) {
			_uLinePos = 1;
		} else {
			// NAV 4 - this is complicated by whether the menu is displayed or not.
			if(_menuActive) {
				if(_menuPos < 3) _menuPos++;
			} else {
				if(_uLinePos < 3) _uLinePos++;
			}
		}
	}
}

void KLN89NavPage::Knob2Left1() {
	// If the inner-knob is out on the nav4 page, the only effect is to cycle the displayed waypoint.
	if(3 == _subPage && fgGetBool("/instrumentation/kln89/scan-pull")) {
		if(_kln89->_activeFP->waypoints.size()) {	// TODO - find out what happens when scan-pull is on on nav4 without an active FP.
			// It's unlikely that we could get here without _scanWpSet, but theoretically possible, so we need to cover it.
			if(!_scanWpSet) {
				_scanWpIndex = _kln89->GetActiveWaypointIndex();
				_scanWpSet = true;
			} else {
				if(0 == _scanWpIndex) {
					_scanWpIndex = _kln89->_activeFP->waypoints.size() - 1;
				} else {
					_scanWpIndex--;
				}
			}
		}
		return;
	}
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Left1();
		return;
	}
	if(_subPage == 0) {
		if(_uLinePos == 1 && _cdiFormat == 2) {
			_kln89->CDIFSDIncrease();
		}
	} else if(_subPage == 3) {
		if(_menuActive) {
			if(_menuPos == 0) {
				_kln89->_drawSUA = !_kln89->_drawSUA;
			} else if(_menuPos == 1) {
				_kln89->_drawVOR = !_kln89->_drawVOR;
			} else if(_menuPos == 2) {
				_kln89->_drawApt = !_kln89->_drawApt;
			} else {
				if(_kln89->_mapOrientation == 0) {
					// Don't allow heading up for now
					_kln89->_mapOrientation = 2;
				} else {
					_kln89->_mapOrientation--;
				}
				_kln89->UpdateMapHeading();
			}
		} else if(_uLinePos == 3) {
			// TODO - add AUTO
			if(_kln89->_mapScaleIndex == 0) {
				_kln89->_mapScaleIndex = 20;
			} else {
				_kln89->_mapScaleIndex--;
			}
		}
	}
}

void KLN89NavPage::Knob2Right1() {
	// If the inner-knob is out on the nav4 page, the only effect is to cycle the displayed waypoint.
	if(3 == _subPage && fgGetBool("/instrumentation/kln89/scan-pull")) {
		if(_kln89->_activeFP->waypoints.size()) {	// TODO - find out what happens when scan-pull is on on nav4 without an active FP.
			// It's unlikely that we could get here without _scanWpSet, but theoretically possible, so we need to cover it.
			if(!_scanWpSet) {
				_scanWpIndex = _kln89->GetActiveWaypointIndex();
				_scanWpSet = true;
			} else {
				_scanWpIndex++;
				if(_scanWpIndex > static_cast<int>(_kln89->_activeFP->waypoints.size()) - 1) {
					_scanWpIndex = 0;
				}
			}
		}
		return;
	}
	if(_kln89->_mode != KLN89_MODE_CRSR || _uLinePos == 0) {
		KLN89Page::Knob2Right1();
		return;
	}
	if(_subPage == 0) {
		if(_uLinePos == 1 && _cdiFormat == 2) {
			_kln89->CDIFSDDecrease();
		}
	} else if(_subPage == 3) {
		if(_menuActive) {
			if(_menuPos == 0) {
				_kln89->_drawSUA = !_kln89->_drawSUA;
			} else if(_menuPos == 1) {
				_kln89->_drawVOR = !_kln89->_drawVOR;
			} else if(_menuPos == 2) {
				_kln89->_drawApt = !_kln89->_drawApt;
			} else {
				if(_kln89->_mapOrientation >= 2) {
					// Don't allow heading up for now
					_kln89->_mapOrientation = 0;
				} else {
					_kln89->_mapOrientation++;
				}
				_kln89->UpdateMapHeading();
			}
		} else if(_uLinePos == 3) {
			// TODO - add AUTO
			if(_kln89->_mapScaleIndex == 20) {
				_kln89->_mapScaleIndex = 0;
			} else {
				_kln89->_mapScaleIndex++;
			}
		}
	}
}
