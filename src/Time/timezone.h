/* -*- Mode: C++ -*- *****************************************************
 * timezone.h
 * Written by Durk Talsma. Started July 1999.
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
 * Timezone is derived from geocoord, and stores the timezone centerpoint,
 * as well as the countrycode and the timezone descriptor. The latter is 
 * used in order to get the local time. 
 *
 ************************************************************************/

#ifndef _TIMEZONE_H_
#define _TIMEZONE_H_

#include "geocoord.h"
#include <stdio.h>

class Timezone : public GeoCoord
{
private:
  char* countryCode;
  char* descriptor;

public:
  Timezone() :
    GeoCoord()
    { 
      countryCode = 0; 
      descriptor = 0;
    };
  Timezone(float la, float lo, char* cc, char* desc);
  Timezone(const char *infoString);
  Timezone(const Timezone &other);
  virtual ~Timezone() { delete [] countryCode; delete [] descriptor; };
  

  virtual void print() { printf("%s", descriptor);};
  virtual char * getDescription() { return descriptor; };
};

/************************************************************************
 * Timezone container is derived from GeoCoordContainer, and has some 
 * added functionality.
 ************************************************************************/

class TimezoneContainer : public GeoCoordContainer
{
 public:
  TimezoneContainer(const char *filename);
  virtual ~TimezoneContainer();
};



#endif // _TIMEZONE_H_
