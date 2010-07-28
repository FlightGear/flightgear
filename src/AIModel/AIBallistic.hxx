// FGAIBallistic.hxx - AIBase derived class creates an AI ballistic object
//
// Written by David Culp, started November 2003.
// - davidculp2@comcast.net
//
// With major additions by Vivian Meazza, Feb 2008
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
#include <simgear/scene/material/mat.hxx>


#include "AIManager.hxx"
#include "AIBase.hxx"

using std::vector;
using std::list;

class FGAIBallistic : public FGAIBase {

public:

    FGAIBallistic(object_type ot = otBallistic);
    ~FGAIBallistic();

    void readFromScenario(SGPropertyNode* scFileNode);

    bool init(bool search_in_AI_path=false);
    virtual void bind();
    virtual void unbind();

    void update(double dt);

    FGAIBallistic *ballistic;

    void Run(double dt);

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
    void setWeight( double w );
    void setNoRoll( bool nr );
    void setRandom( bool r );
	void setRandomness( double r );
    void setName(const string&);
    void setCollision(bool c);
	void setExpiry(bool e);
    void setImpact(bool i);
    void setImpactReportNode(const string&);
    void setContentsNode(const string&);
    void setFuseRange(double f);
    void setSMPath(const string&);
    void setSubID(int i);
    void setSubmodel(const string&);
    void setExternalForce( bool f );
    void setForcePath(const string&);
    void setForceStabilisation( bool val );
    void setGroundOffset(double g);
    void setLoadOffset(double l);
    void setSlaved(bool s);
    void setSlavedLoad(bool s);
    void setHitchPos();
    void setPch (double e, double dt, double c);
    void setHdg (double az, double dt, double c);
    void setBnk(double r, double dt, double c);
    void setHt(double h, double dt, double c);
    void setHitchVelocity(double dt);
    void setFormate(bool f);

    double _getTime() const;
    double getRelBrgHitchToUser() const;
    double getElevHitchToUser() const;
    double getLoadOffset() const;
    double getContents();

    SGVec3d getCartHitchPos() const;

    bool getHtAGL();
    bool getSlaved() const;
    bool getSlavedLoad() const;

    virtual const char* getTypeString(void) const { return "ballistic"; }
    static const double slugs_to_kgs; //conversion factor
    static const double slugs_to_lbs; //conversion factor

    SGPropertyNode_ptr _force_node;
    SGPropertyNode_ptr _force_azimuth_node;
    SGPropertyNode_ptr _force_elevation_node;

    SGGeod hitchpos;

    double _height;
    double _ht_agl_ft;       // height above ground level
    double _azimuth;         // degrees true
    double _elevation;       // degrees
    double _rotation;        // degrees

    bool   _formate_to_ac;

    void setTgtXOffset(double x);
    void setTgtYOffset(double y);
    void setTgtZOffset(double z);
    void setTgtOffsets(double dt, double c);

    double getTgtXOffset() const;
    double getTgtYOffset() const;
    double getTgtZOffset() const;

    double _tgt_x_offset;
    double _tgt_y_offset;
    double _tgt_z_offset;
    double _elapsed_time;

private:

    virtual void reinit() { init(); }

    bool   _aero_stabilised; // if true, object will align with trajectory
    double _drag_area;       // equivalent drag area in ft2
    double _life_timer;      // seconds
    double _gravity;         // fps^2
    double _buoyancy;        // fps^2
    double _wind_from_east;  // fps
    double _wind_from_north; // fps
    bool   _wind;            // if true, local wind will be applied to object
    double _Cd;              // drag coefficient
    double _mass;            // slugs
    bool   _random;          // modifier for Cd, life, az
	double _randomness;		 // dimension for _random
    double _load_resistance; // ground load resistanc N/m^2
    double _frictionFactor;  // dimensionless modifier for Coefficient of Friction
    bool   _solid;           // if true ground is solid for FDMs
    double _elevation_m;     // ground elevation in meters
    bool   _force_stabilised;// if true, object will align to external force
    bool   _slave_to_ac;     // if true, object will be slaved to the parent ac pos and orientation
    bool   _slave_load_to_ac;// if true, object will be slaved to the parent ac pos
    double _contents_lb;     // contents of the object
    double _weight_lb;       // weight of the object (no contents if appropriate) (lbs)

    bool   _report_collision;       // if true a collision point with AI Objects is calculated
    bool   _report_impact;          // if true an impact point on the terrain is calculated
    bool   _external_force;         // if true then apply external force
	bool   _report_expiry;

    SGPropertyNode_ptr _impact_report_node;  // report node for impact and collision
    SGPropertyNode_ptr _contents_node;  // report node for impact and collision

    double _fuse_range;
    double _distance;
    double _dt_count;
    double _next_run;

    string _name;
    string _path;
    string _submodel;
    string _force_path;

    const SGMaterial* _material;

    void handle_collision();
	void handle_expiry();
    void handle_impact();
    void report_impact(double elevation, const FGAIBase *target = 0);
    void slaveToAC(double dt);
    void setContents(double c);
    void formateToAC(double dt);

    SGVec3d getCartUserPos() const;

    double getDistanceLoadToHitch() const;
    double getElevLoadToHitch() const;
    double getBearingLoadToHitch() const;

    double getRecip(double az);
    double getMass() const;

    double hs;
    double _ground_offset;
    double _load_offset;
    double _old_height;

    SGVec3d _oldcarthitchPos;

    SGGeod oldhitchpos;

};

#endif  // _FG_AIBALLISTIC_HXX
