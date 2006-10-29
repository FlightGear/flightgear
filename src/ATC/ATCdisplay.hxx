// ATCdisplay.hxx - class to manage the graphical display of ATC messages.
//                - The idea is to separate the display of ATC messages from their
//	          - generation so that the generation may come from any source.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef _FG_ATC_DISPLAY_HXX
#define _FG_ATC_DISPLAY_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <osg/State>
#include <simgear/structure/subsystem_mgr.hxx>

#include <vector>
#include <string>

SG_USING_STD(vector);
SG_USING_STD(string);

struct atcMessage {
    string msg;
    bool repeating;
    double counter;		// count of how many seconds since the message was registered
    double start_count;	// value of counter at which display should start (seconds)
    double stop_count;	// value of counter at which display should stop (seconds)
    int id;
	double dsp_offset;
};

// ASSUMPTION - with two radios the list won't be long so we don't need to map the id's
typedef vector<atcMessage> atcMessageList;
typedef atcMessageList::iterator atcMessageListIterator;

class FGATCDisplay : public SGSubsystem 
{

private:
    bool rep_msg;		// Flag to indicate there is a repeating transmission to display
    bool change_msg_flag;	// Flag to indicate that the repeating message has changed
    double dsp_offset1;		// Used to set the correct position of scrolling display
    double dsp_offset2;
    string rep_msg_str;		// The repeating transmission to play
    atcMessageList msgList;
    atcMessageListIterator msgList_itr;

public:
    FGATCDisplay();
    ~FGATCDisplay();

    void init();

    void bind();

    void unbind();

    // Display any registered messages
    void update(double dt, osg::State&);
    void update(double dt);

    // Register a single message for display after a delay of delay seconds
    // Will automatically stop displaying after a suitable interval.
    void RegisterSingleMessage(const string& msg, double delay = 0.0);

    // For now we will assume only one repeating message at once
    // This is not really robust

    // Register a continuously repeating message
    void RegisterRepeatingMessage(const string& msg);

    // Change a repeating message - assume that the message changes after the string has finished for now
    void ChangeRepeatingMessage(const string& newmsg); 

    // Cancel the current repeating message
    void CancelRepeatingMessage();
};

#endif // _FG_ATC_DISPLAY_HXX
