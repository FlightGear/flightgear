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
 * (Log is kept at end of this file)
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
#include "solarsystem.hxx"

/***************************************************************************
 * default constructor for class  SolarSystem:   
 * or course there can only be one way to create an entire solar system -:) )
 * the fgTIME argument is needed to properly initialize the the current orbital
 * elements
 *************************************************************************/
SolarSystem::SolarSystem(fgTIME *t)
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

/* --------------------------------------------------------------------------
   the destructor for class SolarSystem;
   danger: Huge Explosions ahead! (-:))
   ------------------------------------------------------------------------*/
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
  //delete pluto;
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
  fgLIGHT *l = &cur_light_params;
  fgTIME  *t = &cur_time_params;  
  float x, y, z,
    xx, yy,zz;
  double ra, dec;
  double x_2, x_4, x_8, x_10;
  double magnitude;
  GLfloat ambient;
  GLfloat amb[4];
  GLfloat moonColor[4] = {0.85, 0.75, 0.35, 1.0};
  GLfloat black[4] = {0.0, 0.0,0.0,1.0};
  GLfloat white[4] = {1.0, 1.0,1.0,1.0};

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
                      // running for the first time.
  if (displayList)
    xglDeleteLists(displayList, 1);

  displayList = xglGenLists(1);
  // Step 2: rebuild the display list
  xglNewList( displayList, GL_COMPILE);
  {
    // Step 2a: Add the moon...
    xglEnable( GL_LIGHTING );
    xglEnable( GL_LIGHT0 );
    // set lighting parameters
    xglLightfv(GL_LIGHT0, GL_AMBIENT, white );
    xglLightfv(GL_LIGHT0, GL_DIFFUSE, white );
    xglEnable( GL_CULL_FACE );
    
    // Enable blending, in order to effectively eliminate the dark side of the
    // moon
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    earthsMoon->getPos(&ra, &dec);
    x = 60000.0 * cos(ra) * cos (dec);
    y = 60000.0 * sin(ra) * cos (dec);
    z = 60000.0 * sin(dec);
    xx = cos(ra) * cos(dec);
    yy = sin(ra) * cos(dec);
    zz = sin(dec);
    xglMaterialfv(GL_FRONT, GL_AMBIENT, black);
    xglMaterialfv(GL_FRONT, GL_DIFFUSE, moonColor); 
    xglPushMatrix();
    {
	earthsMoon->newImage(ra,dec);
    }
    xglPopMatrix();
    glDisable(GL_BLEND);
    xglDisable(GL_LIGHTING);
 
    // Step 2b:  Add the sun
    x_2 = l -> sun_angle * l->sun_angle;
    x_4 = x_2 * x_2;
    x_8 = x_4 * x_4;
    x_10 = x_8 * x_2;
    ambient = (0.4 * pow (1.1, - x_10 / 30.0));
    if (ambient < 0.3) ambient = 0.3;
    if (ambient > 1.0) ambient = 1.0;

    amb[0] = 0.00 + ((ambient * 6.0)  - 1.0); // minimum value = 0.8
    amb[1] = 0.00 + ((ambient * 11.0) - 3.0); // minimum value = 0.3
    amb[2] = 0.00 + ((ambient * 12.0) - 3.6); // minimum value = 0.0
    amb[3] = 1.00;

    if (amb[0] > 1.0) amb[0] = 1.0;
    if (amb[1] > 1.0) amb[1] = 1.0;
    if (amb[2] > 1.0) amb[2] = 1.0;

    ourSun->getPos(&ra, &dec);
    x = 60000.0 * cos(ra) * cos(dec);
    y = 60000.0 * sin(ra) * cos(dec);
    z = 60000.0 * sin(dec);
    xglPushMatrix();
    {
      // xglPushMatrix();
      xglTranslatef(x,y,z);
      xglColor3f(amb[0], amb[1], amb[2]);
      glutSolidSphere(1400.0, 10, 10);
    }
    glPopMatrix();
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







