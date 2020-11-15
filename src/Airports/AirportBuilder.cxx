// AirportBuilder.hxx -- Builder to create airports based on airport data for
//                       rendering in the scenery
//
// Written by Stuart Buchanan, started June 2020
//
// Copyright (C) 2020  Stuart Buchanan stuart13@gmail.com
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

#include <algorithm>

#include <osgDB/Registry>
#include <osgUtil/Tessellator>
#include <osgUtil/DelaunayTriangulator>

#include <osg/Geode>
#include <osg/Geometry>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/math/SGGeodesy.hxx>

#include <Airports/apt_loader.hxx>
#include "airport.hxx"
#include "runways.hxx"
#include "pavement.hxx"

#include "AirportBuilder.hxx"

using namespace osg;
using namespace simgear;

namespace flightgear
{

AirportBuilder::AirportBuilder()
{
    supportsExtension("icao", "Dummy name to build an airport from apt.dat");
}

AirportBuilder::~AirportBuilder()
{
}

const char* AirportBuilder::className() const
{
    return "Airport Builder";
}

osgDB::ReaderWriter::ReadResult AirportBuilder::readNode(const std::string& fileName,
                                                         const osgDB::Options* options) const
{
  SGPath aptFile = SGPath(fileName);

  if (! aptFile.isFile()) {
    // Search for the file if we don't have a full path.
    auto pathList = options->getDatabasePathList();
    for(auto itr = pathList.begin(); itr != pathList.end(); ++itr) {
      aptFile = SGPath(*itr, fileName);
      if (aptFile.isFile()) break;
    }
  }

  if (! aptFile.isFile()) return ReadResult::FILE_NOT_HANDLED;;

  const string airportId = aptFile.file_base();
  APTLoader aptLoader;

  const FGAirport* airport = aptLoader.loadAirportFromFile(airportId, aptFile);
  if (! airport) return ReadResult::FILE_NOT_HANDLED;

  SG_LOG( SG_GENERAL, SG_DEBUG, "Building airport : "  << airportId << " " << airport->getName());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Lat/Lon : "  << airport->getLatitude() << ", " << airport->getLongitude());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Elevation : "  << airport->getElevation());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Runways   : " << airport->numRunways());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Helipads  : " << airport->numHelipads());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Taxiways  : " << airport->numTaxiways());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Pavements : " << airport->numPavements());
  SG_LOG( SG_GENERAL, SG_DEBUG, "Line Features : " << airport->numLineFeatures());

  const SGGeod zeroAltitudeCenter = SGGeod::fromDegM(airport->getLongitude(), airport->getLatitude(), 0.0f);

  const SGVec3f center = SGVec3f::fromGeod(zeroAltitudeCenter);

  // Create a matrix operation that will center the airport facing the z axis
  // We cannot also perform the translate to the center of the airport as we
  // hit floating point precision issues, so we do that separately.
  osg::Matrixd mat = osg::Matrix(toOsg(SGQuatd::fromLonLat(zeroAltitudeCenter)));
  mat.preMultRotate(osg::Quat(0.0, 1.0, 0.0, 0.0));

  std::vector<osg::Node*> nodeList;
  osg::Group* group = new osg::Group();

  // Build the boundary
  auto boundaryList = airport->getBoundary();
  std::for_each( boundaryList.begin(),
                boundaryList.end(),
                [&, mat, center, options] (FGPavementRef p) { group->addChild(this->createBoundary(mat, center, p, options)); } );

  // Build the pavements
  auto pavementlist = airport->getPavements();
  std::for_each( pavementlist.begin(),
                 pavementlist.end(),
                 [&, mat, center, options] (FGPavementRef p) { group->addChild(this->createPavement(mat, center, p, options)); } );

  // Build the runways
  auto runwaylist = airport->getRunways();
  std::for_each( runwaylist.begin(),
                 runwaylist.end(),
                 [&, mat, center, options] (FGRunwayRef p) { group->addChild(this->createRunway(mat, center, p, options)); } );

  // Build line features
  auto lineList = airport->getLineFeatures();
  std::for_each( lineList.begin(),
                lineList.end(),
                [&, mat, center, options] (FGPavementRef p) { group->addChild(this->createLine(mat, center, p, options)); } );

  return group;
}

osg::Node* AirportBuilder::createRunway(const osg::Matrixd mat, const SGVec3f center, const FGRunwayRef runway, const osgDB::Options* options) const
{
  std::vector<SGGeod> nodes;
  nodes.push_back(runway->pointOffCenterline(0.0, -0.5 * runway->widthM()));
  nodes.push_back(runway->pointOffCenterline(0.0,  0.5 * runway->widthM()));
  nodes.push_back(runway->pointOffCenterline(runway->lengthM(),  0.5 * runway->widthM()));
  nodes.push_back(runway->pointOffCenterline(runway->lengthM(), -0.5 * runway->widthM()));

  osg::Vec3Array* points = new osg::Vec3Array;
  points->reserve(nodes.size());
  std::transform( nodes.begin(),
                  nodes.end(),
                  std::back_inserter(*points),
                  [mat, center](SGGeod n) {
                    return mat * toOsg(SGVec3f::fromGeod(n) - center) + osg::Vec3f(0,0, RUNWAY_OFFSET);
                  }
                );

  osg::ref_ptr<osgUtil::DelaunayTriangulator> triangulator = new osgUtil::DelaunayTriangulator;
  triangulator->setInputPointArray(points);
  triangulator->triangulate();

  osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
  geometry->setVertexArray(points);
  geometry->addPrimitiveSet(triangulator->getTriangles()); // triangles with constraint cut

  osg::Vec3Array* n = new osg::Vec3Array;
  n->push_back(osg::Vec3f(0.0f, 0.0f, 1.0f));

  osg::Vec4Array* c = new osg::Vec4Array;
  c->push_back(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

  geometry->setColorArray(c, osg::Array::BIND_OVERALL);
  geometry->setNormalArray(n, osg::Array::BIND_OVERALL);

  EffectGeode* geode = new EffectGeode;
  ref_ptr<Effect> effect = getMaterialEffect("pa_rest", options);
  geode->setEffect(effect.get());
  geode->addDrawable(geometry);

  return geode;
}

osg::Node* AirportBuilder::createPavement(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef pavement, const osgDB::Options* options) const
{
  const FGPavement::NodeList nodes = pavement->getNodeList();
  if (nodes.size() < 2) return NULL;

  osg::ref_ptr<osgUtil::Tessellator> tessellator = new osgUtil::Tessellator;
  tessellator->setBoundaryOnly(false);
  tessellator->beginTessellation();
  tessellator->beginContour();

  // Track the previous vertex for bezier curve generation.
  osg::Vec3f* previous = NULL;

  // Bezier control. Bezier curve is node n-1 -> n -> n+1 so need to store for
  // handling the next node in addition to generating for the BezierNode itself.
  bool bezier = false;
  osg::Vec3f control;

  auto itr = nodes.begin();
  for (; itr < nodes.end(); ++itr) {
    FGPavement::NodeBase* node = *itr;

    osg::Vec3f* v = new osg::Vec3f(mat * toOsg(SGVec3f::fromGeod(node->mPos) - center)  + osg::Vec3f(0,0, PAVEMENT_OFFSET));

    if (FGPavement::BezierNode *bez = dynamic_cast<FGPavement::BezierNode*>(node)) {
      // Store the bezier control node for generation when we know the next node.
      control = mat * toOsg(SGVec3f::fromGeod(bez->mControl) - center)  + osg::Vec3f(0,0, PAVEMENT_OFFSET);
      bezier = true;

      // Generate from the last node to this one, reflecting the control point
      for (float t = 0.05f; t < 0.95f; t += 0.05f ) {

        osg::Vec3f p0 = osg::Vec3f();
        if (previous != NULL) p0 = *previous;
        osg::Vec3f p1 = *v + *v - control;
        osg::Vec3f p2 = *v;

        osg::Vec3f* b = new osg::Vec3f(p1 +
                    (p0 - p1)*(1.0f - t)*(1.0f - t) +
                    (p2 - p1)*t*t);

        tessellator->addVertex(b);
      }

      tessellator->addVertex(v);

    } else if (bezier) {
          // Last node was a BezierNode, so generate the required bezier curve to this node.
          for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
            osg::Vec3f p0 = *previous;
            osg::Vec3f p1 = control;
            osg::Vec3f p2 = *v;

            osg::Vec3f* b = new osg::Vec3f(p1 +
                         (p0 - p1)*(1.0f - t)*(1.0f - t) +
                         (p2 - p1)*t*t);

            tessellator->addVertex(b);
          }

          bezier = false;
          tessellator->addVertex(v);
    } else {
      // SimpleNode
      tessellator->addVertex(v);
    }
    previous = v;
    if (node->mClose) {
      tessellator->endContour();
      tessellator->beginContour();
    }
  }

  tessellator->endContour();
  tessellator->endTessellation();

  // Build the geometry based on the tessellation results.

  osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
  auto primList = tessellator->getPrimList();
  osg::Vec3Array* points = new osg::Vec3Array();
  osg::Vec3Array* normals = new osg::Vec3Array();
  osg::Vec2Array* texCoords = new osg::Vec2Array();
  geometry->setVertexArray(points);
  unsigned int idx = 0;
  auto primItr = primList.begin();
  for (; primItr < primList.end(); ++ primItr) {
    auto vertices = (*primItr)->_vertices;
    std::for_each( vertices.begin(),
                   vertices.end(),
                   [points, texCoords, normals](auto v) {
                     points->push_back(*v);
                     texCoords->push_back(osg::Vec2f(v->x(), v->y()));
                     normals->push_back(osg::Vec3f(0.0, 0.0, 1.0));
                   }
                  );

    geometry->addPrimitiveSet(new osg::DrawArrays((*primItr)->_mode, idx, vertices.size()));
    idx += vertices.size();
  }

  osg::Vec4Array* c = new osg::Vec4Array;
  c->push_back(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

  geometry->setColorArray(c, osg::Array::BIND_OVERALL);
  geometry->setNormalArray(normals, osg::Array::BIND_PER_VERTEX);
  geometry->setTexCoordArray(0, texCoords, osg::Array::BIND_PER_VERTEX);

  EffectGeode* geode = new EffectGeode;
  ref_ptr<Effect> effect = getMaterialEffect("Asphalt", options);
  geode->setEffect(effect.get());
  geode->addDrawable(geometry);

  return geode;
}

osg::Node* AirportBuilder::createBoundary(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef pavement, const osgDB::Options* options) const
{
  const FGPavement::NodeList nodes = pavement->getNodeList();

  if (nodes.size() == 0) return NULL;

  osg::ref_ptr<osgUtil::Tessellator> tessellator = new osgUtil::Tessellator;
  tessellator->setBoundaryOnly(false);
  tessellator->beginTessellation();
  tessellator->beginContour();

  // Track the previous vertex for bezier curve generation.
  osg::Vec3f* previous = NULL;

  // Bezier control. Bezier curve is node n-1 -> n -> n+1 so need to store for
  // handling the next node in addition to generating for the BezierNode itself.
  bool bezier = false;
  osg::Vec3f control;

  auto itr = nodes.begin();
  for (; itr < nodes.end(); ++itr) {
    FGPavement::NodeBase* node = *itr;

    osg::Vec3f* v = new osg::Vec3f(mat * toOsg(SGVec3f::fromGeod(node->mPos) - center) + osg::Vec3f(0,0, BOUNDARY_OFFSET));

    if (FGPavement::BezierNode *bez = dynamic_cast<FGPavement::BezierNode*>(node)) {
      // Store the bezier control node for generation when we know the next node.
      control = osg::Vec3f(mat * toOsg(SGVec3f::fromGeod(bez->mControl) - center) + osg::Vec3f(0,0, BOUNDARY_OFFSET));
      bezier = true;

      // Generate from the last node to this one, reflecting the control point
      for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
        osg::Vec3f p0 = osg::Vec3f();
        if (previous != NULL) p0 = *previous;
        osg::Vec3f p1 = *v + *v - control;
        osg::Vec3f p2 = *v;

        osg::Vec3f* b = new osg::Vec3f(p1 +
                    (p0 - p1)*(1.0f - t)*(1.0f - t) +
                    (p2 - p1)*t*t);

        tessellator->addVertex(b);
      }

      tessellator->addVertex(v);

    } else if (bezier) {
          // Last node was a BezierNode, so generate the required bezier curve to this node.
          for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
            osg::Vec3f p0 = *previous;
            osg::Vec3f p1 = control;
            osg::Vec3f p2 = *v;

            osg::Vec3f* b = new osg::Vec3f(p1 +
                         (p0 - p1)*(1.0f - t)*(1.0f - t) +
                         (p2 - p1)*t*t);

            tessellator->addVertex(b);
          }

          bezier = false;
          tessellator->addVertex(v);
    } else {
      // SimpleNode
      tessellator->addVertex(v);
    }

    previous = v;
    if (node->mClose) {
      tessellator->endContour();
      tessellator->beginContour();
    }
  }

  tessellator->endContour();
  tessellator->endTessellation();

  // Build the geometry based on the tessellation results.
  osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
  auto primList = tessellator->getPrimList();
  osg::Vec3Array* points = new osg::Vec3Array();
  geometry->setVertexArray(points);
  unsigned int idx = 0;
  auto primItr = primList.begin();
  for (; primItr < primList.end(); ++ primItr) {
    auto vertices = (*primItr)->_vertices;
    std::for_each( vertices.begin(),
                   vertices.end(),
                   [points](auto v) { points->push_back(*v); }
                  );

    geometry->addPrimitiveSet(new osg::DrawArrays((*primItr)->_mode, idx, vertices.size()));
    idx += vertices.size();
  }

  osg::Vec3Array* n = new osg::Vec3Array;
  n->push_back(osg::Vec3f(0.0, 0.0, 1.0));

  osg::Vec4Array* c = new osg::Vec4Array;
  c->push_back(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));

  geometry->setColorArray(c, osg::Array::BIND_OVERALL);
  geometry->setNormalArray(n, osg::Array::BIND_OVERALL);

  EffectGeode* geode = new EffectGeode;
  ref_ptr<Effect> effect = getMaterialEffect("Default", options);
  geode->setEffect(effect.get());
  geode->addDrawable(geometry);

  return geode;
}

osg::Node* AirportBuilder::createLine(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef line, const osgDB::Options* options) const
{
  const FGPavement::NodeList nodes = line->getNodeList();

  if (nodes.size() == 0) return NULL;

  // Track the previous vertex for bezier curve generation.
  osg::Vec3f previous;

  // Track along the vertex list to create the correct Drawables
  unsigned int idx = 0;
  unsigned int length = 0;

  // Track the last set line code
  unsigned int paintCode = 0;

  unsigned int lines = 0;

  // Bezier control. Bezier curve is node n-1 -> n -> n+1 so need to store for
  // handling the next node in addition to generating for the BezierNode itself.
  bool bezier = false;
  osg::Vec3f control;

  osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
  osg::Vec3Array* points = new osg::Vec3Array();
  geometry->setVertexArray(points);

  osg::Vec3Array* n = new osg::Vec3Array;
  n->push_back(osg::Vec3f(0.0, 0.0, 1.0));
  geometry->setNormalArray(n, osg::Array::BIND_OVERALL);

  osg::Vec4Array* c = new osg::Vec4Array;
  geometry->setColorArray(c, osg::Array::BIND_PER_VERTEX);

  auto itr = nodes.begin();
  for (; itr < nodes.end(); ++itr) {
    FGPavement::NodeBase* node = *itr;
    osg::Vec3f v = osg::Vec3f(mat * toOsg(SGVec3f::fromGeod(node->mPos) - center) + osg::Vec3f(0,0, MARKING_OFFSET));

    if (node->mPaintCode != 0) paintCode = node->mPaintCode;

    if (FGPavement::BezierNode *bez = dynamic_cast<FGPavement::BezierNode*>(node)) {
      // Store the bezier control node for generation when we know the next node.
      control = osg::Vec3f(mat * toOsg(SGVec3f::fromGeod(bez->mControl) - center) + osg::Vec3f(0,0, MARKING_OFFSET));
      bezier = true;

      // Generate from the last node to this one, reflecting the control point
      for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
        osg::Vec3f p0 = previous;
        osg::Vec3f p1 = v + v - control;
        osg::Vec3f p2 = v;

        osg::Vec3f b =
          osg::Vec3f(p1 +
                    (p0 - p1)*(1.0f - t)*(1.0f - t) +
                    (p2 - p1)*t*t);

        points->push_back(b);
        c->push_back(getLineColor(paintCode));
        length += 1;
      }

      points->push_back(v);
      c->push_back(getLineColor(paintCode));
      length += 1;

      if (node->mClose && node->mLoop) {
        // Special case if this is the end node and it loops back to the start.
        // In this case there is not going to be a forward node to generate, so
        // we need to generate the bezier curve now.

        for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
          osg::Vec3f p0 = v;
          osg::Vec3f p1 = control;
          osg::Vec3f p2 = points->at(idx);

          osg::Vec3f b =
            osg::Vec3f(p1 +
                      (p0 - p1)*(1.0f - t)*(1.0f - t) +
                      (p2 - p1)*t*t);

          points->push_back(b);
          c->push_back(getLineColor(paintCode));
          length += 1;
        }

        bezier = false;
      }

    } else if (bezier) {
          // Last node was a BezierNode, so generate the required bezier curve to this node.
          for (float t = 0.05f; t < 0.95f; t += 0.05f ) {
            osg::Vec3f p0 = previous;
            osg::Vec3f p1 = control;
            osg::Vec3f p2 = v;

            osg::Vec3f b =
              osg::Vec3f(p1 +
                        (p0 - p1)*(1.0f - t)*(1.0f - t) +
                        (p2 - p1)*t*t);

            points->push_back(b);
            c->push_back(getLineColor(paintCode));
            length += 1;
          }

          bezier = false;
    } else {
      // SimpleNode
      points->push_back(v);
      c->push_back(getLineColor(paintCode));
      length += 1;
    }

    previous = v;

    if (node->mClose) {
      geometry->addPrimitiveSet(new osg::DrawArrays(
         node->mLoop ?  GL_LINE_LOOP : GL_LINE_STRIP,
         idx,
         length));
      idx += length;
      length = 0;
      lines++;
    }
  }

  //  We should be using different Effects for different lines, and/or
  //  parameterizing a single Effect to encode some line type information so
  //  we can use a single Effect for all lines.  
  EffectGeode* geode = new EffectGeode;
  ref_ptr<Effect> effect = getMaterialEffect("lf_sng_solid_yellow", options);
  geode->setEffect(effect.get());
  geode->addDrawable(geometry);


  return geode;
}

osg::Vec4f AirportBuilder::getLineColor(const int aPaintCode) const
{
  if (aPaintCode == 1) return osg::Vec4f(0.7f, 0.7f, 0.3f, 1.0f);  // Solid yellow
  if (aPaintCode == 2) return osg::Vec4f(0.7f, 0.7f, 0.3f, 1.0f);  // Broken yellow
  if (aPaintCode == 3) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Double yellow
  if (aPaintCode == 4) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Two broken yellow lines and two solid yellow lines.  Broken line on left of string.
  if (aPaintCode == 5) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Broken yellow line with parallel solid yellow line
  if (aPaintCode == 6) return osg::Vec4f(0.5f, 0.5f, 0.0f, 1.0f);  // Yellow cross-hatched
  if (aPaintCode == 7) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Solid Yellow with broken yellow on each side
  if (aPaintCode == 8) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Widely separated, broken yellow line
  if (aPaintCode == 9) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Widely separated, broken double yellow line

  // As above with black border
  if (aPaintCode == 51) return osg::Vec4f(0.7f, 0.7f, 0.3f, 1.0f);  // Solid yellow
  if (aPaintCode == 52) return osg::Vec4f(0.7f, 0.7f, 0.3f, 1.0f);  // Broken yellow
  if (aPaintCode == 53) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Double yellow
  if (aPaintCode == 54) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Two broken yellow lines and two solid yellow lines.  Broken line on left of string.
  if (aPaintCode == 55) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Broken yellow line with parallel solid yellow line
  if (aPaintCode == 56) return osg::Vec4f(0.5f, 0.5f, 0.0f, 1.0f);  // Yellow cross-hatched
  if (aPaintCode == 57) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Solid Yellow with broken yellow on each side
  if (aPaintCode == 58) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Widely separated, broken yellow line
  if (aPaintCode == 59) return osg::Vec4f(0.9f, 0.9f, 0.3f, 1.0f);  // Widely separated, broken double yellow line

  if (aPaintCode == 20) return osg::Vec4f(0.8f, 0.8f, 0.8f, 1.0f);  // Solid White
  if (aPaintCode == 21) return osg::Vec4f(0.7f, 0.7f, 0.7f, 1.0f);  // White chequerboard
  if (aPaintCode == 22) return osg::Vec4f(0.6f, 0.6f, 0.6f, 1.0f);  // Broken white line

  return osg::Vec4f(0.7f, 0.7f, 0.3f, 1.0f);  // Default (probably an error), so red
}

osg::ref_ptr<Effect> AirportBuilder::getMaterialEffect(std::string material, const osgDB::Options* options) const
{
  ref_ptr<Effect> effect;
  SGPropertyNode_ptr effectProp = new SGPropertyNode();

  osg::ref_ptr<SGReaderWriterOptions> sgOpts(SGReaderWriterOptions::copyOrCreate(options));

  if (sgOpts->getMaterialLib()) {
    const SGGeod loc = SGGeod(sgOpts->getLocation());
    SGMaterialCache* matcache = sgOpts->getMaterialLib()->generateMatCache(loc);
    SGMaterial* mat = matcache->find(material);
    delete matcache;

    if (mat) {
      return mat->get_effect();
    }
  }

  SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get effect for " << material);
  makeChild(effectProp, "inherits-from")->setStringValue("Effects/model-default");
  effectProp->addChild("default")->setBoolValue(true);
  effect = makeEffect(effectProp, true, sgOpts);
  return effect;
}


}
typedef simgear::ModelRegistryCallback<simgear::DefaultProcessPolicy, simgear::NoCachePolicy,
                              simgear::NoOptimizePolicy,
                              simgear::NoSubstitutePolicy, simgear::BuildGroupBVHPolicy>
AirportCallback;

namespace
{
  osgDB::RegisterReaderWriterProxy<flightgear::AirportBuilder> g_readerAirportBuilder;
  simgear::ModelRegistryCallbackProxy<AirportCallback> g_icaoCallbackProxy("icao");
}
