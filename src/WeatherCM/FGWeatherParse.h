/*****************************************************************************

 Header:       FGWeatherParse.h	
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
Parse the weather that can be downloaded from 

    http://129.13.102.67/out/flight/yymmddhhdata.txt.gz

where yy stands for the year, mm for the month, dd for the day and hh for the
hour.
The columns are explained at

    http://129.13.102.67/out/flight/kopf.txt

and a list of the stations can be found at

    http://129.13.102.67/out/flight/wmoconv.txt.gz

Many thanks to Georg Mueller (Georg.Mueller@imk.fzk.de) of the 

    Institut fuer Meteorologie und Klimaforschung, Universitaet Karlsruhe

for makeking such a service aviable. 
You can also visit his homepage at http://www.wetterzentrale.de

HISTORY
------------------------------------------------------------------------------
18.10.1999 Christian Mayer	Created
14.12.1999 Christian Mayer	minor internal changes
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef FGWeatherParse_H
#define FGWeatherParse_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#include <Include/compiler.h>
#include <vector>

#include <Misc/fgstream.hxx>

#include "FGPhysicalProperties.h"

/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
FG_USING_STD(vector);

/****************************************************************************/
/* CLASS DECLARATION							    */
/****************************************************************************/
class FGWeatherParse
{
public:
    struct entry;

private:
    vector<entry> weather_station;

protected:
public:
    /************************************************************************/
    /* A line (i.e. entry) in the data file looks like:			    */
    /*  yyyy mm dd hh XXXXX BBBBB LLLLLL UUUUU VVVVV TTTTT DDDDD PPPPP pppp */
    /************************************************************************/
    struct entry
    {
	int year;			// The yyyy
	int month;			// The mm
	int day;			// The dd
	int hour;			// The hh
	unsigned int station_number;	// The XXXXX
	float lat;			// The BBBBBB	negative = south
	float lon;			// The LLLLLLL	negative = west	
	float x_wind;			// The UUUUU	negative = to the weat
	float y_wind;			// The VVVVV	negative = to the south
	float temperature;		// The TTTTT	in degC
	float dewpoint;			// The DDDDD	in degC
	float airpressure;		// The PPPPP	in hPa
	float airpressure_history[4];	// The pppp	in hPa
    };

    FGWeatherParse();
    ~FGWeatherParse();

    void input(const char *file);

    unsigned int stored_stations(void) const
    {
	return weather_station.size();
    }

    entry getEntry(const unsigned int nr) const
    {
	return weather_station[nr];
    }

    FGPhysicalProperties getFGPhysicalProperties(const unsigned int nr) const;
    void getPosition(const unsigned int nr, sgVec2 pos) const;
};

/****************************************************************************/
#endif /*FGWeatherParse_H*/
