//  groundradar.cxx - Background layer for the ATC radar.
//
//  Copyright (C) 2007 Csaba Halasz.
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

#include <cassert>

#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osgViewer/Viewer>

#include <osgText/Text>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgUtil/Tessellator>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <Cockpit/panel.hxx>
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <Airports/pavement.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/beziercurve.hxx>

#include "groundradar.hxx"

static const char* airport_source_node_name = "airport-id-source";
static const char* default_airport_node_name = "/sim/airport/closest-airport-id";
static const char* texture_node_name = "texture-name";
static const char* default_texture_name = "Aircraft/Instruments/Textures/od_groundradar.rgb";
static const char* range_source_node_name = "range-source";
static const char* default_range_node_name = "/instrumentation/radar/range";

struct SingleFrameCallback : public osg::Camera::DrawCallback
{
    virtual void operator () (const osg::Camera& camera) const
    {
        const_cast<osg::Camera&>(camera).setNodeMask(0);
    }
};

GroundRadar::GroundRadar(SGPropertyNode *node)
{
    _airport_node = fgGetNode(node->getStringValue(airport_source_node_name, default_airport_node_name), true);
    _range_node = fgGetNode(node->getStringValue(range_source_node_name, default_range_node_name), true);
    createTexture(node->getStringValue(texture_node_name, default_texture_name));
    updateTexture();
    _airport_node->addChangeListener(this);
    _range_node->addChangeListener(this);
}

GroundRadar::~GroundRadar()
{
    _airport_node->removeChangeListener(this);
    _range_node->removeChangeListener(this);
}

void GroundRadar::update (double /* dt */)
{
  
}

void GroundRadar::valueChanged(SGPropertyNode*)
{
    updateTexture();
}

inline static osg::Vec3 fromPolar(double fi, double r)
{
    return osg::Vec3(sin(fi * SGD_DEGREES_TO_RADIANS) * r, cos(fi * SGD_DEGREES_TO_RADIANS) * r, 0);
}

void GroundRadar::createTexture(const char* texture_name)
{
    setSize(TextureHalfSize + TextureHalfSize);
    allocRT();

    _geode = new osg::Geode();
    osg::StateSet* stateset = _geode->getOrCreateStateSet();
    stateset->setMode(GL_BLEND, osg::StateAttribute::OFF);

    osg::Vec4Array* taxi_color = new osg::Vec4Array;
    taxi_color->push_back(osg::Vec4(0.0f, 0.5f, 0.0f, 1.0f));
    osg::Vec4Array* rwy_color = new osg::Vec4Array;
    rwy_color->push_back(osg::Vec4(0.0f, 0.5f, 0.5f, 1.0f));

    osg::Geometry *taxi_geom = new osg::Geometry();
    taxi_geom->setColorArray(taxi_color);
    taxi_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    taxi_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 0)); // Taxiways
    _geode->addDrawable(taxi_geom);

    osg::Geometry *pvt_geom = new osg::Geometry();
    pvt_geom->setColorArray(taxi_color);
    pvt_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    // no primitive set for the moment. It needs tessellation
    _geode->addDrawable(pvt_geom);

    osg::Geometry *rwy_geom = new osg::Geometry();
    rwy_geom->setColorArray(rwy_color);
    rwy_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
    rwy_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 0)); // Runways
    _geode->addDrawable(rwy_geom);

    osg::Camera* camera = getCamera();
    camera->setPostDrawCallback(new SingleFrameCallback());
    camera->addChild(_geode.get());
    camera->setNodeMask(0);
    camera->setProjectionMatrixAsOrtho2D(0, getTexture()->getTextureWidth(), 0, getTexture()->getTextureHeight());

    // Texture in the 2D panel system
    FGTextureManager::addTexture(texture_name, getTexture());
}

void GroundRadar::addRunwayVertices(const FGRunwayBase* aRunway, double aTowerLat, double aTowerLon, double aScale, osg::Vec3Array* aVertices)
{
  double az1, az2, dist_m;
  geo_inverse_wgs_84(aTowerLat, aTowerLon, aRunway->latitude(), aRunway->longitude(), &az1, &az2, &dist_m);

  osg::Vec3 center = fromPolar(az1, dist_m * aScale) + osg::Vec3(TextureHalfSize, TextureHalfSize, 0);
  osg::Vec3 leftcenter = fromPolar(aRunway->headingDeg(), aRunway->lengthM() * aScale / 2) + center;
  osg::Vec3 lefttop = fromPolar(aRunway->headingDeg() - 90, aRunway->widthM() * aScale / 2) + leftcenter;
  osg::Vec3 leftbottom = leftcenter * 2 - lefttop;
  osg::Vec3 rightbottom = center * 2 - lefttop;
  osg::Vec3 righttop = center * 2 - leftbottom;

  aVertices->push_back(lefttop);
  aVertices->push_back(leftbottom);
  aVertices->push_back(rightbottom);
  aVertices->push_back(righttop);
}

osg::Geometry *GroundRadar::addPavementGeometry(const FGPavement* aPavement, double aTowerLat, double aTowerLon, double aScale)
{

  osg::ref_ptr<osgUtil::Tessellator> tess = new osgUtil::Tessellator;
  osg::ref_ptr<osg::Geometry> polygon = new osg::Geometry;
  osg::ref_ptr<osg::Vec3Array> pts = new osg::Vec3Array;

  double az1, az2, dist_m;
  const FGPavement::NodeList &nodeLst = aPavement->getNodeList();
  FGPavement::NodeList::const_iterator it = nodeLst.begin(),
                                        loopBegin = it;
  while ( it != nodeLst.end() )
  {
    bool close = (*it)->mClose;
    geo_inverse_wgs_84(aTowerLat, aTowerLon, (*it)->mPos.getLatitudeDeg(), (*it)->mPos.getLongitudeDeg(), &az1, &az2, &dist_m);
    osg::Vec3 p1 = fromPolar(az1, dist_m * aScale) + osg::Vec3(TextureHalfSize, TextureHalfSize, 0);
    const FGPavement::BezierNode *bn = dynamic_cast<const FGPavement::BezierNode *>( it->ptr() );
    if ( bn != 0 )
    {
      geo_inverse_wgs_84(aTowerLat, aTowerLon, bn->mControl.getLatitudeDeg(), bn->mControl.getLongitudeDeg(), &az1, &az2, &dist_m);
      osg::Vec3 p2 = fromPolar(az1, dist_m * aScale) + osg::Vec3(TextureHalfSize, TextureHalfSize, 0),
                p3;
      ++it;
      if ( it == nodeLst.end() || close )
      {
        geo_inverse_wgs_84(aTowerLat, aTowerLon, (*loopBegin)->mPos.getLatitudeDeg(), (*loopBegin)->mPos.getLongitudeDeg(), &az1, &az2, &dist_m);
      }
      else
      {
        geo_inverse_wgs_84(aTowerLat, aTowerLon, (*it)->mPos.getLatitudeDeg(), (*it)->mPos.getLongitudeDeg(), &az1, &az2, &dist_m);
      }
      p3 = fromPolar(az1, dist_m * aScale) + osg::Vec3(TextureHalfSize, TextureHalfSize, 0);
      simgear::BezierCurve<osg::Vec3> bCurv( p1, p2, p3 );
      simgear::BezierCurve<osg::Vec3>::PointList &ptList = bCurv.pointList();
      for ( simgear::BezierCurve<osg::Vec3>::PointList::iterator ii = ptList.begin(); ii != ptList.end(); ++ii )
      {
        pts->push_back( *ii );
      }
      pts->pop_back(); // Last point belongs to next segment
    }
    else
    {
      pts->push_back( p1 );
      ++it;
    }

    if ( close ) // One loop for the moment
      break;
  }
  geo_inverse_wgs_84(aTowerLat, aTowerLon, (*loopBegin)->mPos.getLatitudeDeg(), (*loopBegin)->mPos.getLongitudeDeg(), &az1, &az2, &dist_m);
  osg::Vec3 p1 = fromPolar(az1, dist_m * aScale) + osg::Vec3(TextureHalfSize, TextureHalfSize, 0);
  pts->push_back( p1 );
  polygon->setVertexArray( pts.get() );

  polygon->addPrimitiveSet( new osg::DrawArrays( osg::PrimitiveSet::POLYGON, 0, pts->size() ) );

  tess->setTessellationType( osgUtil::Tessellator::TESS_TYPE_GEOMETRY );
  tess->setBoundaryOnly( false );
  tess->setWindingType( osgUtil::Tessellator::TESS_WINDING_ODD );
  tess->retessellatePolygons( *polygon );
  return polygon.release();
}

void GroundRadar::updateTexture()
{
    osg::ref_ptr<osg::Vec3Array> rwy_vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> taxi_vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> pvt_vertices = new osg::Vec3Array;

    const string airport_name = _airport_node->getStringValue();

    const FGAirport* airport = fgFindAirportID(airport_name);
    if (airport == 0)
        return;

    const SGGeod& tower_location = airport->getTowerLocation();
    const double tower_lat = tower_location.getLatitudeDeg();
    const double tower_lon = tower_location.getLongitudeDeg();
    double scale = SG_METER_TO_NM * 200 / _range_node->getDoubleValue();
  
    const FGAirport* apt = fgFindAirportID(airport_name);
    assert(apt);

    for (unsigned int i=0; i<apt->numTaxiways(); ++i)
    {
      FGTaxiway* txwy(apt->getTaxiwayByIndex(i));
      addRunwayVertices(txwy, tower_lat, tower_lon, scale, taxi_vertices.get());
    }
    osg::Geometry *taxi_geom = dynamic_cast<osg::Geometry *>(_geode->getDrawable(0));
    taxi_geom->setVertexArray(taxi_vertices.get());
    osg::DrawArrays* taxi = dynamic_cast<osg::DrawArrays*>(taxi_geom->getPrimitiveSet(0));
    taxi->setCount(taxi_vertices->size());

    osg::Geometry *pvt_geom = dynamic_cast<osg::Geometry *>(_geode->getDrawable(1));
    osg::Geometry::PrimitiveSetList &pvt_prim_list = pvt_geom->getPrimitiveSetList();
    pvt_prim_list.clear();
    for (unsigned int i=0; i<apt->numPavements(); ++i)
    {
      FGPavement* pvt(apt->getPavementByIndex(i));
      osg::ref_ptr<osg::Geometry> geom = addPavementGeometry(pvt, tower_lat, tower_lon, scale);
      osg::Geometry::PrimitiveSetList &prim_list = geom->getPrimitiveSetList();
      osg::Vec3Array *vertices = dynamic_cast<osg::Vec3Array *>(geom->getVertexArray());
      size_t before = pvt_vertices->size(),
            count = vertices->size();
      for (size_t i = 0; i < count; ++i )
      {
        pvt_vertices->push_back( (*vertices)[i] );
      }
      for (osg::Geometry::PrimitiveSetList::iterator ii = prim_list.begin(); ii != prim_list.end(); ++ii )
      {
        osg::DrawArrays *da;
        osg::DrawElementsUByte *de1;
        osg::DrawElementsUShort *de2;
        osg::DrawElementsUInt *de3;
        if ((da = dynamic_cast<osg::DrawArrays *>(ii->get())) != 0)
        {
          osg::DrawArrays *ps = new osg::DrawArrays(*da);
          ps->setFirst(da->getFirst() + before);
          pvt_prim_list.push_back(ps);
        }
        else if ((de1 = dynamic_cast<osg::DrawElementsUByte *>(ii->get())) != 0)
        {
          if (before + count <= 255)
          {
            osg::DrawElementsUByte *ps = new osg::DrawElementsUByte(*de1);
            for (size_t j = 0; j < ps->size(); ++j)
            {
              (*ps)[j] += before;
            }
            pvt_prim_list.push_back(ps);
          }
          else if (before + count <= 65535)
          {
            osg::DrawElementsUShort *ps = new osg::DrawElementsUShort(de1->getMode(), de1->begin(), de1->end());
            for (size_t j = 0; j < ps->size(); ++j)
            {
              (*ps)[j] += before;
            }
            pvt_prim_list.push_back(ps);
          }
          else
          {
            osg::DrawElementsUInt *ps = new osg::DrawElementsUInt(de1->getMode(), de1->begin(), de1->end());
            for (size_t j = 0; j < ps->size(); ++j)
            {
              (*ps)[j] += before;
            }
            pvt_prim_list.push_back(ps);
          }
        }
        else if ((de2 = dynamic_cast<osg::DrawElementsUShort *>(ii->get())) != 0)
        {
          if (before + count <= 65535)
          {
            osg::DrawElementsUShort *ps = new osg::DrawElementsUShort(*de2);
            for (size_t j = 0; j < ps->size(); ++j)
            {
              (*ps)[j] += before;
            }
            pvt_prim_list.push_back(ps);
          }
          else
          {
            osg::DrawElementsUInt *ps = new osg::DrawElementsUInt(de2->getMode(), de2->begin(), de2->end());
            for (size_t j = 0; j < ps->size(); ++j)
            {
              (*ps)[j] += before;
            }
            pvt_prim_list.push_back(ps);
          }
        }
        else if ((de3 = dynamic_cast<osg::DrawElementsUInt *>(ii->get())) != 0)
        {
          osg::DrawElementsUInt *ps = new osg::DrawElementsUInt(*de3);
          for (size_t j = 0; j < ps->size(); ++j)
          {
            (*ps)[j] += before;
          }
          pvt_prim_list.push_back(ps);
        }
      }
    }
    pvt_geom->setVertexArray(pvt_vertices.get());

    for (unsigned int i=0; i<apt->numRunways(); ++i)
    {
      FGRunway* runway(apt->getRunwayByIndex(i));
      if (runway->isReciprocal()) continue;
      
      addRunwayVertices(runway, tower_lat, tower_lon, scale, rwy_vertices.get());
    }
    osg::Geometry *rwy_geom = dynamic_cast<osg::Geometry *>(_geode->getDrawable(2));
    rwy_geom->setVertexArray(rwy_vertices.get());
    osg::DrawArrays* rwy = dynamic_cast<osg::DrawArrays*>(rwy_geom->getPrimitiveSet(0));
    rwy->setCount(rwy_vertices->size());

    getCamera()->setNodeMask(0xffffffff);
}

// end of GroundRadar.cxx
