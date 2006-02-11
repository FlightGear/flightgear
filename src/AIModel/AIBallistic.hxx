// FGAIBallistic.hxx - AIBase derived class creates an AI ballistic object
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

    FGAIBallistic();
    ~FGAIBallistic();

    void readFromScenario(SGPropertyNode* scFileNode);

    bool init();
    virtual void bind();
    virtual void unbind();
    void update(double dt);

    void setAzimuth( double az );
    void setElevation( double el );
    void setRoll( double rl );
    void setStabilisation( bool val );
    void setDragArea( double a );
    void setLife( double seconds );
    void setBuoyancy( double fpss );
    void setWind_from_east( double fps );
    void setWind_from_north( double fps );
    void setWind( bool val );
    void setCd( double c );
    void setMass( double m );

    double _getTime() const;

    virtual const char* getTypeString(void) const { return "ballistic"; }

private:

    double azimuth;         // degrees true
    double elevation;       // degrees
    double rotation;        // degrees
    double hs;              // horizontal speed (fps)
    bool aero_stabilised;   // if true, object will align wit trajectory
    double drag_area;       // equivalent drag area in ft2
    double life_timer;      // seconds
    double gravity;         // fps2
    double buoyancy;        // fps2
    double wind_from_east;  // fps
    double wind_from_north; // fps
    bool wind;              // if true, local wind will be applied to object
    double Cd;              // drag coefficient
    double mass;            // slugs

    void Run(double dt);
};

#endif  // _FG_AIBALLISTIC_HXX

