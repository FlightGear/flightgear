/*****************************************************************************

 Header:       BalloonSim.h	
 Author:       Christian Mayer
 Date started: 01.09.99

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
Header for the hot air balloon simulator

HISTORY
------------------------------------------------------------------------------
01.09.1999 Christian Mayer	Created
03.10.1999 Christian Mayer	cleaned  the code  by moveing  WeatherDatabase 
				calls inside the update()
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef BalloonSim_H
#define BalloonSim_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <plib/sg.h>
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class balloon
{
private:
    float dt;				//in s

    sgVec3 gravity_vector;		//in m/s*s
    sgVec3 hpr;				//the balloon isn't allways exactly vertical (e.g. during gusts); normalized
    sgVec3 velocity;			//current velocity; it gets iterated at each 'update'
    sgVec3 position;			//current position in lat/lon/alt

    float balloon_envelope_area;	//area of the envelope
    float balloon_envelope_volume;	//volume of the envelope

    float wind_facing_area_of_balloon;
    float wind_facing_area_of_basket;

    float cw_envelope;			//wind resistance of the envelope
    float cw_basket;			//wind resistance of the bakset

    //all weights in kg
    float weight_of_total_fuel;
    float weight_of_envelope;
    float weight_of_basket;		//weight of all the unmovable stuff such as the basket, the burner and the empty tanks
    float weight_of_cargo;		//passengers and anything left (e.g. sand bags that are thrown away to give additional lift)

    float fuel_left;			//as a percentage
    float max_flow_of_fuel_per_second;	//in percent per second 
    float current_burner_strength;

    float lambda;			//waermeuebergangskoeffizient (heat transmission coefficent?!?) for the envelope
    float l_of_the_envelope;		//the thickness of the envelope (in m)

    float T;				//tempereature inside the balloon

    float ground_level;

public:
    balloon();				//constructor for initializing the balloon 

    void update();			//dt = time in seconds since last call
    void set_burner_strength(const float bs);

    void getVelocity(sgVec3 v) const;
    void setVelocity(const sgVec3 v);

    void getPosition(sgVec3 v) const;
    void setPosition(const sgVec3 v);

    void getHPR(sgVec3 angles) const;   //the balloon isn't allways exactly vertical
    void setHPR(const sgVec3 angles);   //the balloon isn't allways exactly vertical

    void setGroundLevel(const float altitude);

    float getTemperature(void) const;
    float getFuelLeft(void) const;

    void set_dt(const float new_dt) { dt = new_dt; }
};

/****************************************************************************/
#endif /*BalloonSim_H*/
