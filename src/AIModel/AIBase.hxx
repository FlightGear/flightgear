/*
 * SPDX-FileName: AIBase.hxx
 * SPDX-FileComment: abstract base class for AI objects, based on David Luff's FGAIEntity class.
 * SPDX-FileCopyrightText: Written by David Culp, started Nov 2003 - davidculp2@comcast.net
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <string>
#include <string_view>

#include <osg/ref_ptr>

#include <simgear/constants.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>


namespace osg {
class PagedLOD;
}
namespace simgear {
class BVHMaterial;
}

class FGAIManager;
class FGAIFlightPlan;
class FGFX;
class FGAIModelData; // defined below


class FGAIBase : public SGReferenced
{
public:
    enum class object_type {
        otNull = 0,
        otAircraft,
        otShip,
        otCarrier,
        otBallistic,
        otRocket,
        otStorm,
        otThermal,
        otStatic,
        otWingman,
        otGroundVehicle,
        otEscort,
        otMultiplayer,
        MAX_OBJECTS // Needs to be last!!!
    };

    FGAIBase(object_type ot, bool enableHot);
    virtual ~FGAIBase();

    virtual void readFromScenario(SGPropertyNode* scFileNode);

    enum class ModelSearchOrder {
        DATA_ONLY,  // don't search AI/ prefix at all
        PREFER_AI,  // search AI first, override other paths
        PREFER_DATA // search data first but fall back to AI
    };

    virtual bool init(ModelSearchOrder searchOrder);
    virtual void initModel();
    virtual void update(double dt);
    virtual void bind();
    virtual void unbind();
    virtual void reinit() {}

    // default model radius for LOD.
    virtual double getDefaultModelRadius() { return 20.0; }

    void updateLOD();
    void updateInterior();

    void setManager(FGAIManager* mgr, SGPropertyNode* p);

    void setPath(const char* model);
    void setPathLowres(std::string model);

    void setFallbackModelIndex(const int i);
    void setSMPath(const std::string& p);
    void setCallSign(const std::string&);

    void setSpeed(double speed_KTAS);
    void setMaxSpeed(double kts);

    void setAltitude(double altitude_ft);
    void setAltitudeAGL(double altitude_agl_ft);
    void setHeading(double heading);
    void setLatitude(double latitude);
    void setLongitude(double longitude);

    void setBank(double bank);
    void setPitch(double newpitch);
    void setRadius(double radius);

    void setXoffset(double x_offset);
    void setYoffset(double y_offset);
    void setZoffset(double z_offset);

    void setPitchoffset(double x_offset);
    void setRolloffset(double y_offset);
    void setYawoffset(double z_offset);

    void setServiceable(bool serviceable);

    bool getDie();
    void setDie(bool die);
    bool isValid() const;

    void setCollisionData(bool i, double lat, double lon, double elev);
    void setImpactData(bool d);
    void setImpactLat(double lat);
    void setImpactLon(double lon);
    void setImpactElev(double e);

    void setName(const std::string& n);
    bool setParentNode();
    void setParentName(const std::string& p);

    void setCollisionLength(int range);
    void setCollisionHeight(int height);

    void calcRangeBearing(double lat, double lon, double lat2, double lon2,
                          double& range, double& bearing) const;
    double calcRelBearingDeg(double bearing, double heading);
    double calcTrueBearingDeg(double bearing, double heading);
    double calcRecipBearingDeg(double bearing);

    int getID() const;
    int _getSubID() const;

    void setFlightPlan(std::unique_ptr<FGAIFlightPlan> f);

    SGGeod getGeodPos() const;
    void setGeodPos(const SGGeod& pos);
    SGVec3d getCartPosAt(const SGVec3d& off) const;
    SGVec3d getCartPos() const;

    bool getGroundElevationM(const SGGeod& pos, double& elev,
                             const simgear::BVHMaterial** material) const;

    SGPropertyNode* getPositionFromNode(SGPropertyNode* scFileNode, const std::string& key, SGVec3d& position);

    double getTrueHeadingDeg() const { return hdg; }

    double _getCartPosX() const;
    double _getCartPosY() const;
    double _getCartPosZ() const;

    osg::LOD* getSceneBranch() const;

    virtual int getCollisionHeight() const;
    virtual int getCollisionLength() const;

    /**
     *
     * @return true if at least one model (either low_res or high_res) is loaded
     */
    bool modelLoaded() const;

    void setScenarioPath(const std::string& scenarioPath);

protected:
    double _elevation_m = 0.0;

    double _x_offset;
    double _y_offset;
    double _z_offset;

    double _pitch_offset;
    double _roll_offset;
    double _yaw_offset;

    double _max_speed = 300.0;

    int collisionHeight = 0;
    int collisionLength = 0;

    std::string _path;
    std::string _callsign;
    std::string _submodel;
    std::string _name;
    std::string _parent;
    std::string _scenarioPath;

    /**
     * Tied-properties helper, record nodes which are tied for easy un-tie-ing
     */
    template <typename T>
    void tie(const char* aRelPath, const SGRawValue<T>& aRawValue)
    {
        _tiedProperties.Tie(props->getNode(aRelPath, true), aRawValue);
    }

    simgear::TiedPropertyList _tiedProperties;
    SGPropertyNode_ptr _selected_ac;
    SGPropertyNode_ptr props;
    SGPropertyNode_ptr trigger_node;
    SGPropertyNode_ptr replay_time;
    SGPropertyNode_ptr model_removed; // where to report model removal
    FGAIManager* manager = nullptr;

    // these describe the model's actual state
    SGGeod pos;             // WGS84 lat & lon in degrees, elev above sea-level in meters
    double hdg;             // True heading in degrees
    double roll;            // degrees, left is negative
    double pitch;           // degrees, nose-down is negative
    double speed;           // knots true airspeed
    double speed_fps = 0.0; // fps true airspeed
    double altitude_ft;     // feet above sea level
    double vs_fps;          // vertical speed
    double speed_north_deg_sec;
    double speed_east_deg_sec;
    double turn_radius_ft; // turn radius ft at 15 kts rudder angle 15 degrees
    double altitude_agl_ft = 0.0;

    double ft_per_deg_lon;
    double ft_per_deg_lat;

    // these describe the model's desired state
    double tgt_heading;     // target heading, degrees true
    double tgt_altitude_ft; // target altitude, *feet* above sea level
    double tgt_speed;       // target speed, KTAS
    double tgt_roll;
    double tgt_pitch;
    double tgt_yaw;
    double tgt_vs;

    // these describe radar information for the user
    bool in_range;                 // true if in range of the radar, otherwise false
    double bearing;                // true bearing from user to this model
    double elevation;              // elevation in degrees from user to this model
    double range;                  // range from user to this model, nm
    double rdot;                   // range rate, in knots
    double horiz_offset;           // look left/right from user to me, deg
    double vert_offset;            // look up/down from user to me, deg
    double x_shift;                // value used by radar display instrument
    double y_shift;                // value used by radar display instrument
    double rotation;               // value used by radar display instrument
    double ht_diff;                // value used by radar display instrument

    std::string model_path;        // Path to the 3D model
    std::string model_path_lowres; // Path to optional low res 3D model
    int _fallback_model_index = 0; // Index into /sim/multiplay/fallback-models[]
    SGModelPlacement aip;

    bool delete_me;
    bool invisible = false;
    bool no_roll;
    bool serviceable;
    bool _installed = false;
    int _subID = 0;

    double life;

    std::unique_ptr<FGAIFlightPlan> fp;

    bool _impact_reported;
    bool _collision_reported;
    bool _expiry_reported;

    double _impact_lat;
    double _impact_lon;
    double _impact_elev;
    double _impact_hdg;
    double _impact_pitch;
    double _impact_roll;
    double _impact_speed;

    ModelSearchOrder _searchOrder = ModelSearchOrder::DATA_ONLY;
    void Transform();
    double UpdateRadar(FGAIManager* manager);

    void removeModel();
    void removeSoundFx();

    static int _newAIModelID();

private:
    int _refID;
    object_type _otype;
    bool _initialized = false;
    osg::ref_ptr<osg::PagedLOD> _model;
    osg::ref_ptr<FGAIModelData> _modeldata;

    SGSharedPtr<FGFX> _fx;

    std::vector<std::string> resolveModelPath(ModelSearchOrder searchOrder);

public:
    object_type getType();

    virtual std::string_view getTypeString(void) const { return "null"; }

    bool isa(object_type otype);

    void _setVS_fps(double _vs);
    void _setAltitude(double _alt);
    void _setLongitude(double longitude);
    void _setLatitude(double latitude);
    void _setSubID(int s);

    double _getAltitudeAGL(SGGeod inpos, double start);

    double _getVS_fps() const;
    double _getAltitude() const;
    double _getLongitude() const;
    double _getLatitude() const;
    double _getElevationFt() const;
    double _getRdot() const;
    double _getH_offset() const;
    double _getV_offset() const;
    double _getX_shift() const;
    double _getY_shift() const;
    double _getRotation() const;
    double _getSpeed() const;
    double _getRoll() const;
    double _getPitch() const;
    double _getHeading() const;
    double _get_speed_east_fps() const;
    double _get_speed_north_fps() const;
    double _get_SubPath() const;
    double _getImpactLat() const;
    double _getImpactLon() const;
    double _getImpactElevFt() const;
    double _getImpactHdg() const;
    double _getImpactPitch() const;
    double _getImpactRoll() const;
    double _getImpactSpeed() const;
    double _getXOffset() const;
    double _getYOffset() const;
    double _getZOffset() const;

    bool _getServiceable() const;
    bool _getFirstTime() const;
    bool _getImpact();
    bool _getImpactData();
    bool _getCollisionData();
    bool _getExpiryData();

    SGPropertyNode* _getProps() const;

    const char* _getPath() const;
    const char* _getSMPath() const;
    const char* _getCallsign() const;
    const char* _getTriggerNode() const;
    const char* _getName() const;
    const char* _getSubmodel() const;
    int _getFallbackModelIndex() const;

    static const double e;
    static const double lbs_to_slugs;

    double _getRange() const { return range; }
    double _getBearing() const { return bearing; }

    static bool _isNight();

    const std::string& getCallSign() const { return _callsign; }
    ModelSearchOrder getSearchOrder() const { return _searchOrder; }
};

typedef SGSharedPtr<FGAIBase> FGAIBasePtr;

inline void FGAIBase::setManager(FGAIManager* mgr, SGPropertyNode* p)
{
    manager = mgr;
    props = p;
}

inline void FGAIBase::setPath(const char* model)
{
    model_path.append(model);
}

inline void FGAIBase::setPathLowres(std::string model)
{
    model_path_lowres.append(model);
}

inline void FGAIBase::setFallbackModelIndex(const int i)
{
    _fallback_model_index = i;
}

inline void FGAIBase::setSMPath(const std::string& p)
{
    _path = p;
}

inline void FGAIBase::setServiceable(bool s)
{
    serviceable = s;
}

inline void FGAIBase::setSpeed(double speed_KTAS)
{
    speed = tgt_speed = speed_KTAS;
}

inline void FGAIBase::setRadius(double radius)
{
    turn_radius_ft = radius;
}

inline void FGAIBase::setHeading(double heading)
{
    hdg = tgt_heading = heading;
}

inline void FGAIBase::setAltitude(double alt_ft)
{
    altitude_ft = tgt_altitude_ft = alt_ft;
    pos.setElevationFt(altitude_ft);
}

inline void FGAIBase::setAltitudeAGL(double alt_ft)
{
    altitude_agl_ft = alt_ft;
}

inline void FGAIBase::setBank(double bank)
{
    roll = tgt_roll = bank;
    no_roll = false;
}

inline void FGAIBase::setPitch(double newpitch)
{
    pitch = tgt_pitch = newpitch;
}

inline void FGAIBase::setLongitude(double longitude)
{
    pos.setLongitudeDeg(longitude);
}

inline void FGAIBase::setLatitude(double latitude)
{
    pos.setLatitudeDeg(latitude);
}

inline void FGAIBase::setCallSign(const std::string& s)
{
    _callsign = s;
}

inline void FGAIBase::setXoffset(double x)
{
    _x_offset = x;
}

inline void FGAIBase::setYoffset(double y)
{
    _y_offset = y;
}

inline void FGAIBase::setZoffset(double z)
{
    _z_offset = z;
}

inline void FGAIBase::setPitchoffset(double p)
{
    _pitch_offset = p;
}

inline void FGAIBase::setRolloffset(double r)
{
    _roll_offset = r;
}

inline void FGAIBase::setYawoffset(double y)
{
    _yaw_offset = y;
}

inline void FGAIBase::setParentName(const std::string& p)
{
    _parent = p;
}

inline void FGAIBase::setName(const std::string& n)
{
    _name = n;
}

inline void FGAIBase::setCollisionLength(int length)
{
    collisionLength = length;
}

inline void FGAIBase::setCollisionHeight(int height)
{
    collisionHeight = height;
}

inline void FGAIBase::setDie(bool die) { delete_me = die; }

inline bool FGAIBase::getDie() { return delete_me; }

inline FGAIBase::object_type FGAIBase::getType() { return _otype; }

inline void FGAIBase::calcRangeBearing(double lat, double lon, double lat2, double lon2,
                                       double& range, double& bearing) const
{
    // calculate the bearing and range of the second pos from the first
    double az2, distance;
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);
    range = distance * SG_METER_TO_NM;
}

inline double FGAIBase::calcRelBearingDeg(double bearing, double heading)
{
    double angle = bearing - heading;
    SG_NORMALIZE_RANGE(angle, -180.0, 180.0);
    return angle;
}

inline double FGAIBase::calcTrueBearingDeg(double bearing, double heading)
{
    double angle = bearing + heading;
    SG_NORMALIZE_RANGE(angle, 0.0, 360.0);
    return angle;
}

inline double FGAIBase::calcRecipBearingDeg(double bearing)
{
    double angle = bearing - 180;
    SG_NORMALIZE_RANGE(angle, 0.0, 360.0);
    return angle;
}

inline void FGAIBase::setMaxSpeed(double m)
{
    _max_speed = m;
}

/*
 * Default height and lengths for AI submodel collision detection.
 * The difference in height is used first and then the range must be within 
 * the value specifed in the length field. This effective chops the top and 
 * bottom off the circle - but does not take into account the orientation of the
 * AI model; so this algorithm is fast but fairly inaccurate.
 * 
 * Default values:
 * +---------------+-------------+------------+
 * | Type          | Height(m)   |  Length(m) |
 * +---------------+-------------+------------+
 * | Null          |      0      |        0   |
 * | Aircraft      |     50      |      100   |
 * | Ship          |    100      |      200   |
 * | Carrier       |    250      |      750   |
 * | Ballistic     |      0      |        0   |
 * | Rocket        |    100      |       50   |
 * | Storm         |      0      |        0   |
 * | Thermal       |      0      |        0   |
 * | Static        |     50      |      200   |
 * | Wingman       |     50      |      100   |
 * | GroundVehicle |     20      |       40   |
 * | Escort        |    100      |      200   |
 * | Multiplayer   |     50      |      100   |
 * +---------------+-------------+------------+
 */
const static double tgt_ht[] = {0, 50, 100, 250, 0, 100, 0, 0, 50, 50, 20, 100, 50};
const static double tgt_length[] = {0, 100, 200, 750, 0, 50, 0, 0, 200, 100, 40, 200, 100};

inline int FGAIBase::getCollisionHeight() const
{
    if (collisionHeight == 0)
        return tgt_ht[static_cast<int>(_otype)];

    return collisionHeight;
}
inline int FGAIBase::getCollisionLength() const
{
    if (collisionLength == 0)
        return tgt_length[static_cast<int>(_otype)];

    return collisionLength;
}
