// network.cxx -- data structures for managing network.
//
// Written by Oliver Delise, started May 1999.
//
// Copyleft (C) 1999  Oliver Delise - delise@mail.isis.de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/*

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#include <GL/glut.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_VALUES_H
#  include <values.h>  // for MAXINT
#endif

#include <simgear/logstream.hxx>
#include <simgear/constants.h>
#include <simgear/fg_random.h>
#include <simgear/mat3.h>
#include <simgear/polar3d.hxx>

#include <Aircraft/aircraft.hxx>
#include <GUI/gui.h>
#include <Main/options.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
  extern void *memmove(void *, const void *, size_t);
}
#endif

*/

#include <Main/options.hxx>
#include <Cockpit/hud.hxx>
#include <NetworkOLK/network.h>

extern char *fgd_callsign;


void net_hud_update(){
 static char fgd_str[80];
 static float fgd_lon, fgd_lat, fgd_alt;
 int LinePos;
 
  fgd_lon = get_longitude();
  fgd_lat = get_latitude();
  fgd_alt = get_altitude();
//  sprintf(fgd_str,"Found %s %3.3f %3.3f", net_callsign, fgd_lat, fgd_lon);
//  HUD_TextList.add( fgText( 40, 18, fgd_str) );
  other = head->next;
  LinePos = 38;
  while ( other != tail) {
     if ( strcmp( other->ipadr, fgd_mcp_ip) != 0 ) {
        sprintf( fgd_str, "%-16s%-16s", other->callsign, other->ipadr);
        HUD_TextList.add( fgText( 40, LinePos, fgd_str) );
        LinePos += 13;
     }
     other = other->next;
  }
  sprintf(fgd_str,"%-16s%-16s", fgd_callsign, fgd_mcp_ip);
  HUD_TextList.add( fgText( 40, LinePos ,fgd_str) );
  sprintf(fgd_str,"Callsign        IP");
  HUD_TextList.add( fgText( 40, LinePos + 13 ,fgd_str) );
}
