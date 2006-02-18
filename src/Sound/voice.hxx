// speech synthesis interface subsystem
//
// Written by Melchior FRANZ, started February 2006.
//
// Copyright (C) 2006  Melchior FRANZ - mfranz@aon.at
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

#ifndef _VOICE_HXX
#define _VOICE_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <vector>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/io/sg_socket.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <Main/fg_props.hxx>

#if defined(ENABLE_THREADS)
#  include <simgear/threads/SGThread.hxx>
#  include <simgear/threads/SGQueue.hxx>
#else
#  include <queue>
   SG_USING_STD(queue);
#endif // ENABLE_THREADS

SG_USING_STD(vector);



class FGVoiceMgr : public SGSubsystem {
public:
	FGVoiceMgr();
	~FGVoiceMgr();
	void init(void);
	void update(double dt);

private:
	class FGVoice;

#if defined(ENABLE_THREADS)
	class FGVoiceThread;
	FGVoiceThread *_thread;
#endif

	string _host;
	string _port;
	bool _enabled;
	SGPropertyNode_ptr _pausedNode;
	bool _paused;
	vector<FGVoice *> _voices;
};



#if defined(ENABLE_THREADS)
class FGVoiceMgr::FGVoiceThread : public SGThread {
public:
	FGVoiceThread(FGVoiceMgr *mgr) : _mgr(mgr) {}
	void run();
	void wake_up() { _jobs.signal(); }

private:
	void wait_for_jobs() { SGGuard<SGMutex> g(_mutex); _jobs.wait(_mutex); }
	SGPthreadCond _jobs;
	SGMutex _mutex;
	FGVoiceMgr *_mgr;
};
#endif



class FGVoiceMgr::FGVoice {
public:
	FGVoice(FGVoiceMgr *, const SGPropertyNode_ptr);
	~FGVoice();
	bool speak();
	void update();
	void setVolume(double);
	void setPitch(double);
	void setSpeed(double);
	void pushMessage(string);

private:
	class FGVoiceListener;
	SGSocket *_sock;
	bool _connected;
	double _volume;
	double _pitch;
	double _speed;
	SGPropertyNode_ptr _volumeNode;
	SGPropertyNode_ptr _pitchNode;
	SGPropertyNode_ptr _speedNode;
	bool _festival;
	FGVoiceMgr *_mgr;

#if defined(ENABLE_THREADS)
	SGLockedQueue<string> _msg;
#else
	queue<string> _msg;
#endif

};



class FGVoiceMgr::FGVoice::FGVoiceListener : public SGPropertyChangeListener {
public:
	FGVoiceListener(FGVoice *voice) : _voice(voice) {}
	void valueChanged(SGPropertyNode *node);

private:
	FGVoice *_voice;
};


#endif // _VOICE_HXX
