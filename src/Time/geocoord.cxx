/* -*- Mode: C++ -*- *****************************************************
 * geocoord.h
 * Written by Durk Talsma. Started March 1998.
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
 **************************************************************************/

/*************************************************************************
 *
 * This file defines a small and simple class to store geocentric 
 * coordinates. Basically, class GeoCoord is intended as a base class for
 * any kind of of object, that can be categorized according to its 
 * location on earth, be it navaids, or aircraft. This class for originally
 * written for FlightGear, in order to store Timezone control points. 
 *
 ************************************************************************/
#include "geocoord.h"
#include <plib/sg.h>

GeoCoord::GeoCoord(const GeoCoord& other)
{
  lat = other.lat;
  lon = other.lon;
}

// double GeoCoord::getAngle(const GeoCoord& other) const
// {
//   Vector first(      getX(),       getY(),       getZ());
//   Vector secnd(other.getX(), other.getY(), other.getZ());
//     double
//       dot = VecDot(first, secnd),
//       len1 = first.VecLen(),
//       len2 = secnd.VecLen(),
//       len = len1 * len2,
//       angle = 0;
//     //printf ("Dot: %f, len1: %f len2: %f\n", dot, len1, len2);
//     /*Vector pPos = prevPos - Reference->prevPos;
//       Vector pVel = prevVel - Reference->prevVel;*/


//     if ( ( (dot / len) < 1) && (dot / len > -1) && len )
//     	angle = acos(dot / len);
//     return angle;
// }

// GeoCoord* GeoCoordContainer::getNearest(const GeoCoord& ref) const
// {
//   float angle, maxAngle = 180;

//   GeoCoordVectorConstIterator i, nearest;
//   for (i = data.begin(); i != data.end(); i++)
//     {
//       angle = RAD_TO_DEG * (*i)->getAngle(ref);
//       if (angle < maxAngle)
// 	{
// 	  maxAngle = angle;
// 	  nearest = i;
// 	}
//     }
//   return *nearest;
// }


GeoCoord* GeoCoordContainer::getNearest(const GeoCoord& ref) const
{
  sgVec3 first, secnd;
  float dist, maxDist=SG_MAX;
  sgSetVec3( first, ref.getX(), ref.getY(), ref.getZ());
  GeoCoordVectorConstIterator i, nearest;
  for (i = data.begin(); i != data.end(); i++)
    {
      sgSetVec3(secnd, (*i)->getX(), (*i)->getY(), (*i)->getZ());
      dist = sgDistanceSquaredVec3(first, secnd);
      if (dist < maxDist)
	{
	  maxDist = dist;
	  nearest = i;
	}
    }
  return *nearest;
}


GeoCoordContainer::~GeoCoordContainer()
{
    GeoCoordVectorIterator i = data.begin();
  while (i != data.end())
    delete *i++;
}
