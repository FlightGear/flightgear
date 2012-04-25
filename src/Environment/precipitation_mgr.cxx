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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <osg/MatrixTransform>

#include <simgear/constants.h>
#include <simgear/scene/sky/sky.hxx>
#include <simgear/scene/sky/cloud.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>
#include <Scenery/scenery.hxx>

#include "precipitation_mgr.hxx"

/** 
 * @brief FGPrecipitation Manager constructor 
 *
 * Build a new object to manage the precipitation object
 */
FGPrecipitationMgr::FGPrecipitationMgr()
{	
    group = new osg::Group();
    transform = new osg::MatrixTransform();
    precipitation = new SGPrecipitation();


    // By default, no precipitation
    precipitation->setRainIntensity(0);
    precipitation->setSnowIntensity(0);
    transform->addChild(precipitation->build());
    group->addChild(transform.get());
}


/** 
 * @brief FGPrecipitaiton Manager destructor
 */
FGPrecipitationMgr::~FGPrecipitationMgr()
{

}

/**
 * SGSubsystem initialization
 */
void FGPrecipitationMgr::init()
{
    // Read latitude and longitude position
    SGGeod geod = SGGeod::fromDegM(fgGetDouble("/position/longitude-deg", 0.0),
                                   fgGetDouble("/position/latitude-deg", 0.0),
                                   0.0);
    osg::Matrix position(makeZUpFrame(geod));
    // Move the precipitation object to player position
    transform->setMatrix(position);
    // Add to scene graph
    osg::Group* scenery = globals->get_scenery()->get_scene_graph();
    scenery->addChild(getObject());
    fgGetNode("environment/params/precipitation-level-ft", true);
}

void FGPrecipitationMgr::bind ()
{
  _tiedProperties.setRoot( fgGetNode("/sim/rendering", true ) );
  _tiedProperties.Tie("precipitation-enable", precipitation.get(),
          &SGPrecipitation::getEnabled,
          &SGPrecipitation::setEnabled);
}

void FGPrecipitationMgr::unbind ()
{
  _tiedProperties.Untie();
}

void FGPrecipitationMgr::setPrecipitationLevel(double a)
{
    fgSetDouble("environment/params/precipitation-level-ft",a);
}

/** 
 * @brief Get OSG precipitation object
 * 
 * @returns A pointer on the OSG object (osg::Group *)
 */
osg::Group * FGPrecipitationMgr::getObject(void)
{
    return this->group.get();
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
    SGPropertyNode *boundaryNode, *boundaryEntry;


    // By default (not cloud layer)
    max = SGCloudLayer::SG_MAX_CLOUD_COVERAGES;
    result = 0;

     SGSky* thesky = globals->get_renderer()->getSky();
    
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


    // If we haven't found clouds layers, we read the bounday layers table.
    if (result > 0)
        return result;


    // Read boundary layers node
    boundaryNode = fgGetNode("/environment/config/boundary");

    if (boundaryNode != NULL) {
        i = 0;

        // For each boundary layers
        while ( ( boundaryEntry = boundaryNode->getNode( "entry", i ) ) != NULL ) {
            double elev = boundaryEntry->getDoubleValue( "elevation-ft" );

            if (elev > result)
                result = elev;

            ++i;
        }
    }

	// Convert the result in meter
	result = result * SG_FEET_TO_METER;

    return result;
}


/** 
 * @brief Update the precipitation drawing
 * 
 * To seem real, we stop the precipitation above the cloud or boundary layer.
 * If METAR information doesn't give us this altitude, we will see precipitations
 * in space...
 * Moreover, below 0°C we change rain into snow.
 */
void FGPrecipitationMgr::update(double dt)
{
    double dewtemp;
    double currtemp;
    double rain_intensity;
    double snow_intensity;

    float altitudeAircraft;
    float altitudeCloudLayer;

    altitudeCloudLayer = this->getPrecipitationAtAltitudeMax() * SG_METER_TO_FEET;
    setPrecipitationLevel(altitudeCloudLayer);

	// Does the user enable the precipitation ?
	if (!precipitation->getEnabled() ) {
		// Disable precipitations
	    precipitation->setRainIntensity(0);
	    precipitation->setSnowIntensity(0);

	    // Update the drawing...
	    precipitation->update();

		// Exit
		return;
	}

    // Get the elevation of aicraft and of the cloud layer
    altitudeAircraft = fgGetDouble("/position/altitude-ft", 0.0);

    if ((altitudeCloudLayer > 0) && (altitudeAircraft > altitudeCloudLayer)) {
        // The aircraft is above the cloud layer
        rain_intensity = 0;
        snow_intensity = 0;
    }
    else {
        // The aircraft is bellow the cloud layer
        rain_intensity = fgGetDouble("/environment/rain-norm", 0.0);
        snow_intensity = fgGetDouble("/environment/snow-norm", 0.0);
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
}
