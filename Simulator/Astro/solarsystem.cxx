/**************************************************************************
 * solarsystem.cxx
 * Written by Durk Talsma. Originally started October 1997, for distribution  
 * with the FlightGear project. Version 2 was written in August and 
 * September 1998. This code is based upon algorithms and data kindly 
 * provided by Mr. Paul Schlyter. (pausch@saaf.se). 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#include <GL/glut.h>
#include <XGL/xgl.h>
#include <Debug/logstream.hxx>
#include <Time/sunpos.hxx>
#include <Time/moonpos.hxx>
#include "solarsystem.hxx"

/***************************************************************************
 * default constructor for class  SolarSystem:   
 * or course there can only be one way to create an entire solar system -:) )
 * the FGTime argument is needed to properly initialize the the current orbital
 * elements
 *************************************************************************/
SolarSystem::SolarSystem(FGTime *t)
{
  if (theSolarSystem)
    {
      FG_LOG( FG_GENERAL, FG_ALERT, "Error: only one solarsystem allowed" );
      exit(-1);
    }
  theSolarSystem = this;
  ourSun     = new Star(t);   
  earthsMoon = new Moon(t);
  mercury    = new Mercury(t);
  venus      = new Venus(t);
  mars       = new Mars(t);
  jupiter    = new Jupiter(t);
  saturn     = new Saturn(t);
  uranus     = new Uranus(t);
  neptune    = new Neptune(t);

  displayList = 0;
};

/**************************************************************************
 * the destructor for class SolarSystem;
 **************************************************************************/
SolarSystem::~SolarSystem()
{
  delete ourSun;
  delete earthsMoon;
  delete mercury;
  delete venus;
  delete mars;
  delete jupiter;
  delete saturn;
  delete uranus;
  delete neptune;
}
/****************************************************************************
 * void SolarSystem::rebuild()
 *
 * this member function updates the positions for the sun, moon, and planets,
 * and then rebuilds the display list. 
 *
 * arguments: none
 * return value: none
 ***************************************************************************/
void SolarSystem::rebuild()
{
  //fgLIGHT *l = &cur_light_params;
  FGTime *t = FGTime::cur_time_params;  
  //float x, y, z;
  //double sun_angle;
  double ra, dec;
  //double x_2, x_4, x_8, x_10;*/
  double magnitude;
  //GLfloat ambient;
  //GLfloat amb[4];
  
  
  glDisable(GL_LIGHTING);


  // Step 1: update all the positions
  ourSun->updatePosition(t);
  earthsMoon->updatePosition(t, ourSun);
  mercury->updatePosition(t, ourSun);
  venus->updatePosition(t, ourSun);
  mars->updatePosition(t, ourSun);
  jupiter->updatePosition(t, ourSun);
  saturn->updatePosition(t, ourSun);
  uranus->updatePosition(t, ourSun);
  neptune->updatePosition(t, ourSun);
  
  fgUpdateSunPos();   // get the right sun angle (especially important when 
                      // running for the first time).
  fgUpdateMoonPos();

  if (displayList)
    xglDeleteLists(displayList, 1);

  displayList = xglGenLists(1);

  // Step 2: rebuild the display list
  xglNewList( displayList, GL_COMPILE);
  {
    // Step 2a: Add the moon...
    // Not that it is preferred to draw the moon first, and the sun next, in order to mime a
    // solar eclipse. This is yet untested though...

    earthsMoon->newImage();
    // Step 2b:  Add the sun
    //xglPushMatrix();
    //{
      ourSun->newImage();
      //}
    //xglPopMatrix();
    // Step 2c: Add the planets
    xglBegin(GL_POINTS);
    mercury->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    venus  ->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    mars   ->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    jupiter->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    saturn ->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    uranus ->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    neptune->getPos(&ra, &dec, &magnitude);addPlanetToList(ra, dec, magnitude);
    xglEnd();
    xglEnable(GL_LIGHTING);
  }
  xglEndList();
}

/*****************************************************************************
 * double SolarSystem::scaleMagnitude(double magn)
 * This private member function rescales the original magnitude, as used in the
 * astronomical sense of the word, into a value used by OpenGL to draw a 
 * convincing Star or planet
 * 
 * Argument: the astronomical magnitude
 *
 * return value: the rescaled magnitude
 ****************************************************************************/
double SolarSystem::scaleMagnitude(double magn)
{
  double magnitude = (0.0 - magn) / 5.0 + 1.0;
  magnitude = magnitude * 0.7 + (3 * 0.1);
  if (magnitude > 1.0) magnitude = 1.0;
  if (magnitude < 0.0) magnitude = 0.0;
  return magnitude;
}

/***************************************************************************
 * void SolarSytem::addPlanetToList(double ra, double dec, double magn);
 *
 * This private member function first causes the magnitude to be properly
 * rescaled, and then adds the planet to the display list.
 * 
 * arguments: Right Ascension, declination, and magnitude
 *
 * return value: none
 **************************************************************************/
void SolarSystem::addPlanetToList(double ra, double dec, double magn)
{
  double
    magnitude = scaleMagnitude ( magn );

  fgLIGHT *l = &cur_light_params;

  if ((double) (l->sun_angle - FG_PI_2) > 
      ((magnitude - 1.0) * - 20 * DEG_TO_RAD)) 
    {
      xglColor3f (magnitude, magnitude, magnitude);
      xglVertex3f( 50000.0 * cos (ra) * cos (dec),
		   50000.0 * sin (ra) * cos (dec),
		   50000.0 * sin (dec));
    }
}


SolarSystem* SolarSystem::theSolarSystem = 0;

/******************************************************************************
 * void solarSystemRebuild()
 * this a just a wrapper function, provided for use as an interface to the 
 * event manager
 *****************************************************************************/
void solarSystemRebuild()
{
  SolarSystem::theSolarSystem->rebuild();
}
