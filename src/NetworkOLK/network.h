// network.h -- public data structures for managing network.
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

#ifndef NETWORK_H
#define NETWORK_H

#define FGD

#include <plib/ssg.h>

extern char *net_callsign;
extern int  net_hud_display;
extern int  net_blast_toggle;
extern int  net_is_registered;
extern int  net_r, current_port;
extern u_short base_port, end_port;
extern char *fg_net_init( ssgRoot *orig_scene );
extern char *FGFS_host, *fgd_mcp_ip, *fgd_name;

extern void net_hud_update( void );
extern int  net_resolv_fgd( char *);

#include "fgd.h"

extern sgMat4 sgFGD_VIEW;

struct list_ele {
   /* unsigned */ char ipadr[16], callsign[16];
   /* unsigned */ char lon[8], lat[8], alt[8], roll[8], pitch[8], yaw[8];
   float lonf, latf, altf, speedf, rollf, pitchf, yawf;
   sgMat4 sgFGD_COORD;
   ssgSelector  *fgd_sel;
   ssgTransform * fgd_pos;   
   struct list_ele *next, *prev;
};

extern struct list_ele *head, *tail, *other;

extern void fgd_send_com( char *FGD_com, char *FGFS_host);
extern void list_init( void );
extern void fgd_init( void);
#endif
