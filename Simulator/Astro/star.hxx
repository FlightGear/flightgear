/**************************************************************************
 * star.hxx
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
#ifndef _STAR_HXX_
#define _STAR_HXX_

#include <Time/fg_time.hxx>
#include "celestialBody.hxx"


class Star : public CelestialBody
{
private:
  //double longitude;  // the sun's true longitude - this is depreciated by
                       // CelestialBody::lonEcl 
  double xs, ys;     // the sun's rectangular geocentric coordinates
  double distance;   // the sun's distance to the earth
   GLUquadricObj *SunObject;
  GLuint sun_texid;
  GLubyte *sun_texbuf;

  void setTexture();
public:
  Star (fgTIME *t);
  ~Star();
  void updatePosition(fgTIME *t);
  double getM();
  double getw();
  //double getLon();
  double getxs();
  double getys();
  double getDistance();
  void newImage();
};



inline double Star::getM()
{
  return M;
}

inline double Star::getw()
{
  return w;
}

inline double Star::getxs()
{
  return xs;
}

inline double Star::getys()
{
  return ys;
}

inline double Star::getDistance()
{
  return distance;
}


#endif // _STAR_HXX_














