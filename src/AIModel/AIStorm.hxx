// FGAIStorm - AIBase derived class creates an AI thunderstorm
//
// Written by David Culp, started Feb 2004.
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

#ifndef _FG_AIStorm_HXX
#define _FG_AIStorm_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"

#include <string>
SG_USING_STD(string);


class FGAIStorm : public FGAIBase {

public:

	FGAIStorm(FGAIManager* mgr);
	~FGAIStorm();
	
	bool init();
        virtual void bind();
        virtual void unbind();
	void update(double dt);

private:

        double dt; 
	void Run(double dt);

        double delay;   // average time (sec) between lightning flashes
        int subflashes; // number of subflashes per flash
        double random_delay;  // delay +/- random number
        double timer; 
        SGPropertyNode* flash_node; 
        int flashed;    // number of subflashes already done this flash
        bool flashing;  // true if currently flashing;
        int subflash_array[8];
        int subflash_index;
};



#endif  // _FG_AIStorm_HXX
