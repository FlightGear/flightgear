/* -*- Mode: C++ -*- *****************************************************
 * timezone.cc
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
#include <stdio.h>
#include "timezone.h"


Timezone::Timezone(float la, float lo, char* cc, char* desc) :
  GeoCoord(la, lo)
{ 
  countryCode = strdup(cc);
  descriptor = strdup(desc);
}

/* Build a timezone object from a textline in zone.tab */
Timezone::Timezone(const char *infoString) :
  GeoCoord()
{
  int i = 0;
  while (infoString[i] != '\t')
    i++;
  char buffer[128];
  char latlon[128];
  strncpy(buffer, infoString, i);
  buffer[i] = 0;
  countryCode = strdup(buffer);
  i ++;
  int start = i;
  while (infoString[i] != '\t')
    i++;
  int size = i - start;
  strncpy(latlon, (&infoString[start]), size);
  latlon[size] = 0;
  char sign;
  sign = latlon[0];
  strncpy(buffer, &latlon[1], 2);
  lat = atof(buffer);
  strncpy(buffer, &latlon[3], 2);
  lat += (atof(buffer) / 60);
  int nextPos;
  if (strlen(latlon) > 12)
    {
      nextPos = 7;
      strncpy(buffer, &latlon[5], 2);
      lat += (atof(buffer) / 3600.0);
    }
  else
    nextPos = 5;
  if (sign == '-')
    lat = -lat;

  sign = latlon[nextPos];
  nextPos++;
  strncpy(buffer, &latlon[nextPos], 3);
  lon = atof(buffer);
  nextPos += 3;
  strncpy(buffer, &latlon[nextPos], 2);
  buffer[2] = 0;
 
  lon  += (atof(buffer) / 60);
  if (strlen(latlon) > 12)
    {
      nextPos += 2;
      strncpy(buffer, &latlon[nextPos], 2); 
      lon +=  (atof (buffer) / 3600.00);
    }
  if (sign == '-')
    lon = -lon;
  i ++;
  start = i;
  while (!((infoString[i] == '\t') || (infoString[i] == '\n')))
    i++;
  size = i - start;
  strncpy(buffer, (&infoString[start]), size);
  buffer[size] = 0;
  descriptor = strdup(buffer);
}

/* the copy constructor */
Timezone::Timezone(const Timezone& other)
{
  lat = other.getLat();
  lon = other.getLon();
  countryCode = strdup(other.countryCode);
  descriptor = strdup(other.descriptor);
}


/********* Member functions for TimezoneContainer class ********/

TimezoneContainer::TimezoneContainer(const char *filename)
{
  char buffer[256];
  FILE* infile = fopen(filename, "r");
  if (!(infile))
    {
      fprintf(stderr, "Unable to open file %s\n", filename);
      exit(1);
    }
  else
    { 
      
      while (1)
	{
	  fgets(buffer, 256, infile);
	  if (feof(infile))
	    break;
	  for (int i = 0; i < 256; i++)
	    {
	      if (buffer[i] == '#')
		buffer[i] = 0;
	    }
	  if (buffer[0])
	    {
	      data.push_back(new Timezone(buffer));
	    }
	}
    }
}

TimezoneContainer::~TimezoneContainer()
{
}
