//  groundradar.hxx - Background layer for the ATC radar.
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


#ifndef __INST_GROUNDRADAR_HXX
#define __INST_GROUNDRADAR_HXX

#include <osg/ref_ptr>
#include <osg/Geometry>
#include <simgear/props/props.hxx>
#include "od_gauge.hxx"
#include <simgear/structure/subsystem_mgr.hxx>

// forward decls
class FGRunwayBase;

////////////////////////////////////////////////////////////////////////
// Built-in layer for the atc radar.
////////////////////////////////////////////////////////////////////////

class GroundRadar : public SGSubsystem, public SGPropertyChangeListener, private FGODGauge
{
public:
    static const int TextureHalfSize = 256;
    GroundRadar(SGPropertyNode* node);
    virtual ~GroundRadar();
    void updateTexture();
    virtual void valueChanged(SGPropertyNode*);
    virtual void update (double dt);
protected:
    void createTexture(const char* texture_name);
    
    void addRunwayVertices(const FGRunwayBase* aRunway, double aTowerLat, double aTowerLon, double aScale, osg::Vec3Array* aVertices);
    osg::Geometry *addPavementGeometry(const FGPavement* aPavement, double aTowerLat, double aTowerLon, double aScale);
    
    osg::ref_ptr<osg::Geode> _geode;
    SGPropertyNode_ptr _airport_node;
    SGPropertyNode_ptr _range_node;
};

#endif // __INST_GROUNDRADAR_HXX
