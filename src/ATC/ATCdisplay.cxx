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

#ifdef HAVE_WINDOWS_H
#   include <windows.h>
#endif

#include <simgear/compiler.h>

#include SG_GLU_H

#include <simgear/props/props.hxx>

#include <Include/general.hxx>
#include <Main/fg_props.hxx>
#include <GUI/gui.h>

#include "ATCdisplay.hxx"


// Constructor
FGATCDisplay::FGATCDisplay() {
	rep_msg = false;
	change_msg_flag = false;
	dsp_offset1 = 0.0;
	dsp_offset2 = 0.0;
}


// Destructor
FGATCDisplay::~FGATCDisplay() {
}

void FGATCDisplay::init() {
}

void FGATCDisplay::bind() {
}

void FGATCDisplay::unbind() {
}

// update - this actually draws the visuals and should be called from the main Flightgear rendering loop.
void FGATCDisplay::update(double dt) {
	
	// These strings are used for temporary storage of the transmission string in order
	// that the string we view only changes when the next repetition starts scrolling
	// even though the master string (rep_msg_str) may change at any time.
	static string msg1 = "";
	static string msg2 = "";
	
	if( rep_msg || msgList.size() ) {
		//cout << "In FGATC::update()" << endl;
		SGPropertyNode *xsize_node = fgGetNode("/sim/startup/xsize");
		SGPropertyNode *ysize_node = fgGetNode("/sim/startup/ysize");
		int iwidth   = xsize_node->getIntValue();
		int iheight  = ysize_node->getIntValue();
		
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
		
		float fps = general.get_frame_rate();
		
		if(rep_msg) {
			//cout << "dsp_offset1 = " << dsp_offset1 << " dsp_offset2 = " << dsp_offset2 << endl;
			if(dsp_offset1 == 0) {
				msg1 = rep_msg_str;
			}
			if(dsp_offset2 == 0) {
				msg2 = rep_msg_str;
			}
			// Check for the situation where one offset is negative and the message is changed
			if(change_msg_flag) {
				if(dsp_offset1 < 0) {
					msg1 = rep_msg_str;
				} else if(dsp_offset2 < 0) {
					msg2 = rep_msg_str;
				}
				change_msg_flag = false;
			}
			
			//	guiFnt.drawString( rep_msg_str.c_str(),
			//	    int(iwidth - guiFnt.getStringWidth(buf) - 10 - (int)dsp_offset),
			//	    (iheight - 20) );
			guiFnt.drawString( msg1.c_str(),
			int(iwidth - 10 - dsp_offset1),
			(iheight - 20) );
			guiFnt.drawString( msg2.c_str(),
			int(iwidth - 10 - dsp_offset2),
			(iheight - 20) );
			
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
		
		if(msgList.size()) {
			//cout << "Attempting to render single message\n";
			// We have at least one non-repeating message to process
			if(fgGetBool("/ATC/display/scroll-single-messages")) {	// Scroll single messages across the screen.
				msgList_itr = msgList.begin();
				int i = 0;
				while(msgList_itr != msgList.end()) {
					atcMessage m = *msgList_itr;
					//cout << "m.counter = " << m.counter << '\n';
					if(m.dsp_offset > (iwidth + (m.msg.size() * 10))) {
						//cout << "Stopping single message\n";
						msgList_itr = msgList.erase(msgList_itr);
					} else if(m.counter > m.start_count) {
						//cout << "Drawing single message\n";
						guiFnt.drawString( m.msg.c_str(),
						int(iwidth - 10 - m.dsp_offset),
						(iheight - 40) );
						m.counter += dt;
						m.dsp_offset += (80.0/fps);
						msgList[i] = m;
						++msgList_itr;
						++i;
					} else {
						//cout << "Not yet started single message\n";
						m.counter += dt;
						msgList[i] = m;
						++msgList_itr;
						++i;
					}
				}
			} else {	// Display single messages for a short period of time.
				msgList_itr = msgList.begin();
				int i = 0;
				while(msgList_itr != msgList.end()) {
					atcMessage m = *msgList_itr;
					//cout << "m.counter = " << m.counter << '\n';
					if(m.counter > m.stop_count) {
						//cout << "Stopping single message\n";
						msgList_itr = msgList.erase(msgList_itr);
					} else if(m.counter > m.start_count) {
						int pin = (((int)m.msg.size() * 8) >= iwidth ? 5 : (iwidth - (m.msg.size() * 8))/2);
						//cout << m.msg << '\n';
						//cout << "pin = " << pin << ", iwidth = " << iwidth << ", msg.size = " << m.msg.size() << '\n';
						guiFnt.drawString( m.msg.c_str(), pin, (iheight - 40) );
						m.counter += dt;
						msgList[i] = m;
						++msgList_itr;
						++i;
					} else {
						m.counter += dt;
						msgList[i] = m;
						++msgList_itr;
						++i;
					}
				}
			}
		}
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_LIGHTING );
		glMatrixMode( GL_PROJECTION );
		glPopMatrix();
		glMatrixMode( GL_MODELVIEW );
		glPopMatrix();
	}
}

void FGATCDisplay::RegisterSingleMessage(string msg, double delay) {
	//cout << msg << '\n';
	atcMessage m;
	m.msg = msg;
	m.repeating = false;
	m.counter = 0.0;
	m.start_count = delay;
	m.stop_count = m.start_count + 5.0;		// Display for 5ish seconds for now - this might have to change eg. be related to length of message in future
	//cout << "m.stop_count = " << m.stop_count << '\n';
	m.id = 0;
	m.dsp_offset = 0.0;
	
	msgList.push_back(m);
	//cout << "Single message registered\n";
}

void FGATCDisplay::RegisterRepeatingMessage(string msg) {
	rep_msg = true;
	rep_msg_str = msg;
	return;
}

void FGATCDisplay::ChangeRepeatingMessage(string newmsg) {
	rep_msg_str = newmsg;
	change_msg_flag = true;
	return;
}

void FGATCDisplay::CancelRepeatingMessage() {
	rep_msg = false;
	rep_msg_str = "";
	dsp_offset1 = 0;
	dsp_offset2 = 0;
	return;
}

