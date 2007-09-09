//  groundradar.cxx - Background layer for the ATC radar.
//
//  Written by Csaba Halasz.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osgViewer/Viewer>

#include <osgText/Text>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/renderer.hxx>
#include <Cockpit/panel.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "groundradar.hxx"

static const char* airport_node_name = "/sim/tower/airport-id";
static const char* texture_name = "Aircraft/Instruments/Textures/od_groundradar.rgb";
static const char* radar_node_name = "/instrumentation/radar/range";

struct SingleFrameCallback : public osg::Camera::DrawCallback
{
    virtual void operator () (const osg::Camera& camera) const
    {
        const_cast<osg::Camera&>(camera).setNodeMask(0);
    }
};

GroundRadar::GroundRadar(SGPropertyNode *node)
{
    _airport_node = fgGetNode(airport_node_name, true);
    _radar_node = fgGetNode(radar_node_name, true);
    createTexture();
    updateTexture();
    _airport_node->addChangeListener(this);
    _radar_node->addChangeListener(this);
}

GroundRadar::~GroundRadar()
{
    _airport_node->removeChangeListener(this);
    _radar_node->removeChangeListener(this);
}

void GroundRadar::valueChanged(SGPropertyNode*)
{
    updateTexture();    
}

inline static osg::Vec3 fromPolar(double fi, double r)
{
    return osg::Vec3(sin(fi * SGD_DEGREES_TO_RADIANS) * r, cos(fi * SGD_DEGREES_TO_RADIANS) * r, 0);
}

void GroundRadar::createTexture()
{        
    setSize(512);
    allocRT();
    
    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(0.0f,0.5f,0.0f,1.0f));
    colors->push_back(osg::Vec4(0.0f,0.5f,0.5f,1.0f));
        
    _geom = new osg::Geometry();
    _geom->setColorArray(colors);
    _geom->setColorBinding(osg::Geometry::BIND_PER_PRIMITIVE_SET);
    _geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 0));
    _geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 0));

    osg::Geode* geode = new osg::Geode();        
    osg::StateSet* stateset = geode->getOrCreateStateSet();
    stateset->setMode(GL_BLEND, osg::StateAttribute::OFF);
    geode->addDrawable(_geom.get());

    osg::Camera* camera = getCamera();        
    camera->setPostDrawCallback(new SingleFrameCallback());
    camera->addChild(geode);
    camera->setNodeMask(0);
    camera->setProjectionMatrixAsOrtho2D(0, getTexture()->getTextureWidth(), 0, getTexture()->getTextureHeight());

    // Texture in the 2D panel system
    FGTextureManager::addTexture(texture_name, getTexture());
}

void GroundRadar::updateTexture()
{
    osg::ref_ptr<osg::Vec3Array> rwy_vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> taxi_vertices = new osg::Vec3Array;

    const string airport_name = _airport_node->getStringValue();
    FGRunwayList* runways = globals->get_runways();

    const FGAirport* airport = fgFindAirportID(airport_name);
    const SGGeod& tower_location = airport->getTowerLocation();
    const double tower_lat = tower_location.getLatitudeDeg();
    const double tower_lon = tower_location.getLongitudeDeg();
    double scale = SG_METER_TO_NM * 200 / _radar_node->getDoubleValue();

    for(FGRunway runway = runways->search(airport_name); runway._id == airport_name; runway = runways->next())
    {
        double az1, az2, dist_m;
        geo_inverse_wgs_84(tower_lat, tower_lon, runway._lat, runway._lon, &az1, &az2, &dist_m);
            
        osg::Vec3 center = fromPolar(az1, dist_m * scale) + osg::Vec3(256, 256, 0);
        osg::Vec3 leftcenter = fromPolar(runway._heading, runway._length * SG_FEET_TO_METER * scale / 2) + center;
        osg::Vec3 lefttop = fromPolar(runway._heading - 90, runway._width * SG_FEET_TO_METER * scale / 2) + leftcenter;
        osg::Vec3 leftbottom = leftcenter * 2 - lefttop;
        osg::Vec3 rightbottom = center * 2 - lefttop;
        osg::Vec3 righttop = center * 2 - leftbottom;

        osg::Vec3Array* vertices = runway._rwy_no[0] == 'x' ? taxi_vertices.get() : rwy_vertices.get();
        vertices->push_back(lefttop);
        vertices->push_back(leftbottom);
        vertices->push_back(rightbottom);
        vertices->push_back(righttop);
    }
        
    osg::Vec3Array* vertices = new osg::Vec3Array(*taxi_vertices.get());
    vertices->insert(vertices->end(), rwy_vertices->begin(), rwy_vertices->end());
    _geom->setVertexArray(vertices);
    osg::DrawArrays* taxi = dynamic_cast<osg::DrawArrays*>(_geom->getPrimitiveSet(0));
    osg::DrawArrays* rwy = dynamic_cast<osg::DrawArrays*>(_geom->getPrimitiveSet(1));
    taxi->setCount(taxi_vertices->size());
    rwy->setFirst(taxi_vertices->size());
    rwy->setCount(rwy_vertices->size());
    
    getCamera()->setNodeMask(0xffffffff);
}

// end of GroundRadar.cxx
