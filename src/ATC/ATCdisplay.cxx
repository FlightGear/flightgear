// ATCdisplay.cxx - routines to display ATC output - graphically for now
//
// Written by David Luff, started October 2001.
//
// Copyright (C) 2001  David C Luff - david.luff@nottingham.ac.uk
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/misc/props.hxx>

#include <Include/general.hxx>
#include <Main/fg_props.hxx>
#include <GUI/gui.h>

#include "ATCdisplay.hxx"


FGATCDisplay *current_atcdisplay;


// Constructor
FGATCDisplay::FGATCDisplay( void ) {
    rep_msg = false;
    dsp_offset1 = 0;
    dsp_offset2 = 0;
}


// Destructor
FGATCDisplay::~FGATCDisplay( void ) {
}

void FGATCDisplay::init( void ) {
}


// update - this actually draws the visuals and should be called from the main Flightgear rendering loop.
void FGATCDisplay::update() {

    if(rep_msg) {
	float fps = general.get_frame_rate();
	
	//cout << "In FGATC::update()" << endl;
	SGPropertyNode *xsize_node = fgGetNode("/sim/startup/xsize");
	SGPropertyNode *ysize_node = fgGetNode("/sim/startup/ysize");
	int iwidth   = xsize_node->getIntValue();
	int iheight  = ysize_node->getIntValue();
	
	//TODO - if the string is bigger than the buffer the program exits - we really ought to have a robust check here
	char buf[256];
	//float fps = visibility/1600;
	//      sprintf(buf,"%-4.1f  %7.0f  %7.0f", fps, tris, culled);
//	sprintf(buf,"%s %-5.1f", "visibility ", visibility);
	sprintf(buf,"%s", rep_msg_str.c_str());
	
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D( 0, iwidth, 0, iheight );
	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glLoadIdentity();
	
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_LIGHTING );
	
	glColor3f( 0.9, 0.4, 0.2 );
	
//	guiFnt.drawString( buf,
//	    int(iwidth - guiFnt.getStringWidth(buf) - 10 - (int)dsp_offset),
//	    (iheight - 20) );
	guiFnt.drawString( buf,
	    int(iwidth - 10 - dsp_offset1),
	    (iheight - 20) );
    	guiFnt.drawString( buf,
	    int(iwidth - 10 - dsp_offset2),
	    (iheight - 20) );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_LIGHTING );
	glMatrixMode( GL_PROJECTION );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
	glPopMatrix();
	
	// Try to scroll at a frame rate independent speed
	// 40 pixels/second looks about right for now
	if(dsp_offset1 >= dsp_offset2) {
	    dsp_offset1+=(40.0/fps);
	    dsp_offset2 = dsp_offset1 - (rep_msg_str.size() * 10) - 100;
	    if(dsp_offset1 > (iwidth + (rep_msg_str.size() * 10)))
		dsp_offset1 = 0;
	} else {
	    dsp_offset2+=(40.0/fps);
	    dsp_offset1 = dsp_offset2 - (rep_msg_str.size() * 10) - 100;
	    if(dsp_offset2 > (iwidth + (rep_msg_str.size() * 10)))
		dsp_offset2 = 0;
	}
	
    }
}


void FGATCDisplay::RegisterRepeatingMessage(string msg) {
    rep_msg = true;
    rep_msg_str = msg;
    return;
}

void FGATCDisplay::ChangeRepeatingMessage(string newmsg) {
    //Not implemented yet
    return;
}

void FGATCDisplay::CancelRepeatingMessage() {
    rep_msg = false;
    rep_msg_str = "";
    dsp_offset1 = 0;
    dsp_offset2 = 0;
    return;
}
