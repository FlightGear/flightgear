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
#include <osgDB/Registry>
#include "airport.hxx"
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

namespace flightgear
{

  static const float RUNWAY_OFFSET   = 3.0;

class AirportBuilder : public osgDB::ReaderWriter {
public:

    // The different layers are offset to avoid z-buffering issues.  As they
    // are viewed from above only, this doesn't cause any problems visually.
    const float MARKING_OFFSET  = 2.0;
    const float PAVEMENT_OFFSET = 1.0;
    const float BOUNDARY_OFFSET = 0.0;

    AirportBuilder();
    virtual ~AirportBuilder();

    virtual const char* className() const;

    virtual ReadResult readNode(const std::string& fileName,
                                const osgDB::Options* options)
        const;

private:
    osg::Node* createAirport(const std::string airportId) const;
    osg::Node* createRunway(const osg::Matrixd mat, const SGVec3f center, const FGRunwayRef runway, const osgDB::Options* options) const;
    osg::Node* createPavement(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef pavement, const osgDB::Options* options) const;
    osg::Node* createBoundary(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef pavement, const osgDB::Options* options) const;
    osg::Node* createLine(const osg::Matrixd mat, const SGVec3f center, const FGPavementRef pavement, const osgDB::Options* options) const;
    osg::Vec4f getLineColor(const int aPaintCode) const;
    osg::ref_ptr<simgear::Effect> getMaterialEffect(std::string material, const osgDB::Options* options) const;
};

}
