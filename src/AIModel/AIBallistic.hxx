// FGAIBallistic - AIBase derived class creates an AI ballistic object
//
// Written by David Culp, started November 2003.
// - davidculp2@comcast.net
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

#ifndef _FG_AIBALLISTIC_HXX
#define _FG_AIBALLISTIC_HXX

#include "AIManager.hxx"
#include "AIBase.hxx"


class FGAIBallistic : public FGAIBase {
	
public:
	
    FGAIBallistic(FGAIManager* mgr);
    ~FGAIBallistic();
	
    bool init();
    virtual void bind();
    virtual void unbind();
    void update(double dt);

    void setAzimuth( double az );
    void setElevation( double el );
    void setStabilization( bool val );
    void setDragArea( double a );
    void setLife( double seconds );
    void setBuoyancy( double fps2 );
	
private:

    double azimuth;         // degrees true
    double elevation;       // degrees
    double hs;              // horizontal speed (fps)
    bool aero_stabilized;   // if true, object will point where it's going
    double drag_area;       // equivalent drag area in ft2
    double life_timer;      // seconds
    double gravity;         // fps2
    double buoyancy;        // fps2
		
    void Run(double dt);
};

#endif  // _FG_AIBALLISTIC_HXX
