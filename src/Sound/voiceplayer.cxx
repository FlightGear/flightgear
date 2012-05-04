// voiceplayer.cxx -- voice/sound sample player
//
// Written by Jean-Yves Lefort, started September 2005.
//
// Copyright (C) 2005, 2006  Jean-Yves Lefort - jylefort@FreeBSD.org
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
#  pragma warning( disable: 4355 )
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <string>
#include <sstream>

#include <simgear/debug/logstream.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

#include "voiceplayer.hxx"

///////////////////////////////////////////////////////////////////////////////
// constants //////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// helpers ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define ADD_VOICE(Var,Sample,Twice) \
    { make_voice(&Var);append(Var,Sample);\
      if (Twice) append(Var,Sample); }

#define test_bits(_bits, _test) (((_bits) & (_test)) != 0)

///////////////////////////////////////////////////////////////////////////////
// FGVoicePlayer //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void
FGVoicePlayer::Speaker::bind (SGPropertyNode *node)
{
    // uses xmlsound property names
    tie(node, "volume", &volume);
    tie(node, "pitch", &pitch);
}

void
FGVoicePlayer::Speaker::update_configuration ()
{
    map< string, SGSharedPtr<SGSoundSample> >::iterator iter;
    for (iter = player->samples.begin(); iter != player->samples.end(); iter++)
    {
        SGSoundSample *sample = (*iter).second;

        sample->set_pitch(pitch);
    }

    if (player->voice)
        player->voice->volume_changed();
}

FGVoicePlayer::Voice::~Voice ()
{
    for (iter = elements.begin(); iter != elements.end(); iter++)
        delete *iter;       // we owned the element
    elements.clear();
}

void
FGVoicePlayer::Voice::play ()
{
    iter = elements.begin();
    element = *iter;

    element->play(get_volume());
}

void
FGVoicePlayer::Voice::stop (bool now)
{
    if (element)
    {
        if (now || element->silence)
        {
            element->stop();
            element = NULL;
        }
        else
            iter = elements.end() - 1; // stop after the current element finishes
    }
}

void
FGVoicePlayer::Voice::set_volume (float _volume)
{
    volume = _volume;
    volume_changed();
}

void
FGVoicePlayer::Voice::volume_changed ()
{
    if (element)
        element->set_volume(get_volume());
}

void
FGVoicePlayer::Voice::update ()
{
    if (element)
    {
        if (! element->is_playing())
        {
            if (++iter == elements.end())
                element = NULL;
            else
            {
                element = *iter;
                element->play(get_volume());
            }
        }
    }
}

FGVoicePlayer::~FGVoicePlayer ()
{
    vector<Voice *>::iterator iter1;
    for (iter1 = _voices.begin(); iter1 != _voices.end(); iter1++)
        delete *iter1;
    _voices.clear();
    samples.clear();
}

void
FGVoicePlayer::bind (SGPropertyNode *node, const char* default_dir_prefix)
{
    dir_prefix = node->getStringValue("voice/file-prefix", default_dir_prefix);
    speaker.bind(node);
}

void
FGVoicePlayer::init ()
{
    SGSoundMgr *smgr = globals->get_soundmgr();
    _sgr = smgr->find("avionics", true);
    _sgr->tie_to_listener();
    speaker.update_configuration();
}

void
FGVoicePlayer::pause()
{
    if (paused)
        return;

    paused = true;
    if (voice)
    {
        voice->stop(true);
    }
}

void
FGVoicePlayer::resume()
{
    if (!paused)
        return;
    paused = false;
    if (voice)
    {
        voice->play();
    }
}

SGSoundSample *
FGVoicePlayer::get_sample (const char *name)
{
    string refname;
    refname = dev_name + "/" + dir_prefix + name;

    SGSoundSample *sample = _sgr->find(refname);
    if (! sample)
    {
        string filename = dir_prefix + string(name) + ".wav";
        try
        {
            sample = new SGSoundSample(filename.c_str(), SGPath());
        }
        catch (const sg_exception &e)
        {
            SG_LOG(SG_SOUND, SG_ALERT, "Error loading sound sample \"" + filename + "\": " + e.getFormattedMessage());
            exit(1);
        }

        _sgr->add(sample, refname);
        samples[refname] = sample;
    }

    return sample;
}

void
FGVoicePlayer::play (Voice *_voice, unsigned int flags)
{
    if (!_voice)
        return;
    if (test_bits(flags, PLAY_NOW) || ! voice ||
            (voice->element && voice->element->silence))
    {
        if (voice)
            voice->stop(true);

        voice = _voice;
        looped = test_bits(flags, PLAY_LOOPED);

        next_voice = NULL;
        next_looped = false;

        if (!paused)
            voice->play();
    }
    else
    {
        next_voice = _voice;
        next_looped = test_bits(flags, PLAY_LOOPED);
    }
}

void
FGVoicePlayer::stop (unsigned int flags)
{
    if (voice)
    {
        voice->stop(test_bits(flags, STOP_NOW));
        if (voice->element)
            looped = false;
        else
            voice = NULL;
        next_voice = NULL;
    }
}

void
FGVoicePlayer::set_volume (float _volume)
{
    volume = _volume;
    if (voice)
        voice->volume_changed();
}

void
FGVoicePlayer::update ()
{
    if (voice)
    {
        voice->update();

        if (next_voice)
        {
            if (! voice->element || voice->element->silence)
            {
                voice = next_voice;
                looped = next_looped;

                next_voice = NULL;
                next_looped = false;

                voice->play();
            }
        }
        else
        {
            if (! voice->element)
            {
                if (looped)
                    voice->play();
                else
                    voice = NULL;
            }
        }
    }
}
