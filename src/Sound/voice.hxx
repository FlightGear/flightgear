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
#  include <OpenThreads/Thread>
#  include <OpenThreads/Mutex>
#  include <OpenThreads/ScopedLock>
#  include <OpenThreads/Condition>
#  include <simgear/threads/SGQueue.hxx>
#else
#  include <queue>
#endif // ENABLE_THREADS

using std::vector;



class FGVoiceMgr : public SGSubsystem {
public:
	FGVoiceMgr();
	~FGVoiceMgr();
	void init(void);
	void shutdown();
	void update(double dt);


  class FGVoice;
protected:
  friend class FGFestivalVoice;
#if defined(ENABLE_THREADS)
	class FGVoiceThread;
	FGVoiceThread *_thread;
#endif

  std::string _host;
  std::string _port;
	bool _enabled;
	SGPropertyNode_ptr _pausedNode;
	bool _paused;
  std::vector<FGVoice *> _voices;
};



#if defined(ENABLE_THREADS)
class FGVoiceMgr::FGVoiceThread : public OpenThreads::Thread {
public:
	FGVoiceThread(FGVoiceMgr *mgr) : _mgr(mgr) {}
	void run();
	void wake_up() { _jobs.signal(); }

private:
	void wait_for_jobs() { OpenThreads::ScopedLock<OpenThreads::Mutex> g(_mutex); _jobs.wait(&_mutex); }
	OpenThreads::Condition _jobs;
	OpenThreads::Mutex _mutex;
protected:
	FGVoiceMgr *_mgr;
};
#endif


class FGVoiceMgr::FGVoice : public SGPropertyChangeListener {
public:
  FGVoice(FGVoiceMgr * mgr ) : _mgr(mgr) {}
  virtual ~FGVoice() {}
  virtual void speak( const std::string & msg ) = 0;
  virtual void update() = 0;
  void pushMessage( const std::string & m);
  bool speak();

protected:
  void valueChanged(SGPropertyNode *node);

  FGVoiceMgr *_mgr;

  #if defined(ENABLE_THREADS)
  SGLockedQueue<std::string> _msg;
#else
  std::queue<std::string> _msg;
#endif


};

#endif // _VOICE_HXX
