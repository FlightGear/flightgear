/*****************************************************************************

 Module:       FGThunderstorm.cpp
 Author:       Christian Mayer
 Date started: 02.11.99

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
Function that calls other parts of fg when a lightning has happened

HISTORY
------------------------------------------------------------------------------
02.11.1999 Christian Mayer	Created
*****************************************************************************/

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#include <stdlib.h>
#include <time.h>

#include <sg.h>

#include "FGThunderstorm.h"

/****************************************************************************/
/********************************** CODE ************************************/
/****************************************************************************/
FGThunderstorm::FGThunderstorm(const float n, const float e, const float s, const float w, const float p, const unsigned int seed)
{
    northBorder = n;
    eastBorder  = e;
    southBorder = s;
    westBorder  = w;

    lightningProbability = p;
    currentProbability = 0.0;

    switch(seed)
    {
    case 0:
	//a seed of 0 means that I have to initialize the seed myself
	srand( (unsigned)time( NULL ) );
	break;

    case 1:
	//a seed of 1 means that I must not initialize the seed
	break;

    default:
	//any other seed means that I have to initialize with that seed
	srand( seed );
    }
}

FGThunderstorm::~FGThunderstorm(void)
{
    //I don't need to do anything
}

void FGThunderstorm::update(const float dt)
{
    //increase currentProbability by a value x that's 0.5 dt <= x <= 1.5 dt
    currentProbability += dt * ( 0.5 - (float(rand())/RAND_MAX) );

    if (currentProbability > lightningProbability)
    {	//ok, I've got a lightning now

	//figure out where the lightning is:
	sgVec2 lightningPosition;
	sgSetVec2( 
	    lightningPosition, 
	    southBorder + (northBorder - southBorder)*(float(rand())/RAND_MAX), 
	    westBorder  + (eastBorder  - westBorder )*(float(rand())/RAND_MAX)
	);

	//call OpenGl:
	/* ... */

	//call Sound:
	/* ... */

	//call Radio module:
	/* ... */

	currentProbability = 0.0;   //and begin again
    }
}




