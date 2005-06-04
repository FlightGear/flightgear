// FGAIStatic - AIBase derived class creates AI static object
//
// Written by David Culp, started Jun 2005.
//
// Copyright (C) 2005  David P. Culp - davidculp2@comcast.net
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

#ifndef _FG_AIStatic_HXX
#define _FG_AIStatic_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"

#include <string>
SG_USING_STD(string);


class FGAIStatic : public FGAIBase {

public:

	FGAIStatic(FGAIManager* mgr);
	~FGAIStatic();
	
	bool init();
        virtual void bind();
        virtual void unbind();
	void update(double dt);

private:

        double dt; 
	
};



#endif  // _FG_AISTATIC_HXX
