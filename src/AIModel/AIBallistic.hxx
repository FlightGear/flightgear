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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_AIBALLISTIC_HXX
#define _FG_AIBALLISTIC_HXX

#include <math.h>
#include <vector>
#include <simgear/structure/SGSharedPtr.hxx>


#include "AIManager.hxx"
#include "AIBase.hxx"

SG_USING_STD(vector);
SG_USING_STD(list);

class FGAIBallistic : public FGAIBase {

public:

    FGAIBallistic();
    ~FGAIBallistic();

    void readFromScenario(SGPropertyNode* scFileNode);

    bool init(bool search_in_AI_path=false);
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
    void setNoRoll( bool nr );
    void setRandom( bool r );
    void setName(const string&);
    void setCollision(bool c);
    void setImpact(bool i);
    void setImpactReportNode(const string&);
    void setFuseRange(double f);
    void setSMPath(const string&);
    void setSubID(int i);
    void setSubmodel(const string&);


    double _getTime() const;

    virtual const char* getTypeString(void) const { return "ballistic"; }
    static const double slugs_to_kgs; //conversion factor

private:

    double _azimuth;         // degrees true
    double _elevation;       // degrees
    double _rotation;        // degrees
    bool   _aero_stabilised; // if true, object will align with trajectory
    double _drag_area;       // equivalent drag area in ft2
    double _life_timer;      // seconds
    double _gravity;         // fps2
    double _buoyancy;        // fps2
    double _wind_from_east;  // fps
    double _wind_from_north; // fps
    bool   _wind;            // if true, local wind will be applied to object
    double _Cd;              // drag coefficient
    double _mass;            // slugs
    bool   _random;          // modifier for Cd
    double _ht_agl_ft;       // height above ground level
    double _load_resistance; // ground load resistanc N/m^2
    bool   _solid;           // if true ground is solid for FDMs

    bool   _report_collision;       // if true a collision point with AI Objects is calculated
    bool   _report_impact;          // if true an impact point on the terrain is calculated

    SGPropertyNode_ptr _impact_report_node;  // report node for impact and collision

    double _fuse_range;

    double _dt_count;
    double _next_run;

    string _mat_name;
    string _name;
    string _path;
    string _submodel;

    void Run(double dt);
    void handle_collision();
    void handle_impact();

    void report_impact(double elevation, const FGAIBase *target = 0);

};

#endif  // _FG_AIBALLISTIC_HXX
