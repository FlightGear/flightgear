// network.cxx -- data structures for initializing & managing network.
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

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <GUI/gui.h>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#if defined ( __sun__ ) || defined ( __sgi )
extern "C" {
  extern void *memmove(void *, const void *, size_t);
}
#endif
*/

#include <Main/options.hxx>
#include <sg.h>
#include <ssg.h>

int  net_blast_toggle, net_hud_display, net_is_registered;
char *net_callsign, *FGFS_host;
sgMat4 sgFGD_VIEW;
ssgRoot *fgd_scene;

extern void list_init();
extern void fgd_init();
extern void fgd_send_com( char *FGD_com, char *FGFS_host);

char *fg_net_init( ssgRoot *orig_scene ){

 // We enable display of netinfos only if user wishes it via cmd-line param
 net_hud_display = (net_hud_display == 0) ? 0 : 1; 
 // Get pilot's name from options, can be modified at runtime via menu
 net_callsign = (char *) current_options.get_net_id().c_str();
 // Disable Blast Mode -1 = Disable, 0 = Enable  
 net_blast_toggle = -1;
 // We start unregistered, we reg. later via menu to fgd 
 net_is_registered = -1;
 fgd_scene = orig_scene;
 // Init list of Pilots
 list_init();
 // Init Sockets et al...
 fgd_init();
 // Register to deamon
 // fgd_send_com( "0", FGFS_host);
 // fgd_send_com( "1", FGFS_host);
 return("activated");
}
