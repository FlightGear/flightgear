/*****************************************************************************

 Module:       FGGlobalWeatherDatabase.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    main program

 ---------- Copyright (C) 1999  Christian Mayer (vader@t-online.de) ----------

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 Place - Suite 330, Boston, MA  02111-1307, USA.

 Further information about the GNU General Public License can also be found on
 the world wide web at http://www.gnu.org.

FUNCTIONAL DESCRIPTION
------------------------------------------------------------------------------
Database for the global weather
This database is only called by the local database and by the weather
simulator driving this database

HISTORY
------------------------------------------------------------------------------
28.05.1999 Christian Mayer	Created
16.06.1999 Durk Talsma		Portability for Linux
20.06.1999 Christian Mayer	added lots of consts
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGGlobalWeatherDatabase.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/

/****************************************************************************/
/* Interpolate p which lies inside the triangle x1, x2, x3		    */
/*									    */
/*  x3\			Do this by calulating q and linear interpolate its  */
/*   |\ \		value as it's laying between x1 and x2.		    */
/*   | \  \		Then interpolate p as it lays between p and x3	    */
/*   |  \   \								    */
/*   |   p    \		Advantages: p has exactly the value of a corner	    */
/*   |    \     \		    when it's laying on it.		    */
/*   |     \      \		    If p isn't in the triangle the algoritm */
/*  x1------q------x2		    extrapolates it's value		    */
/****************************************************************************/
template<class P, class V>
V triangle_interpolate(const P& x1, const V& v1, const P& x2, const V& v2, const P& x3, const V& v3, const P& p)
{
    P q;
    V q_value;
    
    q = x1 + (x2 - x1)*( ((x3-x1).x()*(x1-x2).y() - (x1-x2).x()*(x3-x1).y())/((p-x3).x()*(x2-x1).y() - (x2-x1).x()*(p-x3).y()) );
    
    q_value = v1 + (v2 - v1) * (x1.distance3D(q) / x1.distance3D(x2));
    
    return q_value + (v3 - q_value) * (q.distance3D(p) / q.distance3D(x3));
}

/****************************************************************************/
/* Constructor and Destructor						    */
/****************************************************************************/
FGGlobalWeatherDatabase::FGGlobalWeatherDatabase(const FGGlobalWeatherDatabaseStatus& s)
{
    DatabaseStatus = s;	
}

FGGlobalWeatherDatabase::~FGGlobalWeatherDatabase()
{
}

/****************************************************************************/
/* Get the physical properties on the specified point p			    */
/* do this by interpolating between the 3 closest points		    */
/****************************************************************************/
FGPhysicalProperties FGGlobalWeatherDatabase::get(const Point2D& p) const
{
    WeatherPrecition distance[3];		//store the 3 closest distances
    FGPhysicalProperties2DVectorConstIt iterator[3];	//and the coresponding iterators
    WeatherPrecition d;
    
    distance[0] = 9.46e15;	//init with a distance that every calculated
    distance[1] = 9.46e15;	//distance is guranteed to be shorter as
    distance[2] = 9.46e15;	//9.46e15 metres are 1 light year...
    
    for (FGPhysicalProperties2DVectorConstIt it=database.begin(); it!=database.end(); it++)
    {	//go through the whole database
	d = it->p.distance2Dsquared(p);
	
	if (d<distance[0])
	{
	    distance[2] = distance[1]; distance[1] = distance[0]; distance[0] = d;
	    iterator[2] = iterator[1]; iterator[1] = iterator[0]; iterator[0] = it;
	    //NOTE: The last line causes a warning that an unitialiced variable
	    //is used. You can ignore this warning here.
	}
	else if (d<distance[1])
	{
	    distance[2] = distance[1]; distance[1] = d;
	    iterator[2] = iterator[1]; iterator[1] = it;
	} 
	else if (d<distance[2])
	{
	    distance[2] = d;
	    iterator[2] = it;
	}
    }
    
    //now I've got the closest entry in xx[0], the 2nd closest in xx[1] and the
    //3rd in xx[2];
    
    //interpolate now:
    return triangle_interpolate(
	iterator[0]->p, (FGPhysicalProperties)*iterator[0],
	iterator[1]->p, (FGPhysicalProperties)*iterator[1],
	iterator[2]->p, (FGPhysicalProperties)*iterator[2], p);
}

/****************************************************************************/
/* update the database. Since the last call we had dt seconds		    */
/****************************************************************************/
void FGGlobalWeatherDatabase::update(const WeatherPrecition& dt)
{
    // I've got nothing to update here (yet...)
}

/****************************************************************************/
/* Add a physical property on the specified point p			    */
/****************************************************************************/
void FGGlobalWeatherDatabase::add(const Point2D& p, const FGPhysicalProperties& x)
{
    FGPhysicalProperties2D e;
    
    e.p = p;
    
    e.Wind = x.Wind;	
    e.Turbulence = x.Turbulence;	  
    e.Temperature = x.Temperature;	 
    e.AirPressure = x.AirPressure;	  
    e.VaporPressure = x.VaporPressure;
    
    e.Clouds = x.Clouds;		  
    e.SnowRainIntensity = x.SnowRainIntensity;    
    e.snowRainType = x.snowRainType;	    
    e.LightningProbability = x.LightningProbability;
    
    database.push_back(e);
}

/****************************************************************************/
/* Change the closest physical property to p. If p is further away than	    */
/* tolerance I'm returning false otherwise true				    */
/****************************************************************************/
bool FGGlobalWeatherDatabase::change(const FGPhysicalProperties2D& p, const WeatherPrecition& tolerance)
{
    for (FGPhysicalProperties2DVectorIt it = database.begin(); it != database.end(); it++)
    {
	if (it->p.distance3Dsquared(p.p) < (tolerance*tolerance))
	{   //assume that's my point
	    (*it) = p;
	    return true;
	}
    }
    
    return false;
}

/****************************************************************************/
/* Get all, but at least min, stored point in the circle around p with the  */
/* radius r								    */
/****************************************************************************/
FGPhysicalProperties2DVector FGGlobalWeatherDatabase::getAll(const Point2D& p, const WeatherPrecition& r, const unsigned int& min)
{
    FGPhysicalProperties2DVector ret_list;
    
    if ((DatabaseStatus == FGGlobalWeatherDatabase_only_static)
	||(DatabaseStatus == FGGlobalWeatherDatabase_working) )
    {	//doest it make sense?
	
	FGPhysicalProperties2DVectorIt *it;	    //store the closest entries
	WeatherPrecition *d;
	unsigned int act_it = 0;
	int i;
	
	it = new FGPhysicalProperties2DVectorIt[min+1];
	d = new WeatherPrecition[min+1];
	
	for (it[0]=database.begin(); it[act_it]!=database.end(); it[act_it]++)
	{	//go through the whole database
	    d[act_it] = it[act_it]->p.distance2Dsquared(p);
	    
	    if (r >= d[act_it])
	    {   //add it
		ret_list.push_back(*it[act_it]);
	    }
	    else
	    {
		if (act_it>0)
		{   //figure out if this distance belongs to the closest ones
		    WeatherPrecition dummy;
		    FGPhysicalProperties2DVectorIt dummyIt;
		    
		    for (i = act_it++; i >= 0;)
		    {
			if (d[i] >= d[--i])
			{
			    act_it--;
			    break;  //nope => stop
			}
			
			//swap both
			dummy =d[i]; d[i] = d[i+1]; d[i+1] = dummy;
			dummyIt = it[i]; it[i] = it[i+1]; it[i+1] = dummyIt;
		    }
		}
	    }
	}
	
	if (ret_list.size()<min)
	{
	    for(i = 0; (i < (min - ret_list.size())) && (ret_list.size() < database.size()); i++)
		ret_list.push_back(*it[i]);
	}
	
	delete d;
	delete it;
    }
    
    return ret_list;
}

