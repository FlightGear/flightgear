/*****************************************************************************

 Module:       test.cpp
 Author:       Christian Mayer
 Date started: 28.05.99
 Called by:    command line

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
Test program for the weather database

HISTORY
------------------------------------------------------------------------------
28.05.99   CM   Created
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include "FGLocalWeatherDatabase.h"
#include "FGGlobalWeatherDatabase.h"
#include "LibVoronoi/FGVoronoi.h"
#include "FGWeatherUtils.h"
#include <time.h>
#include STL_IOSTREAM
#include <vector>

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
int main(void)
{
    //init local database:

    FGLocalWeatherDatabase local(Point3D(0,0,0), 10000, FGLocalWeatherDatabase::use_global);
    FGGlobalWeatherDatabase global(FGGlobalWeatherDatabase_working);

    Point3D p(0);
    Point2D p2d(0);
    FGPhysicalProperties x;
    FGPhysicalProperties2D x2d;
    FGPhysicalProperties2DVector getAllvect;

    cout << "\n**************** FGGlobalWeatherDatabase Test ****************\n";
    x.Temperature.insert(FGTemperatureItem(100, Celsius(10)));
    global.add(Point2D(5,5), x);

    x.Temperature.insert(FGTemperatureItem(110, Celsius(-10)));
    global.add(Point3D(10,10,10), x);

    x2d.Temperature.insert(FGTemperatureItem(90, Celsius(-20)));
    x2d.p = Point2D(10,5);

    global.add(x2d);

    getAllvect = global.getAll(p2d, 10000);
    cout << "Returned size: " << getAllvect.size() << "\n";
    getAllvect = global.getAll(p2d, 10000, 30);
    cout << "Returned size: " << getAllvect.size() << "\n";
    getAllvect = global.getAll(p, 10000);
    cout << "Returned size: " << getAllvect.size() << "\n";
    getAllvect = global.getAll(p, 10000, 3);
    cout << "Returned size: " << getAllvect.size() << "\n";

    cout << "Temperature: " << global.get(Point3D(5, 5, 100)).Temperature << "°C\n";
    cout << "AirPressure at    0m: " << global.get(Point3D(5, 5,    0)).AirPressure << "\n";
    cout << "AirPressure at 1000m: " << global.get(Point3D(5, 5, 1000)).AirPressure << "\n";

    cout << global;


    cout << "\n**************** FGMicroWeather Test ****************\n";
    vector<Point2D> points;

    points.push_back(Point2D(0.1, 0.1));
    points.push_back(Point2D(0.9, 0.1));
    points.push_back(Point2D(0.9, 0.9));
    points.push_back(Point2D(0.1, 0.9));
    points.push_back(Point2D(0.1, 0.1));

    x2d.p = Point2D(0.4, 0.4);

    FGMicroWeather micro(x2d, points);

    cout << "hasPoint 0.5, 0.5: ";
    if (micro.hasPoint(Point2D(0.5, 0.5)) == true)
	cout << "true";
    else
	cout << "false";

    cout << "\nhasPoint 0.9, 0.5: ";
    if (micro.hasPoint(Point2D(0.9, 0.5)) == true)
	cout << "true";
    else
	cout << "false";

    cout << "\nhasPoint 1.5, 0.5: ";
    if (micro.hasPoint(Point2D(1.5, 0.5)) == true)
	cout << "true";
    else
	cout << "false";

    cout << "\n";


    cout << "\n**************** Voronoi Diagram Test ****************\n";
    FGVoronoiInputList input;
    FGVoronoiOutputList output;
    FGVoronoiInput test(Point2D(0.0), FGPhysicalProperties2D());

    test.position = Point2D(0.1,0.2);
    input.push_back(test);

    test.position = Point2D(0.8,0.9);
    input.push_back(test);

    test.position = Point2D(0.9,0.1);
    input.push_back(test);

    test.position = Point2D(0.6,0.4);
    input.push_back(test);

    test.position = Point2D(1.1,1.2);
    input.push_back(test);

    test.position = Point2D(1.8,1.9);
    input.push_back(test);

    test.position = Point2D(1.9,1.1);
    input.push_back(test);

    test.position = Point2D(1.6,1.4);
    input.push_back(test);

    test.position = Point2D(2.9,2.1);
    input.push_back(test);

    test.position = Point2D(2.6,2.4);
    input.push_back(test);

    output = Voronoiate(input);

    cout << "\n";
    for (FGVoronoiOutputList::iterator it=output.begin(); it!=output.end(); it++)
    {
	cout << "Cell start: ";
	for (Point2DList::iterator it2= it->boundary.begin();it2!= it->boundary.end();it2++)
	{
	    if (it2==it->boundary.begin())
		cout << "(";
	    else
		cout << "-(";
	    
	    cout << *it2 << ")";
	}
	cout << "\n";
    }

    cout << "\n**************** Database Stress Test ****************\n";

    time_t starttime, currenttime;
    unsigned long count = 0;
    unsigned long count2 = 0;
    float xxx, yyy;
    cout << "Filling Database... ";
    time( &starttime );
    for (count = 0; count < 5000; count++)
    {
	xxx = (rand()%36000)/100.0;
	yyy = (rand()%18000)/100.0;

	local.addProperties(FGPhysicalProperties2D(FGPhysicalProperties(), Point2D(xxx, yyy)));
    }
    local.addProperties(FGPhysicalProperties2D(FGPhysicalProperties(), Point3D(-10.0)));
    local.addProperties(FGPhysicalProperties2D(FGPhysicalProperties(), Point3D(+10.0)));
    local.addProperties(FGPhysicalProperties2D(FGPhysicalProperties(), Point3D(-100.0)));
    local.addProperties(FGPhysicalProperties2D(FGPhysicalProperties(), Point3D(+100.0)));

    time( &currenttime );
    cout << float(count)/float(currenttime - starttime) << "/s filling rate; ";
    time( &starttime );

    for (count = 0; count < 5; count++)
    {
	local.reset(FGLocalWeatherDatabase::use_global);	//make sure I've got a current voronoi
    }

    time( &currenttime );
    cout << float(currenttime - starttime)/float(count) << "s resetting time; Done\n";
    count = 0;

    //for (;count<200;)
    cout << "local.get() test: 10 seconds\n";
    time( &starttime );
    time( &currenttime );
    for (;currenttime<(starttime+10);)
    {
	time( &currenttime );
	count++;
	local.get(Point3D(0.0));
    }

    cout << "Result: " << float(count) / float(currenttime-starttime) << "/s\n";

    count = 0;
    cout << "output = Voronoiate(input) test: 10 seconds\n";
    time( &starttime );
    time( &currenttime );
    for (;currenttime<(starttime+10);)
    {
	time( &currenttime );
	count++;
	output = Voronoiate(input);
    }

    cout << "Result: " << float(count) / float(currenttime-starttime) << "/s\n";
    cout << "Reference: 176800/s\n";

    cout << "\n**************** Database Stress Test end ****************\n";
    return 0;
}















