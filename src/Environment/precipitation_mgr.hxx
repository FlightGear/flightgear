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
 * @par CVS
 *   $Id$
 */

#ifndef _PRECIPITATION_MGR_HXX
#define _PRECIPITATION_MGR_HXX

#include <simgear/environment/precipitation.hxx>

#include "precipitation_mgr.hxx"


class FGPrecipitationMgr {
private:
	osg::Group *group;
	osg::MatrixTransform *transform;
	SGPrecipitation *precipitation;
	float getPrecipitationAtAltitudeMax(void);


public:
	FGPrecipitationMgr();
	~FGPrecipitationMgr();
	bool update(void);
	osg::Group * getObject(void);
};

#endif

