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

////////////////////////////////////////////////////////////////////////
// Built-in layer for the atc radar.
////////////////////////////////////////////////////////////////////////

class GroundRadar : public SGPropertyChangeListener, public FGODGauge
{
public:
    GroundRadar(SGPropertyNode* node);
    virtual ~GroundRadar();
    void updateTexture();
    virtual void valueChanged(SGPropertyNode*);

protected:
    osg::ref_ptr<osg::Geometry> _geom;
    SGPropertyNode_ptr _airport_node;
    SGPropertyNode_ptr _radar_node;

    void createTexture();
};

#endif // __INST_GROUNDRADAR_HXX
