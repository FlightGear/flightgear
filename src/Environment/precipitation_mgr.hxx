/**
 * @file precipitation_mgr.hxx
 * @author Nicolas VIVIEN
 * @date 2008-02-10
 *
 * @note Copyright (C) 2008 Nicolas VIVIEN
 *
 * @brief Precipitation manager
 *   This manager calculate the intensity of precipitation in function of the altitude,
 *   calculate the wind direction and velocity, then update the drawing of precipitation.
 *
 * @par Licences
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of the
 *   License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _PRECIPITATION_MGR_HXX
#define _PRECIPITATION_MGR_HXX

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/environment/precipitation.hxx>
#include <simgear/props/tiedpropertylist.hxx>

class FGPrecipitationMgr : public SGSubsystem
{
private:
    osg::ref_ptr<osg::Group> group;
    osg::ref_ptr<osg::MatrixTransform> transform;
    osg::ref_ptr<SGPrecipitation> precipitation;
    float getPrecipitationAtAltitudeMax(void);
    simgear::TiedPropertyList _tiedProperties;

public:
    FGPrecipitationMgr();
    virtual ~FGPrecipitationMgr();

    // SGSubsystem methods
    virtual void bind ();
    virtual void unbind ();
    virtual void init ();
    virtual void update (double dt);
    
    void setPrecipitationLevel(double l);

    osg::Group * getObject(void);

};

#endif

