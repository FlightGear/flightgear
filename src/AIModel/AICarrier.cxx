// FGAICarrier - FGAIShip-derived class creates an AI aircraft carrier
//
// Written by David Culp, started October 2004.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>
#include <string>
#include <vector>

#include <osg/Geode>
#include <osg/Drawable>
#include <osg/Transform>
#include <osg/NodeVisitor>
#include <osg/TemplatePrimitiveFunctor>

#include <simgear/sg_inlines.h>
#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/bvh/BVHGroup.hxx>
#include <simgear/scene/bvh/BVHLineGeometry.hxx>

#include <math.h>
#include <Main/util.hxx>
#include <Main/viewer.hxx>

#include "AICarrier.hxx"

/// Hmm: move that kind of configuration into the model file???
class LineCollector : public osg::NodeVisitor {
    struct LinePrimitiveFunctor {
        LinePrimitiveFunctor() : _lineCollector(0)
        { }
        void operator() (const osg::Vec3&, bool)
        { }
        void operator() (const osg::Vec3& v1, const osg::Vec3& v2, bool)
        { if (_lineCollector) _lineCollector->addLine(v1, v2); }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&,
                         bool)
        { }
        void operator() (const osg::Vec3&, const osg::Vec3&, const osg::Vec3&,
                         const osg::Vec3&, bool)
        { }
        LineCollector* _lineCollector;
    };
    
public:
    LineCollector() :
        osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                         osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    { }
    virtual void apply(osg::Geode& geode)
    {
        osg::TemplatePrimitiveFunctor<LinePrimitiveFunctor> pf;
        pf._lineCollector = this;
        for (unsigned i = 0; i < geode.getNumDrawables(); ++i) {
            geode.getDrawable(i)->accept(pf);
        }
    }
    virtual void apply(osg::Node& node)
    {
        traverse(node);
    }
    virtual void apply(osg::Transform& transform)
    {
        osg::Matrix matrix = _matrix;
        if (transform.computeLocalToWorldMatrix(_matrix, this))
            traverse(transform);
        _matrix = matrix;
    }
    
    const std::vector<SGLineSegmentf>& getLineSegments() const
    { return _lineSegments; }
    
    void addLine(const osg::Vec3& v1, const osg::Vec3& v2)
    {
        // Trick to get the ends in the right order.
        // Use the x axis in the original coordinate system. Choose the
        // most negative x-axis as the one pointing forward
        SGVec3f tv1(_matrix.preMult(v1));
        SGVec3f tv2(_matrix.preMult(v2));
        if (tv1[0] > tv2[0])
            _lineSegments.push_back(SGLineSegmentf(tv1, tv2));
        else
            _lineSegments.push_back(SGLineSegmentf(tv2, tv1));
    }

    void addBVHElements(osg::Node& node, simgear::BVHLineGeometry::Type type)
    {
        if (_lineSegments.empty())
            return;

        SGSceneUserData* userData;
        userData = SGSceneUserData::getOrCreateSceneUserData(&node);

        simgear::BVHNode* bvNode = userData->getBVHNode();
        if (!bvNode && _lineSegments.size() == 1) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments.front(), type);
            userData->setBVHNode(bvLine);
            return;
        }

        simgear::BVHGroup* group = new simgear::BVHGroup;
        if (bvNode)
            group->addChild(bvNode);

        for (unsigned i = 0; i < _lineSegments.size(); ++i) {
            simgear::BVHLineGeometry* bvLine;
            bvLine = new simgear::BVHLineGeometry(_lineSegments[i], type);
            group->addChild(bvLine);
        }
        userData->setBVHNode(group);
    }
    
private:
    osg::Matrix _matrix;
    std::vector<SGLineSegmentf> _lineSegments;
};

class FGCarrierVisitor : public osg::NodeVisitor {
public:
    FGCarrierVisitor(FGAICarrier* carrier,
                     const std::list<std::string>& wireObjects,
                     const std::list<std::string>& catapultObjects) :
        osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                         osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
        mWireObjects(wireObjects),
        mCatapultObjects(catapultObjects)
    { }
    virtual void apply(osg::Node& node)
    {
        if (std::find(mWireObjects.begin(), mWireObjects.end(), node.getName())
            != mWireObjects.end()) {
            LineCollector lineCollector;
            node.accept(lineCollector);
            simgear::BVHLineGeometry::Type type;
            type = simgear::BVHLineGeometry::CarrierWire;
            lineCollector.addBVHElements(node, type);
        }
        if (std::find(mCatapultObjects.begin(), mCatapultObjects.end(),
                      node.getName()) != mCatapultObjects.end()) {
            LineCollector lineCollector;
            node.accept(lineCollector);
            simgear::BVHLineGeometry::Type type;
            type = simgear::BVHLineGeometry::CarrierCatapult;
            lineCollector.addBVHElements(node, type);
        }
        
        traverse(node);
    }
    
private:
    std::list<std::string> mWireObjects;
    std::list<std::string> mCatapultObjects;
};

FGAICarrier::FGAICarrier() : FGAIShip(otCarrier) {
}

FGAICarrier::~FGAICarrier() {
}

void FGAICarrier::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIShip::readFromScenario(scFileNode);

  setRadius(scFileNode->getDoubleValue("turn-radius-ft", 2000));
  setSign(scFileNode->getStringValue("pennant-number"));
  setWind_from_east(scFileNode->getDoubleValue("wind_from_east", 0));
  setWind_from_north(scFileNode->getDoubleValue("wind_from_north", 0));
  setTACANChannelID(scFileNode->getStringValue("TACAN-channel-ID", "029Y"));
  setMaxLat(scFileNode->getDoubleValue("max-lat", 0));
  setMinLat(scFileNode->getDoubleValue("min-lat", 0));
  setMaxLong(scFileNode->getDoubleValue("max-long", 0));
  setMinLong(scFileNode->getDoubleValue("min-long", 0));

  SGPropertyNode* flols = scFileNode->getChild("flols-pos");
  if (flols) {
    // Transform to the right coordinate frame, configuration is done in
    // the usual x-back, y-right, z-up coordinates, computations
    // in the simulation usual body x-forward, y-right, z-down coordinates
    flols_off(0) = - flols->getDoubleValue("x-offset-m", 0);
    flols_off(1) = flols->getDoubleValue("y-offset-m", 0);
    flols_off(2) = - flols->getDoubleValue("z-offset-m", 0);
  } else
    flols_off = SGVec3d::zeros();

  std::vector<SGPropertyNode_ptr> props = scFileNode->getChildren("wire");
  std::vector<SGPropertyNode_ptr>::const_iterator it;
  for (it = props.begin(); it != props.end(); ++it) {
    std::string s = (*it)->getStringValue();
    if (!s.empty())
      wire_objects.push_back(s);
  }

  props = scFileNode->getChildren("catapult");
  for (it = props.begin(); it != props.end(); ++it) {
    std::string s = (*it)->getStringValue();
    if (!s.empty())
      catapult_objects.push_back(s);
  }

  props = scFileNode->getChildren("parking-pos");
  for (it = props.begin(); it != props.end(); ++it) {
    string name = (*it)->getStringValue("name", "unnamed");
    // Transform to the right coordinate frame, configuration is done in
    // the usual x-back, y-right, z-up coordinates, computations
    // in the simulation usual body x-forward, y-right, z-down coordinates
    double offset_x = -(*it)->getDoubleValue("x-offset-m", 0);
    double offset_y = (*it)->getDoubleValue("y-offset-m", 0);
    double offset_z = -(*it)->getDoubleValue("z-offset-m", 0);
    double hd = (*it)->getDoubleValue("heading-offset-deg", 0);
    ParkPosition pp(name, SGVec3d(offset_x, offset_y, offset_z), hd);
    ppositions.push_back(pp);
  }
}

void FGAICarrier::setWind_from_east(double fps) {
    wind_from_east = fps;
}

void FGAICarrier::setWind_from_north(double fps) {
    wind_from_north = fps;
}

void FGAICarrier::setMaxLat(double deg) {
    max_lat = fabs(deg);
}

void FGAICarrier::setMinLat(double deg) {
    min_lat = fabs(deg);
}

void FGAICarrier::setMaxLong(double deg) {
    max_long = fabs(deg);
}

void FGAICarrier::setMinLong(double deg) {
    min_long = fabs(deg);
}

void FGAICarrier::setSign(const string& s) {
    sign = s;
}

void FGAICarrier::setTACANChannelID(const string& id) {
    TACAN_channel_id = id;
}

void FGAICarrier::update(double dt) {
    // For computation of rotation speeds we just use finite differences here.
    // That is perfectly valid since this thing is not driven by accelerations
    // but by just apply discrete changes at its velocity variables.
    // Update the velocity information stored in those nodes.
    // Transform that one to the horizontal local coordinate system.
    SGQuatd ec2hl = SGQuatd::fromLonLat(pos);
    // The orientation of the carrier wrt the horizontal local frame
    SGQuatd hl2body = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    SGQuatd ec2body = ec2hl*hl2body;
    // The cartesian position of the carrier in the wgs84 world
    SGVec3d cartPos = SGVec3d::fromGeod(pos);

    // Compute the velocity in m/s in the body frame
    aip.setBodyLinearVelocity(SGVec3d(0.51444444*speed, 0, 0));

    // Now update the position and heading. This will compute new hdg and
    // roll values required for the rotation speed computation.
    FGAIShip::update(dt);


    //automatic turn into wind with a target wind of 25 kts otd
    if(turn_to_launch_hdg){
        TurnToLaunch();
    } else if(OutsideBox() || returning) {// check that the carrier is inside the operating box
        ReturnToBox();
    } else {
        TurnToBase();
    }

    // Only change these values if we are able to compute them safely
    if (SGLimits<double>::min() < dt) {
      // Now here is the finite difference ...

      // Transform that one to the horizontal local coordinate system.
      SGQuatd ec2hlNew = SGQuatd::fromLonLat(pos);
      // compute the new orientation
      SGQuatd hl2bodyNew = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
      // The rotation difference
      SGQuatd dOr = inverse(ec2body)*ec2hlNew*hl2bodyNew;
      SGVec3d dOrAngleAxis;
      dOr.getAngleAxis(dOrAngleAxis);
      // divided by the time difference provides a rotation speed vector
      dOrAngleAxis /= dt;

      aip.setBodyAngularVelocity(dOrAngleAxis);
    }

    UpdateWind(dt);
    UpdateElevator(dt, transition_time);
    UpdateJBD(dt, jbd_transition_time);
    // For the flols reuse some computations done above ...

    // The position of the eyepoint - at least near that ...
    SGVec3d eyePos(globals->get_current_view()->get_view_pos());
    // Add the position offset of the AIModel to gain the earth
    // centered position
    SGVec3d eyeWrtCarrier = eyePos - cartPos;
    // rotate the eyepoint wrt carrier vector into the carriers frame
    eyeWrtCarrier = ec2body.transform(eyeWrtCarrier);
    // the eyepoints vector wrt the flols position
    SGVec3d eyeWrtFlols = eyeWrtCarrier - flols_off;
    
    // the distance from the eyepoint to the flols
    dist = norm(eyeWrtFlols);
    
    // now the angle, positive angles are upwards
    if (fabs(dist) < SGLimits<float>::min()) {
      angle = 0;
    } else {
      double sAngle = -eyeWrtFlols(2)/dist;
      sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
      angle = SGMiscd::rad2deg(asin(sAngle));
    }
    
    // set the value of source
    if ( angle <= 4.35 && angle > 4.01 )
      source = 1;
    else if ( angle <= 4.01 && angle > 3.670 )
      source = 2;
    else if ( angle <= 3.670 && angle > 3.330 )
      source = 3;
    else if ( angle <= 3.330 && angle > 2.990 )
      source = 4;
    else if ( angle <= 2.990 && angle > 2.650 )
      source = 5;
    else if ( angle <= 2.650 )
      source = 6;
    else
      source = 0;
} //end update

bool FGAICarrier::init(bool search_in_AI_path) {
    if (!FGAIShip::init(search_in_AI_path))
        return false;

    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);

    _launchbar_state_node = fgGetNode("/gear/launchbar/state", true);

    _surface_wind_from_deg_node =
            fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg", true);
    _surface_wind_speed_node =
            fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt", true);


    turn_to_launch_hdg = false;
    returning = false;

    mOpBoxPos = pos;
    base_course = hdg;
    base_speed = speed;

    pos_norm = 0;
    elevators = false;
    transition_time = 150;
    time_constant = 0.005;
    jbd_pos_norm = raw_jbd_pos_norm = 0;
    jbd = false ;
    jbd_transition_time = 3;
    jbd_time_constant = 0.1;
    return true;
}

void FGAICarrier::initModel(osg::Node *node)
{
    // SG_LOG(SG_GENERAL, SG_BULK, "AICarrier::initModel()" );
    FGAIShip::initModel(node);
    // process the 3d model here
    // mark some objects solid, mark the wires ...
    FGCarrierVisitor carrierVisitor(this, wire_objects, catapult_objects);
    model->accept(carrierVisitor);
    model->setNodeMask(model->getNodeMask() | SG_NODEMASK_TERRAIN_BIT);
}

void FGAICarrier::bind() {
    FGAIShip::bind();

    props->untie("velocities/true-airspeed-kt");

    props->tie("controls/flols/source-lights",
                SGRawValuePointer<int>(&source));
    props->tie("controls/flols/distance-m",
                SGRawValuePointer<double>(&dist));
    props->tie("controls/flols/angle-degs",
                SGRawValuePointer<double>(&angle));
    props->tie("controls/turn-to-launch-hdg",
                SGRawValuePointer<bool>(&turn_to_launch_hdg));
    props->tie("controls/in-to-wind",
                SGRawValuePointer<bool>(&turn_to_launch_hdg));
    props->tie("controls/base-course-deg",
                SGRawValuePointer<double>(&base_course));
    props->tie("controls/base-speed-kts",
                SGRawValuePointer<double>(&base_speed));
    props->tie("controls/start-pos-lat-deg",
               SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLatitudeDeg));
    props->tie("controls/start-pos-long-deg",
               SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLongitudeDeg));
    props->tie("velocities/speed-kts",
                SGRawValuePointer<double>(&speed));
    props->tie("environment/surface-wind-speed-true-kts",
                SGRawValuePointer<double>(&wind_speed_kts));
    props->tie("environment/surface-wind-from-true-degs",
                SGRawValuePointer<double>(&wind_from_deg));
    props->tie("environment/rel-wind-from-degs",
                SGRawValuePointer<double>(&rel_wind_from_deg));
    props->tie("environment/rel-wind-from-carrier-hdg-degs",
                SGRawValuePointer<double>(&rel_wind));
    props->tie("environment/rel-wind-speed-kts",
                SGRawValuePointer<double>(&rel_wind_speed_kts));
    props->tie("controls/flols/wave-off-lights",
                SGRawValuePointer<bool>(&wave_off_lights));
    props->tie("controls/elevators",
                SGRawValuePointer<bool>(&elevators));
    props->tie("surface-positions/elevators-pos-norm",
                SGRawValuePointer<double>(&pos_norm));
    props->tie("controls/elevators-trans-time-s",
                SGRawValuePointer<double>(&transition_time));
    props->tie("controls/elevators-time-constant",
                SGRawValuePointer<double>(&time_constant));
    props->tie("controls/jbd",
        SGRawValuePointer<bool>(&jbd));
    props->tie("surface-positions/jbd-pos-norm",
        SGRawValuePointer<double>(&jbd_pos_norm));
    props->tie("controls/jbd-trans-time-s",
        SGRawValuePointer<double>(&jbd_transition_time));
    props->tie("controls/jbd-time-constant",
        SGRawValuePointer<double>(&jbd_time_constant));

    props->setBoolValue("controls/flols/cut-lights", false);
    props->setBoolValue("controls/flols/wave-off-lights", false);
    props->setBoolValue("controls/flols/cond-datum-lights", true);
    props->setBoolValue("controls/crew", false);
    props->setStringValue("navaids/tacan/channel-ID", TACAN_channel_id.c_str());
    props->setStringValue("sign", sign.c_str());
    props->setBoolValue("controls/lighting/deck-lights", false);
    props->setDoubleValue("controls/lighting/flood-lights-red-norm", 0);
}


void FGAICarrier::unbind() {
    FGAIShip::unbind();

    props->untie("velocities/true-airspeed-kt");
    props->untie("controls/flols/source-lights");
    props->untie("controls/flols/distance-m");
    props->untie("controls/flols/angle-degs");
    props->untie("controls/turn-to-launch-hdg");
    props->untie("velocities/speed-kts");
    props->untie("environment/wind-speed-true-kts");
    props->untie("environment/wind-from-true-degs");
    props->untie("environment/rel-wind-from-degs");
    props->untie("environment/rel-wind-speed-kts");
    props->untie("controls/flols/wave-off-lights");
    props->untie("controls/elevators");
    props->untie("surface-positions/elevators-pos-norm");
    props->untie("controls/elevators-trans-time-secs");
    props->untie("controls/elevators-time-constant");
    props->untie("controls/jbd");
    props->untie("surface-positions/jbd-pos-norm");
    props->untie("controls/jbd-trans-time-s");
    props->untie("controls/jbd-time-constant");

}


bool FGAICarrier::getParkPosition(const string& id, SGGeod& geodPos,
                                  double& hdng, SGVec3d& uvw)
{

    // FIXME: does not yet cover rotation speeds.
    list<ParkPosition>::iterator it = ppositions.begin();
    while (it != ppositions.end()) {
        // Take either the specified one or the first one ...
        if ((*it).name == id || id.empty()) {
            ParkPosition ppos = *it;
            SGVec3d cartPos = getCartPosAt(ppos.offset);
            geodPos = SGGeod::fromCart(cartPos);
            hdng = hdg + ppos.heading_deg;
            double shdng = sin(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
            double chdng = cos(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
            double speed_fps = speed*1.6878099;
            uvw = SGVec3d(chdng*speed_fps, shdng*speed_fps, 0);
            return true;
        }
        ++it;
    }

    return false;
}

// find relative wind
void FGAICarrier::UpdateWind( double dt) {

    //get the surface wind speed and direction
    wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
    wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();

    //calculate the surface wind speed north and east in kts
    double wind_speed_from_north_kts = cos( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;
    double wind_speed_from_east_kts  = sin( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;

    //calculate the carrier speed north and east in kts
    double speed_north_kts = cos( hdg / SGD_RADIANS_TO_DEGREES )* speed ;
    double speed_east_kts  = sin( hdg / SGD_RADIANS_TO_DEGREES )* speed ;

    //calculate the relative wind speed north and east in kts
    double rel_wind_speed_from_east_kts = wind_speed_from_east_kts + speed_east_kts;
    double rel_wind_speed_from_north_kts = wind_speed_from_north_kts + speed_north_kts;

    //combine relative speeds north and east to get relative windspeed in kts
    rel_wind_speed_kts = sqrt((rel_wind_speed_from_east_kts * rel_wind_speed_from_east_kts)
    + (rel_wind_speed_from_north_kts * rel_wind_speed_from_north_kts));

    //calculate the relative wind direction
    rel_wind_from_deg = atan2(rel_wind_speed_from_east_kts, rel_wind_speed_from_north_kts)
                            * SG_RADIANS_TO_DEGREES;

    //calculate rel wind
    rel_wind = rel_wind_from_deg - hdg;
    SG_NORMALIZE_RANGE(rel_wind, -180.0, 180.0);

    //switch the wave-off lights
    if (InToWind())
       wave_off_lights = false;
    else
       wave_off_lights = true;

    // cout << "rel wind: " << rel_wind << endl;

}// end update wind


void FGAICarrier::TurnToLaunch(){

    //calculate tgt speed
    double tgt_speed = 25 - wind_speed_kts;
    if (tgt_speed < 10)
        tgt_speed = 10;

    //turn the carrier
    FGAIShip::TurnTo(wind_from_deg);
    FGAIShip::AccelTo(tgt_speed);

}


void FGAICarrier::TurnToBase(){

    //turn the carrier
    FGAIShip::TurnTo(base_course);
    FGAIShip::AccelTo(base_speed);

}


void FGAICarrier::ReturnToBox(){
    double course, distance, az2;

    //calculate the bearing and range of the initial position from the carrier
    geo_inverse_wgs_84(pos, mOpBoxPos, &course, &az2, &distance);

    distance *= SG_METER_TO_NM;

    //cout << "return course: " << course << " distance: " << distance << endl;
    //turn the carrier
    FGAIShip::TurnTo(course);
    FGAIShip::AccelTo(base_speed);

    if (distance >= 1)
        returning = true;
    else
        returning = false;

} //  end turn to base


bool FGAICarrier::OutsideBox() { //returns true if the carrier is outside operating box

    if ( max_lat == 0 && min_lat == 0 && max_long == 0 && min_long == 0) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "AICarrier: No Operating Box defined" );
        return false;
    }

    if (mOpBoxPos.getLatitudeDeg() >= 0) { //northern hemisphere
        if (pos.getLatitudeDeg() >= mOpBoxPos.getLatitudeDeg() + max_lat)
            return true;

        if (pos.getLatitudeDeg() <= mOpBoxPos.getLatitudeDeg() - min_lat)
            return true;

    } else {                  //southern hemisphere
        if (pos.getLatitudeDeg() <= mOpBoxPos.getLatitudeDeg() - max_lat)
            return true;

        if (pos.getLatitudeDeg() >= mOpBoxPos.getLatitudeDeg() + min_lat)
            return true;
    }

    if (mOpBoxPos.getLongitudeDeg() >=0) { //eastern hemisphere
        if (pos.getLongitudeDeg() >= mOpBoxPos.getLongitudeDeg() + max_long)
            return true;

        if (pos.getLongitudeDeg() <= mOpBoxPos.getLongitudeDeg() - min_long)
            return true;

    } else {                 //western hemisphere
        if (pos.getLongitudeDeg() <= mOpBoxPos.getLongitudeDeg() - max_long)
            return true;

        if (pos.getLongitudeDeg() >= mOpBoxPos.getLongitudeDeg() + min_long)
            return true;
    }

    SG_LOG(SG_GENERAL, SG_DEBUG, "AICarrier: Inside Operating Box" );
    return false;

} // end OutsideBox


bool FGAICarrier::InToWind() {
    if ( fabs(rel_wind) < 5 )
        return true;

    return false;
}


void FGAICarrier::UpdateElevator(double dt, double transition_time) {

    double step = 0;

    if ((elevators && pos_norm >= 1 ) || (!elevators && pos_norm <= 0 ))
        return;

    // move the elevators
    if ( elevators ) {
        step = dt/transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt/transition_time;
        if ( step < -1 )
            step = -1;
    }
    // assume a linear relationship
    raw_pos_norm += step;

    //low pass filter
    pos_norm = (raw_pos_norm * time_constant) + (pos_norm * (1 - time_constant));

    //sanitise the output
    if (raw_pos_norm >= 1) {
        raw_pos_norm = 1;
    } else if (raw_pos_norm <= 0) {
        raw_pos_norm = 0;
    }
    return;

} // end UpdateElevator

void FGAICarrier::UpdateJBD(double dt, double jbd_transition_time) {

    string launchbar_state = _launchbar_state_node->getStringValue();
    double step = 0;

    if (launchbar_state == "Engaged"){
        jbd = true;
    } else {
        jbd = false;
    }

    if (( jbd && jbd_pos_norm >= 1 ) || ( !jbd && jbd_pos_norm <= 0 )){
        return;
    }

    // move the jbds
    if ( jbd ) {
        step = dt/jbd_transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt/jbd_transition_time;
        if ( step < -1 )
            step = -1;
    }

    // assume a linear relationship
    raw_jbd_pos_norm += step;

    //low pass filter
    jbd_pos_norm = (raw_jbd_pos_norm * jbd_time_constant) + (jbd_pos_norm * (1 - jbd_time_constant));

    //sanitise the output
    if (jbd_pos_norm >= 1) {
        jbd_pos_norm = 1;
    } else if (jbd_pos_norm <= 0) {
        jbd_pos_norm = 0;
    }

    return;

} // end UpdateJBD
