// FGAICarrier - AIShip-derived class creates an AI aircraft carrier
//
// Written by David Culp, started October 2004.
//
// Copyright (C) 2004  David P. Culp - davidculp2@comcast.net
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

#ifndef _FG_AICARRIER_HXX
#define _FG_AICARRIER_HXX

#include "AIShip.hxx"
class FGAIManager;

class FGAICarrier  : public FGAIShip {
	
public:
	
	FGAICarrier(FGAIManager* mgr);
	~FGAICarrier();
	
private:

	void update(double dt);
};

#endif  // _FG_AICARRIER_HXX
