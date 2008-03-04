/**
 * @file precipitation_mgr.cxx
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

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>

#include <osg/Depth>
#include <osg/Stencil>
#include <osg/ClipPlane>
#include <osg/ClipNode>
#include <osg/MatrixTransform>
#include <osgUtil/TransformCallback>

#include <simgear/constants.h>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/sky/cloud.hxx>

#include <Main/fg_props.hxx>

#include "precipitation_mgr.hxx"


extern SGSky *thesky;


void WorldCoordinate( osg::Matrix&, double,
                             double, double, double);

/** 
 * @brief FGPrecipitation Manager constructor 
 *
 * Build a new object to manage the precipitation object
 */
FGPrecipitationMgr::FGPrecipitationMgr()
{	
	osg::Matrix position;
	double latitude, longitude;


	group = new osg::Group();
	transform = new osg::MatrixTransform();
	precipitation = new SGPrecipitation();


	// By default, none precipitation
	precipitation->setRainIntensity(0);
	precipitation->setSnowIntensity(0);


	// Read latitude and longitude position
	latitude = fgGetDouble("/position/latitude-deg", 0.0);
	longitude = fgGetDouble("/position/longitude-deg", 0.0);

	WorldCoordinate(position, latitude, longitude, 0, 0);


	// Move the precipitation object to player position
	transform->setMatrix(position);
	transform->addChild(precipitation->build());

	group->addChild(transform);
}


/** 
 * @brief FGPrecipitaiton Manager destructor
 */
FGPrecipitationMgr::~FGPrecipitationMgr()
{
	delete precipitation;
}


/** 
 * @brief Get OSG precipitation object
 * 
 * @returns A pointer on the OSG object (osg::Group *)
 */
osg::Group * FGPrecipitationMgr::getObject(void)
{
	return this->group;
}


/** 
 * @brief Calculate the max alitutude with precipitation
 * 
 * @returns Elevation max in meter
 *
 * This function permits you to know what is the altitude max where we can
 * find precipitation. The value is returned in meter.
 */
float FGPrecipitationMgr::getPrecipitationAtAltitudeMax(void)
{
	int i;
	int max;
	float result;


	// By default (not cloud layer)
	max = SGCloudLayer::SG_MAX_CLOUD_COVERAGES;
	result = 0;

	// To avoid messing up
	if (thesky == NULL)
		return result;

	// For each cloud layer
	for (i=0; i<thesky->get_cloud_layer_count(); i++) {
		int q;

		// Get coverage
		// Value for q are (meaning / thickness) :
		//   5 : "clear"		/ 0
		//   4 : "cirrus"       / ??
		//   3 : "few" 			/ 65
		//   2 : "scattered"	/ 600
		//   1 : "broken"		/ 750
		//   0 : "overcast"		/ 1000
		q = thesky->get_cloud_layer(i)->getCoverage();

		// Save the coverage max
		if (q < max) {
			max = q;
			result = thesky->get_cloud_layer(i)->getElevation_m();
		}
	}

	return result;
}


/** 
 * @brief Update the precipitation drawing
 * 
 * @returns true is all is OK
 *
 * This function permits you to update the precipitation drawing.
 * This function is called by the renderer.
 */
bool FGPrecipitationMgr::update(void)
{
	double dewtemp;
	double currtemp;
	double rain_intensity;
	double snow_intensity;

	float altitudeAircraft;
	float altitudeCloudLayer;

	// Get the elevation of aicraft and of the cloud layer
	altitudeAircraft = fgGetDouble("/position/altitude-ft", 0.0);
	altitudeCloudLayer = this->getPrecipitationAtAltitudeMax() * SG_METER_TO_FEET;

	if (altitudeAircraft > altitudeCloudLayer) {
		// The aircraft is above the cloud layer
		rain_intensity = 0;
		snow_intensity = 0;
	}
	else {
		// The aircraft is bellow the cloud layer
		rain_intensity = fgGetDouble("/environment/metar/rain-norm", 0.0);
		snow_intensity = fgGetDouble("/environment/metar/snow-norm", 0.0);
	}

	// Get the current and dew temperature
	dewtemp = fgGetDouble("/environment/dewpoint-degc", 0.0);
	currtemp = fgGetDouble("/environment/temperature-degc", 0.0);

	if (currtemp < dewtemp) {
		// There is fog... and the weather is very steamy
		if (rain_intensity == 0)
			rain_intensity = 0.15;
	}

	// If the current temperature is below 0°C, we turn off the rain to snow...
	if (currtemp < 0)
		precipitation->setFreezing(true);
	else
		precipitation->setFreezing(false);


	// Set the wind property
	precipitation->setWindProperty(
		fgGetDouble("/environment/wind-from-heading-deg", 0.0),
		fgGetDouble("/environment/wind-speed-kt", 0.0));

	// Set the intensity of precipitation
	precipitation->setRainIntensity(rain_intensity);
	precipitation->setSnowIntensity(snow_intensity);

	// Update the drawing...
	precipitation->update();

	return true;
}


