/*****************************************************************************

 Header:       FGGlobalWeatherDatabase.h	
 Author:       Christian Mayer
 Date started: 28.05.99

 -------- Copyright (C) 1999 Christian Mayer (fgfs@christianmayer.de) --------

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
30.06.1999 Christian Mayer	STL portability
11.10.1999 Christian Mayer	changed set<> to map<> on Bernie Bright's 
				suggestion
19.10.1999 Christian Mayer	change to use PLIB's sg instead of Point[2/3]D
				and lots of wee code cleaning
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGGlobalWeatherDatabase_H
#define FGGlobalWeatherDatabase_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#include <vector>
#include STL_IOSTREAM

#include <sg.h>

#include "FGPhysicalProperties.h"
#include "FGPhysicalProperty.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
FG_USING_STD(vector);
#ifndef FG_HAVE_NATIVE_SGI_COMPILERS
FG_USING_STD(iostream);
#endif
FG_USING_NAMESPACE(std);

enum FGGlobalWeatherDatabaseStatus {
    FGGlobalWeatherDatabase_not_used,
    FGGlobalWeatherDatabase_switched_off,
    FGGlobalWeatherDatabase_only_static,
    FGGlobalWeatherDatabase_working
};

class FGGlobalWeatherDatabase;
ostream& operator<< ( ostream& out, const FGGlobalWeatherDatabase& p );

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGGlobalWeatherDatabase
{
private:
protected:
    FGGlobalWeatherDatabaseStatus DatabaseStatus;
    FGPhysicalProperties2DVector database;

public:
    /************************************************************************/
    /* Constructor and Destructor					    */
    /************************************************************************/
    FGGlobalWeatherDatabase(const FGGlobalWeatherDatabaseStatus s = FGGlobalWeatherDatabase_not_used);
    ~FGGlobalWeatherDatabase();

    /************************************************************************/
    /* Get the physical properties on the specified point p		    */
    /************************************************************************/
    FGPhysicalProperties get(const sgVec2& p) const; 
    FGPhysicalProperty   get(const sgVec3& p) const 
    {
	sgVec2 temp;
	sgSetVec2( temp, p[0], p[1] );

	return FGPhysicalProperty( get(temp), p[3] );
    }

    /************************************************************************/
    /* update the database. Since the last call we had dt seconds	    */
    /************************************************************************/
    void update(const WeatherPrecision dt);

    /************************************************************************/
    /* Add a physical property on the specified point p			    */
    /************************************************************************/
    void add(const sgVec2& p, const FGPhysicalProperties& x);
    void add(const FGPhysicalProperties2D& x) 
    {
	database.push_back(x);
    }

    /************************************************************************/
    /* Change the closest physical property to p. If p is further away than */
    /* tolerance I'm returning false otherwise true			    */
    /************************************************************************/
    bool change(const FGPhysicalProperties2D& p, const WeatherPrecision tolerance = 0.0000001);

    /************************************************************************/
    /* Get all stored points in the circle around p with the radius r, but  */
    /* at least min points.						    */
    /************************************************************************/
    FGPhysicalProperties2DVector getAll(const sgVec2& p, const WeatherPrecision r, const unsigned int min = 0);
    FGPhysicalProperties2DVector getAll(const sgVec3& p, const WeatherPrecision r, const unsigned int min = 0)
    {
	sgVec2 temp;
	sgSetVec2(temp, p[0], p[1]);

	return getAll(temp, r, min);
    }

    /************************************************************************/
    /* get/set the operating status of the database			    */
    /************************************************************************/
    FGGlobalWeatherDatabaseStatus getDatabaseStatus(void) const    { return DatabaseStatus; }
    void setDatabaseStatus(const FGGlobalWeatherDatabaseStatus& s) { DatabaseStatus = s; }

    /************************************************************************/
    /* Dump the whole database						    */
    /************************************************************************/
    friend ostream& operator<< ( ostream& out, const FGGlobalWeatherDatabase& p );

};

inline ostream& operator<< ( ostream& out, const FGGlobalWeatherDatabase& p )
{
    //out << "Database status: " << DatabaseStatus << "\n";
    out << "Database number of entries: " << p.database.size() << "\n";

    for (FGPhysicalProperties2DVector::const_iterator it = p.database.begin(); it != p.database.end(); it++)
	out << "Next entry: " << *it;

    return out;
}

/****************************************************************************/
#endif /*FGGlobalWeatherDatabase_H*/
