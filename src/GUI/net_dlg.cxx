// net_dlg.cxx -- data structures for initializing & managing network.
//
// Written by Oliver Delise, started May 1999.
//
// Copyleft (C) 1999  Oliver Delise - delise@rp-plus.de
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdlib.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Include/general.hxx>
#include <simgear/xgl/xgl.h>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <NetworkOLK/network.h>

#include "gui.h"
#include "net_dlg.hxx"

/// The beginnings of networking :-)
//  Needs cleaning up but works
//  These statics should disapear when this is a class
static puDialogBox     *NetIdDialog = 0;
static puFrame         *NetIdDialogFrame = 0;
static puText          *NetIdDialogMessage = 0;
static puInput         *NetIdDialogInput = 0;

static char NewNetId[16];
static char NewNetIdLabel[] = "Enter New Callsign"; 
extern char *fgd_callsign;

static puOneShot       *NetIdDialogOkButton = 0;
static puOneShot       *NetIdDialogCancelButton = 0;

void NetIdDialog_Cancel(puObject *)
{
	FG_POP_PUI_DIALOG( NetIdDialog );
}

void NetIdDialog_OK (puObject *)
{
	string NetId;

	bool freeze = globals->get_freeze();
	if(!freeze)
		globals->set_freeze( true );
/*  
   The following needs some cleanup because 
   "string options.NetId" and "char *net_callsign" 
*/
	NetIdDialogInput->getValue(&net_callsign);
	NetId = net_callsign;

	NetIdDialog_Cancel( NULL );
	fgSetString("/networking/call-sign", NetId.c_str() );
	strcpy( fgd_callsign, net_callsign);
//    strcpy( fgd_callsign, fgGetString("/sim/networking/call-sign").c_str());
/* Entering a callsign indicates : user wants Net HUD Info */
	net_hud_display = 1;

	if(!freeze)
		globals->set_freeze( false );
}

void NewCallSign(puObject *cb)
{
	sprintf( NewNetId, "%s", fgGetString("/sim/networking/call-sign").c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
	NetIdDialogInput->setValue( NewNetId );

	FG_PUSH_PUI_DIALOG( NetIdDialog );
}

void NewNetIdInit(void)
{
	sprintf( NewNetId, "%s", fgGetString("/sim/networking/call-sign").c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
	int len = 150 - puGetStringWidth( puGetDefaultLabelFont(),
									  NewNetIdLabel ) / 2;

	NetIdDialog = new puDialogBox (150, 50);
	{
		NetIdDialogFrame   = new puFrame           (0,0,350, 150);
		NetIdDialogMessage = new puText            (len, 110);
		NetIdDialogMessage ->    setLabel          (NewNetIdLabel);

		NetIdDialogInput   = new puInput           (50, 70, 300, 100);
		NetIdDialogInput   ->    setValue          (NewNetId);
		NetIdDialogInput   ->    acceptInput();

		NetIdDialogOkButton     =  new puOneShot   (50, 10, 110, 50);
		NetIdDialogOkButton     ->     setLegend   (gui_msg_OK);
		NetIdDialogOkButton     ->     setCallback (NetIdDialog_OK);
		NetIdDialogOkButton     ->     makeReturnDefault(TRUE);

		NetIdDialogCancelButton =  new puOneShot   (240, 10, 300, 50);
		NetIdDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
		NetIdDialogCancelButton ->     setCallback (NetIdDialog_Cancel);

	}
	FG_FINALIZE_PUI_DIALOG( NetIdDialog );
}


/*************** Deamon communication **********/

//  These statics should disapear when this is a class
static puDialogBox     *NetFGDDialog = 0;
static puFrame         *NetFGDDialogFrame = 0;
static puText          *NetFGDDialogMessage = 0;
//static puInput         *NetFGDDialogInput = 0;

//static char NewNetId[16];
static char NewNetFGDLabel[] = "Scan for deamon                        "; 
static char NewFGDHost[64] = "olk.mcp.de"; 
static int  NewFGDPortLo = 10000;
static int  NewFGDPortHi = 10001;

//extern char *fgd_callsign;
extern u_short base_port, end_port;
extern int fgd_ip, verbose, current_port;
extern char *fgd_host;


static puOneShot       *NetFGDDialogOkButton = 0;
static puOneShot       *NetFGDDialogCancelButton = 0;
static puOneShot       *NetFGDDialogScanButton = 0;

static puInput         *NetFGDHostDialogInput = 0;
static puInput         *NetFGDPortLoDialogInput = 0;
static puInput         *NetFGDPortHiDialogInput = 0;

void NetFGDDialog_Cancel(puObject *)
{
	FG_POP_PUI_DIALOG( NetFGDDialog );
}

void NetFGDDialog_OK (puObject *)
{
	char *NetFGD;    

	bool freeze = globals->get_freeze();
	if(!freeze)
		globals->set_freeze( true );
	NetFGDHostDialogInput->getValue( &NetFGD );
	strcpy( fgd_host, NetFGD);
	NetFGDPortLoDialogInput->getValue( (int *) &base_port );
	NetFGDPortHiDialogInput->getValue( (int *) &end_port );
	NetFGDDialog_Cancel( NULL );
	if(!freeze)
		globals->set_freeze( false );
}

void NetFGDDialog_SCAN (puObject *)
{
	char *NetFGD;
	int fgd_port;

	bool freeze = globals->get_freeze();
	if(!freeze)
		globals->set_freeze( true );
//    printf("Vor getvalue %s\n");
	NetFGDHostDialogInput->getValue( &NetFGD );
//    printf("Vor strcpy %s\n", (char *) NetFGD);
	strcpy( fgd_host, NetFGD);
	NetFGDPortLoDialogInput->getValue( (int *) &base_port );
	NetFGDPortHiDialogInput->getValue( (int *) &end_port );
	printf("FGD: %s  Port-Start: %d Port-End: %d\n", fgd_host, 
		   base_port, end_port);
	net_resolv_fgd(fgd_host);
	printf("Resolve : %d\n", net_r);
	if(!freeze)
		globals->set_freeze( false );
	if ( net_r == 0 ) {
		fgd_port = 10000;
		strcpy( fgd_name, "");
		for( current_port = base_port; ( current_port <= end_port); current_port++) {
			fgd_send_com("0" , FGFS_host);
			sprintf( NewNetFGDLabel , "Scanning for deamon Port: %d", current_port); 
			printf("FGD: searching %s\n", fgd_name);
			if ( strcmp( fgd_name, "") != 0 ) {
				sprintf( NewNetFGDLabel , "Found %s at Port: %d", 
						 fgd_name, current_port);
				fgd_port = current_port;
				current_port = end_port+1;
			}
		}
		current_port = end_port = base_port = fgd_port;
	}
	NetFGDDialog_Cancel( NULL );
}


void net_fgd_scan(puObject *cb)
{
	NewFGDPortLo = base_port;
	NewFGDPortHi = end_port;
	strcpy( NewFGDHost, fgd_host);
	NetFGDPortLoDialogInput->setValue( NewFGDPortLo );
	NetFGDPortHiDialogInput->setValue( NewFGDPortHi );
	NetFGDHostDialogInput->setValue( NewFGDHost );

	FG_PUSH_PUI_DIALOG( NetFGDDialog );
}


void NewNetFGDInit(void)
{
//    sprintf( NewNetId, "%s", fgGetString("/sim/networking/call-sign").c_str() );
//    sprintf( NewNetId, "%s", fgd_callsign );
	int len = 170 - puGetStringWidth( puGetDefaultLabelFont(),
									  NewNetFGDLabel ) / 2;

	NetFGDDialog = new puDialogBox (310, 30);
	{
		NetFGDDialogFrame   = new puFrame           (0,0,320, 170);
		NetFGDDialogMessage = new puText            (len, 140);
		NetFGDDialogMessage ->    setLabel          (NewNetFGDLabel);

		NetFGDPortLoDialogInput   = new puInput           (50, 70, 127, 100);
		NetFGDPortLoDialogInput   ->    setValue          (NewFGDPortLo);
		NetFGDPortLoDialogInput   ->    acceptInput();

		NetFGDPortHiDialogInput   = new puInput           (199, 70, 275, 100);
		NetFGDPortHiDialogInput   ->    setValue          (NewFGDPortHi);
		NetFGDPortHiDialogInput   ->    acceptInput();

		NetFGDHostDialogInput   = new puInput           (50, 100, 275, 130);
		NetFGDHostDialogInput   ->    setValue          (NewFGDHost);
		NetFGDHostDialogInput   ->    acceptInput();

		NetFGDDialogScanButton     =  new puOneShot   (130, 10, 200, 50);
		NetFGDDialogScanButton     ->     setLegend   ("Scan");
		NetFGDDialogScanButton     ->     setCallback (NetFGDDialog_SCAN);
		NetFGDDialogScanButton     ->     makeReturnDefault(FALSE);

		NetFGDDialogOkButton     =  new puOneShot   (50, 10, 120, 50);
		NetFGDDialogOkButton     ->     setLegend   (gui_msg_OK);
		NetFGDDialogOkButton     ->     setCallback (NetFGDDialog_OK);
		NetFGDDialogOkButton     ->     makeReturnDefault(TRUE);

		NetFGDDialogCancelButton =  new puOneShot   (210, 10, 280, 50);
		NetFGDDialogCancelButton ->     setLegend   (gui_msg_CANCEL);
		NetFGDDialogCancelButton ->     setCallback (NetFGDDialog_Cancel);

	}
	FG_FINALIZE_PUI_DIALOG( NetFGDDialog );
}

/*
static void net_display_toggle( puObject *cb)
{
	net_hud_display = (net_hud_display) ? 0 : 1;
        printf("Toggle net_hud_display : %d\n", net_hud_display);
}

*/

/***************  End Networking  **************/

