/*****************************************************************************

 Header:       FGVoronoi.h	
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
library for Voronoi Diagram calculation based on Steven Fortune 'Sweep2'
FGVoronoi is the wraper to feed the voronoi calulation with a vetor of points
and any class you want, as it uses templates
NOTE: Sweep2 didn't free *any* memory. So I'm doing what I can, but that's not
      good enough...

HISTORY
------------------------------------------------------------------------------
30.05.1999 Christian Mayer	Created
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
#ifndef FGVoronoi_H
#define FGVoronoi_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <vector>

#include "sg.h"

#include "FGWeatherVectorWrap.h"
#include "FGPhysicalProperties.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/
FG_USING_STD(vector);
FG_USING_NAMESPACE(std);

typedef vector<sgVec2Wrap>	Point2DList;

struct FGVoronoiInput
{
    sgVec2 position;
    FGPhysicalProperties2D value;

    FGVoronoiInput(const sgVec2& p, const FGPhysicalProperties2D& v) 
    { 
	sgCopyVec2(position, p); 
	value = v; 
    }
};

struct FGVoronoiOutput
{
    Point2DList boundary;
    FGPhysicalProperties2D value;

    FGVoronoiOutput(const Point2DList& b, const FGPhysicalProperties2D& v) 
    {
	boundary = b; 
	value = v;
    };
};

typedef vector<FGVoronoiInput>  FGVoronoiInputList;
typedef vector<FGVoronoiOutput> FGVoronoiOutputList;

/****************************************************************************/
/* FUNCTION DECLARATION							    */
/****************************************************************************/
FGVoronoiOutputList Voronoiate(const FGVoronoiInputList& input);

#endif /*FGVoronoi_H*/
