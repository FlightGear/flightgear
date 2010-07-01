/*****************************************************************************

 Module:       BalloonSim.cpp
 Author:       Christian Mayer
 Date started: 01.09.99
 Called by:

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
A hot air balloon simulator

HISTORY
------------------------------------------------------------------------------
01.09.1999 Christian Mayer	Created
03.10.1999 Christian Mayer	cleaned  the code  by moveing  WeatherDatabase 
				calls inside the update()
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <math.h>

#include <simgear/constants.h>

#include "BalloonSim.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/

/****************************************************************************/
/*									    */
/*  Constructor:							    */
/*  Set the balloon model to some values that seem reasonable as I haven't  */
/*  got much original values						    */
/*									    */
/****************************************************************************/
balloon::balloon()
{
    dt = 0.1;
    ground_level = 3400.0;

    gravity_vector = SGVec3f(0.0, 0.0, -9.81);
    velocity = SGVec3f(0.0, 0.0, 0.0);
    position = SGVec3f(0.0, 0.0, 0.0);
    hpr = SGVec3f(0.0, 0.0, 0.0);

    /************************************************************************/
    /* My balloon  has a  radius of  8.8 metres  as that  gives a  envelope */
    /* volume of about 2854 m^3  which is  about 77000 ft^3,  a very common */
    /* size for hot air balloons					    */
    /************************************************************************/

    balloon_envelope_area = 4.0 * (8.8 * 8.8) * SGD_PI; 
    balloon_envelope_volume = (4.0/3.0) * (8.8 * 8.8 * 8.8) * SGD_PI;

    wind_facing_area_of_balloon = SGD_PI * (8.8 * 8.8);
    wind_facing_area_of_basket = 2.0;	//guessed: 2 m^2
    
    cw_envelope=0.45;			//a sphere in this case
    cw_basket=0.8;

    weight_of_total_fuel = 40.0;	//big guess
    weight_of_envelope = 200.0;		//big guess
    weight_of_basket = 40.0;		//big guess
    weight_of_cargo = 750.0;		//big guess

    fuel_left=1.0;
    max_flow_of_fuel_per_second=10.0*1.0/3600.0; //assuming 10% of one hour of total burn time
    current_burner_strength = 0.0;	//the throttle

    lambda = 0.15;			//for plasic
    l_of_the_envelope = 1.0/1000.0;	//the thickness of the envelope (in m): 1mm

    T = 273.16 + 130.6;			//Temperature in the envelope => still at ground level
}

void balloon::update()
{
    /************************************************************************/
    /* I'm  simplifying  the  balloon by  reducing the  simulation  to  two */
    /* points:                                                              */	
    /* the center of the basket (CB) and the center of the envelope (CE)    */
    /*                                                                      */
    /*                                 ce                                   */
    /*                                 I                                    */
    /*                                 I                                    */
    /*                                 cg (=center of gravity)              */
    /*                                 I                                    */
    /*                                 cb                                   */
    /*                                                                      */
    /* On each center  are forces acting:  gravity and  wind resitance.  CE */
    /* additionally got the lift  (=> I need to calculate the weight of the */
    /* air inside, too)							    */
    /*                                                                      */
    /* The weight of the air  in the envelope is  dependant of the tempera- */
    /* ture. This temperature is decreasing over the time that is dependant */
    /* of the insulation  of the envelope  material (lambda),  the gas used */
    /* (air) and the wind speed. For a plane surface it's for air:	    */
    /*                                                                      */
    /*	  alpha = 4.8 + 3.4*v	with v < 5.0 m/s			    */
    /*                                                                      */
    /* The value k that takes all of that into account is defined as:	    */
    /*                                                                      */
    /*	  1 / k = 1 / alpha1 + 1 / alpha2 + l / lambda			    */
    /*                                                                      */
    /* with 'l' beeing the 'length' i.e. thickness of the insulator, alpha1 */
    /* the air inside and alpha2 the air outside of the envelope.  So our k */
    /* is:                                                                  */
    /*                                                                      */
    /*    k = 1 / [1/4.8 + 1/(4.8+3.4v) + l/lambda]			    */
    /*                                                                      */
    /* The energy lost by this process is defined as:			    */
    /*                                                                      */
    /*    dQ = k * A * t * dT						    */
    /*                                                                      */
    /* with Q being  the energy,  k that value  defined above,  A the total */
    /* area of the envelope,  t the time (in hours)  and dT the temperature */
    /* difference between the inside and the outside.			    */
    /* To get the temperature of the air in the inside I need the formula:  */
    /*                                                                      */
    /*    dQ = cAir * m * dT						    */
    /*                                                                      */
    /* with cAir being the specific heat capacity(?)  of air (= 1.00 kcal / */
    /* kg * degree),  m the mass of the air and  dT the temperature change. */
    /* As the envelope is open I'm  assuming that the  same air pressure is */
    /* inside and  outside of  it  (practical there  should  be  a slightly */
    /* higher air pressure  in the inside or the envelope  would collapse). */
    /* So it's easy to calculate the density of the air inside:             */
    /*                                                                      */
    /*    rho = p / R * T						    */
    /*                                                                      */
    /* with p being  the pressure,  R the gas constant(?)  which is for air */
    /* 287.14 N * m / kg * K and T the absolute temperature.		    */
    /*                                                                      */
    /* The value returned by this function is the displacement of the CB    */
    /************************************************************************/

    /************************************************************************/
    /* NOTE:  This is the simplified version:  I'm assuming that  the whole */
    /* balloon consists only of the envelope.I will improove the simulation */
    /* later, but currently was my main concern to get it going...          */
    /************************************************************************/

   // I realy don't think there is a solution for this without WeatherCM
   // but this is a hack, and it's working -- EMH
   double mAir = 1;
   float Q = 0;

    // gain of energy by heating:
    if (fuel_left > 0.0)	//but only with some fuel left ;-)
    {
	float fuel_burning = current_burner_strength * max_flow_of_fuel_per_second * dt * weight_of_total_fuel;	//in kg
    
	//convert to cubemetres (I'm wrongly assuming 'normal' conditions; but that's correct for my special case)
	float cube_metres_burned = fuel_burning / 2.2;  //2.2 is the density for propane

	fuel_left -= fuel_burning / weight_of_total_fuel;

	// get energy through burning. 
	Q += 22250.0 * cube_metres_burned;  //22250 for propan, 29500 would be butane and if you dare: 2580 would be hydrogen...
    }

    // calculate the new temperature in the inside:
    T += Q / (1.00 * mAir);

    //calculate the masses of the envelope and the basket
    float mEnvelope = mAir + weight_of_envelope;
    float mBasket   = weight_of_total_fuel*fuel_left + weight_of_basket + weight_of_cargo;

    float mTotal = mEnvelope + mBasket;

    //calulate the forces
    SGVec3f fTotal, fFriction, fLift;

    fTotal = mTotal*gravity_vector;
   
    // fTotal += fLift;     //FIXME: uninitialized fLift 
    // fTotal += fFriction; //FIXME: uninitialized fFriction
    
    //claculate acceleration: a = F / m
    SGVec3f aTotal, vTotal, dTotal;

    aTotal = (1.0 / mTotal)*fTotal;

    //integrate the displacement: d = 0.5 * a * dt**2 + v * dt + d
    vTotal = dt*velocity; 
    dTotal = (0.5*dt*dt)*aTotal; dTotal += vTotal;

    //integrate the velocity to 'velocity': v = a * dt + v
    vTotal = dt*aTotal; velocity += vTotal;

    /************************************************************************/
    /* VERY WRONG STUFF: it's just here to get some results to start with   */	
    /************************************************************************/

    // care for the ground
    if (position[2] < (ground_level+0.001) )
	position[2] = ground_level;

    //return results
    position += dTotal;

    //cout << "BallonSim: T: " << (T-273.16) << " alt: " << position[2] << " ground: " << ground_level << " throttle: " << current_burner_strength << "\n";
}

void balloon::set_burner_strength(const float bs)
{
    if ((bs>=0.0) && (bs<=1.0))
    current_burner_strength = bs;
}

void balloon::getVelocity(SGVec3f& v) const
{
    v = velocity;
}

void balloon::setVelocity(const SGVec3f& v)
{
    velocity = v;
}

void balloon::getPosition(SGVec3f& v) const
{
    v = position;
}

void balloon::setPosition(const SGVec3f& v)
{
    position = v;
}

void balloon::getHPR(SGVec3f& angles) const //the balloon isn't allways exactly vertical
{
    angles = hpr;
}

void balloon::setHPR(const SGVec3f& angles)  //the balloon isn't allways exactly vertical
{
    hpr = angles;
}

void balloon::setGroundLevel(const float altitude)
{
    ground_level = altitude;
}

float balloon::getTemperature(void) const
{
    return T;
}

float balloon::getFuelLeft(void) const
{
    return fuel_left;
}

/*
void main(void)
{
    bool burner = true;
    balloon bal;
    bool exit = false;
    sgVec3 pos={0.0, 0.0, 0.0};
    char c;
    float acc_dt = 0.0;
    float alt;

bool hysteresis = false; // moving up
    for (;!exit;)
    {
	for (int i=0; i<100; i++)
	{
	    bal.update(0.1); acc_dt += 0.1;
	    bal.getPosition(pos);
	    alt = pos[2];

	    if (alt > 3010) 
	    {
		hysteresis = true;
		burner = false;
	    }
	    if ((alt < 2990) && (hysteresis == true))
	    {
		hysteresis = false;
		burner = true;
	    }
	    if ((bal.getTemperature()-273.16)>250.0)
		burner = false; //emergency
	}

	// toogle burner
	c = getch();
	if (c==' ')
	    burner=!burner;
	//if (c=='s')
	//    show=!show;

	//printf("Position: (%f/%f/%f), dP: (%f/%f/%f), burner: ", pos[0], pos[1], pos[2], dp[0], dp[1], dp[2]);
	printf("%f         \t%f         \t%f         \t%f\n", acc_dt/60.0, bal.getTemperature()-273.16, pos[2], bal.getFuelLeft());
	if (burner==true)
	{
	    //printf("on\n");
	    bal.set_burner_strength(1.0);
	}
	else
	{
	    //printf("off\n");
	    bal.set_burner_strength(0.0);
	}

    }
}
*/
