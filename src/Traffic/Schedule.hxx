/* -*- Mode: C++ -*- *****************************************************
 * Schedule.hxx
 * Written by Durk Talsma. Started May 5, 2004
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
 *
 **************************************************************************/

/**************************************************************************
 * This file contains the definition of the class Shedule.
 *
 * A schedule is basically a number of scheduled flights, wich can be
 * assigned to an AI aircraft. 
 **************************************************************************/

#ifndef _FGSCHEDULE_HXX_
#define _FGSCHEDULE_HXX_


class FGAISchedule
{
 private:
  string modelPath;
  string livery;
  string registration;
  bool heavy;
  FGScheduledFlightVec flights;
  float lat;
  float lon; 
  int AIManagerRef;
  bool firstRun;

 public:
  FGAISchedule();                                           // constructor
  FGAISchedule(string, string, string, bool, FGScheduledFlightVec);  // construct & init
  FGAISchedule(const FGAISchedule &other);             // copy constructor

  ~FGAISchedule(); //destructor

  void update(time_t now);
  // More member functions follow later

};

typedef vector<FGAISchedule>           ScheduleVector;
typedef vector<FGAISchedule>::iterator ScheduleVectorIterator;

#endif

