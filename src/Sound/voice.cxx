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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <Main/globals.hxx>
#include <sstream>
#include <simgear/compiler.h>
#include <Main/fg_props.hxx>
#include "voice.hxx"

#define VOICE "/sim/sound/voices"

using std::string;

/// MANAGER ///

FGVoiceMgr::FGVoiceMgr() :
	_host(fgGetString(VOICE "/host", "localhost")),
	_port(fgGetString(VOICE "/port", "1314")),
	_enabled(fgGetBool(VOICE "/enabled", false)),
	_pausedNode(fgGetNode("/sim/sound/working", true))
{
#if defined(ENABLE_THREADS)
	if (!_enabled)
		return;
	_thread = new FGVoiceThread(this);
#endif
}


FGVoiceMgr::~FGVoiceMgr()
{
#if defined(ENABLE_THREADS)
	if (!_enabled)
		return;
	_thread->cancel();
	_thread->join();
	delete _thread;
#endif
}


void FGVoiceMgr::init()
{
	if (!_enabled)
		return;

	SGPropertyNode *base = fgGetNode(VOICE, true);
	vector<SGPropertyNode_ptr> voices = base->getChildren("voice");
	try {
		for (unsigned int i = 0; i < voices.size(); i++)
			_voices.push_back(new FGVoice(this, voices[i]));
	} catch (const std::string& s) {
		SG_LOG(SG_SOUND, SG_ALERT, "VOICE: " << s);
	}

#if defined(ENABLE_THREADS)
	_thread->setProcessorAffinity(1);
	_thread->start();
#endif
}


void FGVoiceMgr::update(double)
{
	if (!_enabled)
		return;

	_paused = !_pausedNode->getBoolValue();
	for (unsigned int i = 0; i < _voices.size(); i++) {
		_voices[i]->update();
#if !defined(ENABLE_THREADS)
		_voices[i]->speak();
#endif
	}

}




/// VOICE ///

FGVoiceMgr::FGVoice::FGVoice(FGVoiceMgr *mgr, const SGPropertyNode_ptr node) :
	_volumeNode(node->getNode("volume", true)),
	_pitchNode(node->getNode("pitch", true)),
	_speedNode(node->getNode("speed", true)),
	_festival(node->getBoolValue("festival", true)),
	_mgr(mgr)
{
	SG_LOG(SG_SOUND, SG_INFO, "VOICE: adding `" << node->getStringValue("desc", "<unnamed>")
			<< "' voice");
	const string &host = _mgr->_host;
	const string &port = _mgr->_port;

	_sock = new SGSocket(host, port, "tcp");
	_sock->set_timeout(6000);
	if (!_sock->open(SG_IO_OUT))
		throw string("no connection to `") + host + ':' + port + '\'';

	if (_festival) {
		_sock->writestring("(SayText \"\")\015\012");
		char buf[4];
		int len = _sock->read(buf, 3);
		if (len != 3 || buf[0] != 'L' || buf[1] != 'P')
			throw string("unexpected or no response from `") + host + ':' + port
					+ "'. Either it's not\n       Festival listening,"
					" or Festival couldn't open a sound device.";

		SG_LOG(SG_SOUND, SG_INFO, "VOICE: connection to Festival server on `"
				<< host << ':' << port << "' established");

		setVolume(_volume = _volumeNode->getDoubleValue());
		setPitch(_pitch = _pitchNode->getDoubleValue());
		setSpeed(_speed = _speedNode->getDoubleValue());
	}

	string preamble = node->getStringValue("preamble", "");
	if (!preamble.empty())
		pushMessage(preamble);

	node->getNode("text", true)->addChangeListener(new FGVoiceListener(this));
}


FGVoiceMgr::FGVoice::~FGVoice()
{
	_sock->close();
	delete _sock;
}


void FGVoiceMgr::FGVoice::pushMessage(string m)
{
	_msg.push(m + "\015\012");
#if defined(ENABLE_THREADS)
	_mgr->_thread->wake_up();
#endif
}


bool FGVoiceMgr::FGVoice::speak(void)
{
	if (_msg.empty())
		return false;

	const string s = _msg.front();
	_msg.pop();
	_sock->writestring(s.c_str());
	return !_msg.empty();
}


void FGVoiceMgr::FGVoice::update(void)
{
	if (_festival) {
		double d;
		d = _volumeNode->getDoubleValue();
		if (d != _volume)
			setVolume(_volume = d);
		d = _pitchNode->getDoubleValue();
		if (d != _pitch)
			setPitch(_pitch = d);
		d = _speedNode->getDoubleValue();
		if (d != _speed)
			setSpeed(_speed = d);
	}
}


void FGVoiceMgr::FGVoice::setVolume(double d)
{
	std::ostringstream s;
	s << "(set! default_after_synth_hooks (list (lambda (utt)"
			"(utt.wave.rescale utt " << d << " t))))";
	pushMessage(s.str());
}


void FGVoiceMgr::FGVoice::setPitch(double d)
{
	std::ostringstream s;
	s << "(set! int_lr_params '((target_f0_mean " << d <<
			")(target_f0_std 14)(model_f0_mean 170)"
			"(model_f0_std 34)))";
	pushMessage(s.str());
}


void FGVoiceMgr::FGVoice::setSpeed(double d)
{
	std::ostringstream s;
	s << "(Parameter.set 'Duration_Stretch " << d << ')';
	pushMessage(s.str());
}




/// THREAD ///

#if defined(ENABLE_THREADS)
void FGVoiceMgr::FGVoiceThread::run(void)
{
	while (1) {
		bool busy = false;
		for (unsigned int i = 0; i < _mgr->_voices.size(); i++)
			busy |= _mgr->_voices[i]->speak();

		if (!busy)
			wait_for_jobs();
	}
}
#endif




/// LISTENER ///

void FGVoiceMgr::FGVoice::FGVoiceListener::valueChanged(SGPropertyNode *node)
{
	if (_voice->_mgr->_paused)
		return;

	const string s = node->getStringValue();
	//cerr << "\033[31;1mBEFORE [" << s << "]\033[m" << endl;

	string m;
	for (unsigned int i = 0; i < s.size(); i++) {
		char c = s[i];
		if (c == '\n' || c == '\r' || c == '\t')
			m += ' ';
		else if (!isprint(c))
			continue;
		else if (c == '\\' || c == '"')
			m += '\\', m += c;
		else if (c == '|' || c == '_')
			m += ' ';	// don't say "vertical bar" or "underscore"
		else if (c == '&')
			m += " and ";
		else if (c == '{') {
			while (i < s.size())
				if (s[++i] == '|')
					break;
		} else if (c == '}')
			;
		else
			m += c;
	}
	//cerr << "\033[31;1mAFTER [" << m << "]\033[m" << endl;
	if (_voice->_festival)
		m = string("(SayText \"") + m + "\")";

	_voice->pushMessage(m);
}


