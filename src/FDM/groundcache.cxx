// groundcache.cxx -- carries a small subset of the scenegraph near the vehicle
//
// Written by Mathias Froehlich, started Nov 2004.
//
// Copyright (C) 2004  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <utility>

#include <osg/Drawable>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Camera>
#include <osg/Transform>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/CameraView>

#include <simgear/sg_inlines.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <simgear/scene/bvh/BVHNode.hxx>
#include <simgear/scene/bvh/BVHGroup.hxx>
#include <simgear/scene/bvh/BVHTransform.hxx>
#include <simgear/scene/bvh/BVHMotionTransform.hxx>
#include <simgear/scene/bvh/BVHLineGeometry.hxx>
#include <simgear/scene/bvh/BVHStaticGeometry.hxx>
#include <simgear/scene/bvh/BVHStaticData.hxx>
#include <simgear/scene/bvh/BVHStaticNode.hxx>
#include <simgear/scene/bvh/BVHStaticLeaf.hxx>
#include <simgear/scene/bvh/BVHStaticTriangle.hxx>
#include <simgear/scene/bvh/BVHStaticBinary.hxx>
#include <simgear/scene/bvh/BVHSubTreeCollector.hxx>
#include <simgear/scene/bvh/BVHLineSegmentVisitor.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>

#include "flight.hxx"
#include "groundcache.hxx"

using namespace simgear;

static FGInterface::GroundType
materialToGroundType(const SGMaterial* material)
{
    if (!material)
        return FGInterface::Solid;
    if (material->get_solid())
        return FGInterface::Solid;
    return FGInterface::Water;
}

class FGGroundCache::CacheFill : public osg::NodeVisitor {
public:
    CacheFill(const SGVec3d& center, const double& radius,
              const double& startTime, const double& endTime) :
        osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN),
        _center(center),
        _radius(radius),
        _startTime(startTime),
        _endTime(endTime)
    {
        setTraversalMask(SG_NODEMASK_TERRAIN_BIT);
    }
    virtual void apply(osg::Node& node)
    {
        if (!testBoundingSphere(node.getBound()))
            return;

        addBoundingVolume(node);
    }
    
    virtual void apply(osg::Group& group)
    {
        if (!testBoundingSphere(group.getBound()))
            return;

        simgear::BVHSubTreeCollector::NodeList parentNodeList;
        mSubTreeCollector.pushNodeList(parentNodeList);
        
        traverse(group);
        addBoundingVolume(group);
        
        mSubTreeCollector.popNodeList(parentNodeList);
    }
    
    virtual void apply(osg::Transform& transform)
    { handleTransform(transform); }
    virtual void apply(osg::Camera& camera)
    {
        if (camera.getRenderOrder() != osg::Camera::NESTED_RENDER)
            return;
        handleTransform(camera);
    }
    virtual void apply(osg::CameraView& transform)
    { handleTransform(transform); }
    virtual void apply(osg::MatrixTransform& transform)
    { handleTransform(transform); }
    virtual void apply(osg::PositionAttitudeTransform& transform)
    { handleTransform(transform); }
        
    void handleTransform(osg::Transform& transform)
    {
        // Hmm, may be this needs to be refined somehow ...
        if (transform.getReferenceFrame() != osg::Transform::RELATIVE_RF)
            return;

        if (!testBoundingSphere(transform.getBound()))
            return;

        osg::Matrix inverseMatrix;
        if (!transform.computeWorldToLocalMatrix(inverseMatrix, this))
            return;
        osg::Matrix matrix;
        if (!transform.computeLocalToWorldMatrix(matrix, this))
            return;

        // Look for a velocity note
        const SGSceneUserData::Velocity* velocity = getVelocity(transform);
        // ... no velocity of there is only zero velocity
        if (velocity && velocity->linear == SGVec3d::zeros() &&
            velocity->angular == SGVec3d::zeros())
            velocity = 0;
        
        SGVec3d center = _center;
        _center = SGVec3d(inverseMatrix.preMult(_center.osg()));
        double radius = _radius;
        if (velocity)
            _radius += (_endTime - _startTime)*norm(velocity->linear);
        
        simgear::BVHSubTreeCollector::NodeList parentNodeList;
        mSubTreeCollector.pushNodeList(parentNodeList);

        addBoundingVolume(transform);
        traverse(transform);

        if (mSubTreeCollector.haveChildren()) {
            if (velocity) {
                simgear::BVHMotionTransform* bvhTransform;
                bvhTransform = new simgear::BVHMotionTransform;
                bvhTransform->setToWorldTransform(SGMatrixd(matrix.ptr()));
                bvhTransform->setLinearVelocity(velocity->linear);
                bvhTransform->setAngularVelocity(velocity->angular);
                bvhTransform->setReferenceTime(_startTime);
                bvhTransform->setEndTime(_endTime);

                mSubTreeCollector.popNodeList(parentNodeList, bvhTransform);
            } else {
                simgear::BVHTransform* bvhTransform;
                bvhTransform = new simgear::BVHTransform;
                bvhTransform->setToWorldTransform(SGMatrixd(matrix.ptr()));

                mSubTreeCollector.popNodeList(parentNodeList, bvhTransform);
            }
        } else {
            mSubTreeCollector.popNodeList(parentNodeList);
        }
        _center = center;
        _radius = radius;
    }

    const SGSceneUserData::Velocity* getVelocity(osg::Node& node)
    {
        SGSceneUserData* userData = SGSceneUserData::getSceneUserData(&node);
        if (!userData)
            return 0;
        return userData->getVelocity();
    }
    simgear::BVHNode* getNodeBoundingVolume(osg::Node& node)
    {
        SGSceneUserData* userData = SGSceneUserData::getSceneUserData(&node);
        if (!userData)
            return 0;
        return userData->getBVHNode();
    }
    void addBoundingVolume(osg::Node& node)
    {
        simgear::BVHNode* bvNode = getNodeBoundingVolume(node);
        if (!bvNode)
            return;

        // Get that part of the local bv tree that intersects our sphere
        // of interrest.
        mSubTreeCollector.setSphere(SGSphered(_center, _radius));
        bvNode->accept(mSubTreeCollector);
    }
    
    bool testBoundingSphere(const osg::BoundingSphere& bound) const
    {
        if (!bound.valid())
            return false;

        double maxDist = bound._radius + _radius;
        return distSqr(SGVec3d(bound._center), _center) <= maxDist*maxDist;
    }
    
    SGSharedPtr<simgear::BVHNode> getBVHNode() const
    { return mSubTreeCollector.getNode(); }
    
private:
    
    SGVec3d _center;
    double _radius;
    double _startTime;
    double _endTime;

    simgear::BVHSubTreeCollector mSubTreeCollector;
};

FGGroundCache::FGGroundCache() :
    _altitude(0),
    _type(0),
    _material(0),
    cache_ref_time(0),
    _wire(0),
    reference_wgs84_point(SGVec3d(0, 0, 0)),
    reference_vehicle_radius(0),
    down(0.0, 0.0, 0.0),
    found_ground(false)
{
}

FGGroundCache::~FGGroundCache()
{
}

bool
FGGroundCache::prepare_ground_cache(double ref_time, const SGVec3d& pt,
                                    double rad)
{
    // Empty cache.
    found_ground = false;

    SGGeod geodPt = SGGeod::fromCart(pt);
    // Don't blow away the cache ground_radius and stuff if there's no
    // scenery
    if (!globals->get_tile_mgr()->scenery_available(geodPt.getLatitudeDeg(),
                                                    geodPt.getLongitudeDeg(),
                                                    rad))
        return false;
    _altitude = 0;

    // If we have an active wire, get some more area into the groundcache
    if (_wire)
        rad = SGMiscd::max(200, rad);
    
    // Store the parameters we used to build up that cache.
    reference_wgs84_point = pt;
    reference_vehicle_radius = rad;
    // Store the time reference used to compute movements of moving triangles.
    cache_ref_time = ref_time;
    
    // Get a normalized down vector valid for the whole cache
    SGQuatd hlToEc = SGQuatd::fromLonLat(geodPt);
    down = hlToEc.rotate(SGVec3d(0, 0, 1));
    
    // Get the ground cache, that is a local collision tree of the environment
    double endTime = cache_ref_time + 1; //FIXME??
    CacheFill subtreeCollector(pt, rad, cache_ref_time, endTime);
    globals->get_scenery()->get_scene_graph()->accept(subtreeCollector);
    _localBvhTree = subtreeCollector.getBVHNode();

    // Try to get a croase altitude value for the ground cache
    SGLineSegmentd line(pt, pt + 1e4*down);
    simgear::BVHLineSegmentVisitor lineSegmentVisitor(line, ref_time);
    if (_localBvhTree)
        _localBvhTree->accept(lineSegmentVisitor);

    // If this is successful, store this altitude for croase altitude values
    if (!lineSegmentVisitor.empty()) {
        SGGeod geodPt = SGGeod::fromCart(lineSegmentVisitor.getPoint());
        _altitude = geodPt.getElevationM();
        _material = lineSegmentVisitor.getMaterial();
        _type = materialToGroundType(_material);
        found_ground = true;
    } else {
        // Else do a crude scene query for the current point
        found_ground = globals->get_scenery()->
            get_cart_elevation_m(pt, rad, _altitude, &_material);
        _type = materialToGroundType(_material);
    }
    
    // Still not sucessful??
    if (!found_ground)
        SG_LOG(SG_FLIGHT, SG_WARN, "prepare_ground_cache(): trying to build "
               "cache without any scenery below the aircraft");

    return found_ground;
}

bool
FGGroundCache::is_valid(double& ref_time, SGVec3d& pt, double& rad)
{
    pt = reference_wgs84_point;
    rad = reference_vehicle_radius;
    ref_time = cache_ref_time;
    return found_ground;
}

class FGGroundCache::CatapultFinder : public BVHVisitor {
public:
    CatapultFinder(const SGSphered& sphere, const double& t) :
        _sphere(sphere),
        _time(t),
        _haveLineSegment(false)
    { }
    
    virtual void apply(BVHGroup& leaf)
    {
        if (!intersects(_sphere, leaf.getBoundingSphere()))
            return;
        leaf.traverse(*this);
    }
    virtual void apply(BVHTransform& transform)
    {
        if (!intersects(_sphere, transform.getBoundingSphere()))
            return;
        
        SGSphered sphere = _sphere;
        _sphere = transform.sphereToLocal(sphere);
        bool haveLineSegment = _haveLineSegment;
        _haveLineSegment = false;
        
        transform.traverse(*this);
        
        if (_haveLineSegment) {
            _lineSegment = transform.lineSegmentToWorld(_lineSegment);
            _linearVelocity = transform.vecToWorld(_linearVelocity);
            _angularVelocity = transform.vecToWorld(_angularVelocity);
        }
        _haveLineSegment |= haveLineSegment;
        _sphere.setCenter(sphere.getCenter());
    }
    virtual void apply(BVHMotionTransform& transform)
    {
        if (!intersects(_sphere, transform.getBoundingSphere()))
            return;
        
        SGSphered sphere = _sphere;
        _sphere = transform.sphereToLocal(sphere, _time);
        bool haveLineSegment = _haveLineSegment;
        _haveLineSegment = false;
        
        transform.traverse(*this);
        
        if (_haveLineSegment) {
            SGMatrixd toWorld = transform.getToWorldTransform(_time);
            _linearVelocity
                += transform.getLinearVelocityAt(_lineSegment.getStart());
            _angularVelocity += transform.getAngularVelocity();
            _linearVelocity = toWorld.xformVec(_linearVelocity);
            _angularVelocity = toWorld.xformVec(_angularVelocity);
            _lineSegment = _lineSegment.transform(toWorld);
        }
        _haveLineSegment |= haveLineSegment;
        _sphere.setCenter(sphere.getCenter());
    }
    virtual void apply(BVHLineGeometry& node)
    {
        if (node.getType() != BVHLineGeometry::CarrierCatapult)
            return;

        SGLineSegmentd lineSegment(node.getLineSegment());
        if (!intersects(_sphere, lineSegment))
            return;

        _lineSegment = lineSegment;
        double dist = distSqr(lineSegment, getSphere().getCenter());
        _sphere.setRadius(sqrt(dist));
        _linearVelocity = SGVec3d::zeros();
        _angularVelocity = SGVec3d::zeros();
        _haveLineSegment = true;
    }
    virtual void apply(BVHStaticGeometry& node)
    { }
    
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticLeaf&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&) { }
    
    void setSphere(const SGSphered& sphere)
    { _sphere = sphere; }
    const SGSphered& getSphere() const
    { return _sphere; }
    
    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }
    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }
    
    bool getHaveLineSegment() const
    { return _haveLineSegment; }
    
protected:
    SGLineSegmentd _lineSegment;
    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;
    
    bool _haveLineSegment;

    SGSphered _sphere;
    double _time;
};

double
FGGroundCache::get_cat(double t, const SGVec3d& pt,
                       SGVec3d end[2], SGVec3d vel[2])
{
    double maxDistance = 1000;

    // Get the wire in question
    CatapultFinder catapultFinder(SGSphered(pt, maxDistance), t);
    if (_localBvhTree)
        _localBvhTree->accept(catapultFinder);
    
    if (!catapultFinder.getHaveLineSegment())
        return maxDistance;

    // prepare the returns
    end[0] = catapultFinder.getLineSegment().getStart();
    end[1] = catapultFinder.getLineSegment().getEnd();

    // The linear velocity is the one at the start of the line segment ...
    vel[0] = catapultFinder.getLinearVelocity();
    // ... so the end point has the additional cross product.
    vel[1] = catapultFinder.getLinearVelocity();
    vel[1] += cross(catapultFinder.getAngularVelocity(),
                    catapultFinder.getLineSegment().getDirection());

    // Return the distance to the cat
    return sqrt(distSqr(catapultFinder.getLineSegment(), pt));
}

bool
FGGroundCache::get_agl(double t, const SGVec3d& pt, double max_altoff,
                       SGVec3d& contact, SGVec3d& normal, SGVec3d& vel,
                       int *type, const SGMaterial** material, double *agl)
{
    // Just set up a ground intersection query for the given point
    SGLineSegmentd line(pt - max_altoff*down, pt + 1e4*down);
    simgear::BVHLineSegmentVisitor lineSegmentVisitor(line, t);
    if (_localBvhTree)
        _localBvhTree->accept(lineSegmentVisitor);

    if (!lineSegmentVisitor.empty()) {
        // Have an intersection
        contact = lineSegmentVisitor.getPoint();
        normal = lineSegmentVisitor.getNormal();
        if (0 < dot(normal, down))
            normal = -normal;
        *agl = dot(down, contact - pt);
        vel = lineSegmentVisitor.getLinearVelocity();
        // correct the linear velocity, since the line intersector delivers
        // values for the start point and the get_agl function should
        // traditionally deliver for the contact point
        vel += cross(lineSegmentVisitor.getAngularVelocity(),
                     contact - line.getStart());
        *type = materialToGroundType(lineSegmentVisitor.getMaterial());
        if (material)
            *material = lineSegmentVisitor.getMaterial();

        return true;
    } else {
        // Whenever we did not have a ground triangle for the requested point,
        // take the ground level we found during the current cache build.
        // This is as good as what we had before for agl.
        SGGeod geodPt = SGGeod::fromCart(pt);
        *agl = geodPt.getElevationM() - _altitude;
        geodPt.setElevationM(_altitude);
        contact = SGVec3d::fromGeod(geodPt);
        normal = -down;
        vel = SGVec3d(0, 0, 0);
        *type = _type;
        if (material)
            *material = _material;

        return found_ground;
    }
}

class FGGroundCache::WireIntersector : public BVHVisitor {
public:
    WireIntersector(const SGVec3d pt[4], const double& t) :
        _linearVelocity(SGVec3d::zeros()),
        _angularVelocity(SGVec3d::zeros()),
        _wire(0),
        _time(t)
    {
        // Build the two triangles spanning the area where the hook has moved
        // during the past step.
        _triangles[0].set(pt[0], pt[1], pt[2]);
        _triangles[1].set(pt[0], pt[2], pt[3]);
    }

    virtual void apply(BVHGroup& leaf)
    {
        if (!_intersects(leaf.getBoundingSphere()))
            return;

        leaf.traverse(*this);
    }
    virtual void apply(BVHTransform& transform)
    {
        if (!_intersects(transform.getBoundingSphere()))
            return;
        
        SGTriangled triangles[2] = { _triangles[0], _triangles[1] };
        _triangles[0] = triangles[0].transform(transform.getToLocalTransform());
        _triangles[1] = triangles[1].transform(transform.getToLocalTransform());
        
        transform.traverse(*this);
        
        if (_wire) {
            _lineSegment = transform.lineSegmentToWorld(_lineSegment);
            _linearVelocity = transform.vecToWorld(_linearVelocity);
            _angularVelocity = transform.vecToWorld(_angularVelocity);
        }
        _triangles[0] = triangles[0];
        _triangles[1] = triangles[1];
    }
    virtual void apply(BVHMotionTransform& transform)
    {
        if (!_intersects(transform.getBoundingSphere()))
            return;
        
        SGMatrixd toLocal = transform.getToLocalTransform(_time);

        SGTriangled triangles[2] = { _triangles[0], _triangles[1] };
        _triangles[0] = triangles[0].transform(toLocal);
        _triangles[1] = triangles[1].transform(toLocal);
        
        transform.traverse(*this);
        
        if (_wire) {
            SGMatrixd toWorld = transform.getToWorldTransform(_time);
            _linearVelocity
                += transform.getLinearVelocityAt(_lineSegment.getStart());
            _angularVelocity += transform.getAngularVelocity();
            _linearVelocity = toWorld.xformVec(_linearVelocity);
            _angularVelocity = toWorld.xformVec(_angularVelocity);
            _lineSegment = _lineSegment.transform(toWorld);
        }
        _triangles[0] = triangles[0];
        _triangles[1] = triangles[1];
    }
    virtual void apply(BVHLineGeometry& node)
    {
        if (node.getType() != BVHLineGeometry::CarrierWire)
            return;
        SGLineSegmentd lineSegment(node.getLineSegment());
        if (!_intersects(lineSegment))
            return;

        _lineSegment = lineSegment;
        _linearVelocity = SGVec3d::zeros();
        _angularVelocity = SGVec3d::zeros();
        _wire = &node;
    }
    virtual void apply(BVHStaticGeometry& node)
    { }
    
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticLeaf&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&) { }

    bool _intersects(const SGSphered& sphere) const
    {
        if (_wire)
            return false;
        if (intersects(_triangles[0], sphere))
            return true;
        if (intersects(_triangles[1], sphere))
            return true;
        return false;
    }
    bool _intersects(const SGLineSegmentd& lineSegment) const
    {
        if (_wire)
            return false;
        if (intersects(_triangles[0], lineSegment))
            return true;
        if (intersects(_triangles[1], lineSegment))
            return true;
        return false;
    }
    
    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }
    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }
    
    const BVHLineGeometry* getWire() const
    { return _wire; }
    
private:
    SGLineSegmentd _lineSegment;
    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;
    const BVHLineGeometry* _wire;

    SGTriangled _triangles[2];
    double _time;
};

bool FGGroundCache::caught_wire(double t, const SGVec3d pt[4])
{
    // Get the wire in question
    WireIntersector wireIntersector(pt, t);
    if (_localBvhTree)
        _localBvhTree->accept(wireIntersector);
    
    _wire = wireIntersector.getWire();
    return _wire;
}

class FGGroundCache::WireFinder : public BVHVisitor {
public:
    WireFinder(const BVHLineGeometry* wire, const double& t) :
        _wire(wire),
        _time(t),
        _lineSegment(SGVec3d::zeros(), SGVec3d::zeros()),
        _linearVelocity(SGVec3d::zeros()),
        _angularVelocity(SGVec3d::zeros()),
        _haveLineSegment(false)
    { }

    virtual void apply(BVHGroup& leaf)
    {
        if (_haveLineSegment)
            return;
        leaf.traverse(*this);
    }
    virtual void apply(BVHTransform& transform)
    {
        if (_haveLineSegment)
            return;

        transform.traverse(*this);
        
        if (_haveLineSegment) {
            _linearVelocity = transform.vecToWorld(_linearVelocity);
            _angularVelocity = transform.vecToWorld(_angularVelocity);
            _lineSegment = transform.lineSegmentToWorld(_lineSegment);
        }
    }
    virtual void apply(BVHMotionTransform& transform)
    {
        if (_haveLineSegment)
            return;

        transform.traverse(*this);
        
        if (_haveLineSegment) {
            SGMatrixd toWorld = transform.getToWorldTransform(_time);
            _linearVelocity
                += transform.getLinearVelocityAt(_lineSegment.getStart());
            _angularVelocity += transform.getAngularVelocity();
            _linearVelocity = toWorld.xformVec(_linearVelocity);
            _angularVelocity = toWorld.xformVec(_angularVelocity);
            _lineSegment = _lineSegment.transform(toWorld);
        }
    }
    virtual void apply(BVHLineGeometry& node)
    {
        if (_haveLineSegment)
            return;
        if (_wire != &node)
            return;
        if (node.getType() != BVHLineGeometry::CarrierWire)
            return;
        _lineSegment = SGLineSegmentd(node.getLineSegment());
        _linearVelocity = SGVec3d::zeros();
        _angularVelocity = SGVec3d::zeros();
        _haveLineSegment = true;
    }
    virtual void apply(BVHStaticGeometry&) { }
    
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticLeaf&, const BVHStaticData&) { }
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&) { }

    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }
    
    bool getHaveLineSegment() const
    { return _haveLineSegment; }

    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }

private:
    const BVHLineGeometry* _wire;
    double _time;

    SGLineSegmentd _lineSegment;
    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;

    bool _haveLineSegment;
};

bool FGGroundCache::get_wire_ends(double t, SGVec3d end[2], SGVec3d vel[2])
{
    // Fast return if we do not have an active wire.
    if (!_wire)
        return false;

    // Get the wire in question
    WireFinder wireFinder(_wire, t);
    if (_localBvhTree)
        _localBvhTree->accept(wireFinder);
    
    if (!wireFinder.getHaveLineSegment())
        return false;

    // prepare the returns
    end[0] = wireFinder.getLineSegment().getStart();
    end[1] = wireFinder.getLineSegment().getEnd();

    // The linear velocity is the one at the start of the line segment ...
    vel[0] = wireFinder.getLinearVelocity();
    // ... so the end point has the additional cross product.
    vel[1] = wireFinder.getLinearVelocity();
    vel[1] += cross(wireFinder.getAngularVelocity(),
                    wireFinder.getLineSegment().getDirection());

    return true;
}

void FGGroundCache::release_wire(void)
{
    _wire = 0;
}
