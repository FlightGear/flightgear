/**************************************************************************
 * solarsystem.hxx
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
#ifndef _SOLARSYSTEM_H_
#define _SOLARSYSTEM_H_

#include <Time/light.hxx>
#include <Time/fg_time.hxx>
#include <Main/views.hxx>

#include "star.hxx"
#include "moon.hxx"
#include "mercury.hxx"
#include "venus.hxx"
#include "mars.hxx"
#include "jupiter.hxx"
#include "saturn.hxx"
#include "uranus.hxx"
#include "neptune.hxx"
#include "pluto.hxx"


extern fgLIGHT cur_light_params;
extern fgTIME  cur_time_params;
extern fgVIEW  current_view;



class SolarSystem
{
private:
  Star*    ourSun;
  Moon*    earthsMoon;
  Mercury* mercury;
  Venus*   venus;
  Mars*    mars;
  Jupiter* jupiter;
  Saturn*  saturn;
  Uranus*  uranus;
  Neptune* neptune;
  //Pluto*   pluto;
  
  GLint displayList;
  double scaleMagnitude(double magn);
  void addPlanetToList(double ra, double dec, double magn);


public:
  SolarSystem(fgTIME *t);
  Star *getSun();
  ~SolarSystem();

  static SolarSystem *theSolarSystem;  // thanks to Bernie Bright!
  void rebuild();
  friend void solarSystemRebuild();
  void draw();
  
};

inline Star * SolarSystem::getSun()
{
  return ourSun;
}

inline void SolarSystem::draw()
{
  xglCallList(displayList);
}
  
extern void solarSystemRebuild();

#endif // _SOLARSYSTEM_H_













