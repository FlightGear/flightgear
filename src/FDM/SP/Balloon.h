/*****************************************************************************

 Header:       BalloonSimInterface.h	
 Author:       Christian Mayer
 Date started: 07.10.99

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
interface to the the hot air balloon simulator

HISTORY
------------------------------------------------------------------------------
07.10.1999 Christian Mayer	Created
*****************************************************************************/

/****************************************************************************/
/* SENTRY                                                                   */
/****************************************************************************/
#ifndef BalloonSimInterface_H
#define BalloonSimInterface_H

/****************************************************************************/
/* INCLUDES								    */
/****************************************************************************/

#include <FDM/flight.hxx>

#include "BalloonSim.h"
		
/****************************************************************************/
/* DEFINES								    */
/****************************************************************************/

/****************************************************************************/
/* DECLARATIONS  							    */
/****************************************************************************/


class FGBalloonSim: public FGInterface {

    balloon current_balloon;

public:

    FGBalloonSim( double dt );
    ~FGBalloonSim();

    // copy FDM state to BalloonSim structures
    bool copy_to_BalloonSim();

    // copy FDM state from BalloonSim structures
    bool copy_from_BalloonSim();

    // reset flight params to a specific position 
    void init();

    // update position based on inputs, positions, velocities, etc.
    void update( double dt );
};


/****************************************************************************/
#endif /*BalloonSimInterface_H*/




