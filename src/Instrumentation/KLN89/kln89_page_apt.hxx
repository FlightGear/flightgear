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

#ifndef _KLN89_PAGE_APT
#define _KLN89_PAGE_APT

#include "kln89.hxx"

class FGRunway;

struct AptFreq {
	std::string service;
	unsigned short int freq;
};

class KLN89AptPage : public KLN89Page {

public:
	KLN89AptPage(KLN89* parent);
	~KLN89AptPage();
	
	void Update(double dt);

	void CrsrPressed();
	void ClrPressed();
	void EntPressed();
	void Knob1Left1();
	void Knob1Right1();
	void Knob2Left1();
	void Knob2Right1();
	
	void SetId(const std::string& s);
	
private:
	// Update the cached airport details
	void UpdateAirport(const std::string& id);

	std::string _apt_id;
	std::string _last_apt_id;
	std::string _save_apt_id;
	const FGAirport* ap;
	
	vector<FGRunway*> _aptRwys;
	vector<AptFreq> _aptFreqs;
	
	iap_list_type _iaps;
	unsigned int _curIap;	// The index into _iaps of the IAP we are currently selecting
	vector<GPSFlightPlan*> _approachRoutes;	// The approach route(s) from the IAF(s) to the IF.
	vector<GPSWaypoint*> _IAP;	// The compulsory waypoints of the approach procedure (may duplicate one of the above).
								// _IAP includes the FAF and MAF.
	vector<GPSWaypoint*> _MAP;	// The missed approach procedure (doesn't include the MAF).
	unsigned int _curIaf;	// The index into _approachRoutes of the IAF we are currently selecting, and then remembered as the one we selected
	
	// Position in rwy pages
	unsigned int _curRwyPage;
	unsigned int _nRwyPages;
	
	// Position in freq pages
	unsigned int _curFreqPage;
	unsigned int _nFreqPages;
	
	// Position in IAP list (0-based number of first IAP displayed)
	unsigned int _iapStart;
	// ditto for IAF list (can't test this since can't find an approach with > 3 IAF at the moment!)
	unsigned int _iafStart;
	// ditto for list of approach fixes when asking load confirmation
	unsigned int _fStart;
	
	// Various IAP related dialog states that we might need to remember
	bool _iafDialog;
	bool _addDialog;
	bool _replaceDialog;
};

#endif  // _KLN89_PAGE_APT
