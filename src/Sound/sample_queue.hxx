// sample_queue.hxx -- sample queue management class
//
// Started by David Megginson, October 2001
// (Reuses some code from main.cxx, probably by Curtis Olson)
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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
//
// $Id$

#ifndef __FGSAMPLE_QUEUE_HXX
#define __FGSAMPLE_QUEUE_HXX 1

#include <simgear/compiler.h>

#include <queue>

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/props.hxx>
#include <simgear/sound/sample_group.hxx>

class SGSoundSample;

/**
 * FlightGear sample queue class
 *
 *    This modules maintains a queue of 'message' audio files.  These
 *    are played sequentially with no overlap until the queue is finished.
 *    This second mechanisms is useful for things like tutorial messages or
 *    background ATC chatter.
 */
class FGSampleQueue : public SGSampleGroup
{

public:

    FGSampleQueue ( SGSoundMgr *smgr, const string &refname );
    virtual ~FGSampleQueue ();

    virtual void update (double dt);

    inline void add (SGSharedPtr<SGSoundSample> msg) { _messages.push(msg); }

private:

    std::queue< SGSharedPtr<SGSoundSample> > _messages;

    bool last_enabled;
    double last_volume;

    SGPropertyNode_ptr _enabled;
    SGPropertyNode_ptr _volume;
};


#endif

// end of fg_fx.hxx
