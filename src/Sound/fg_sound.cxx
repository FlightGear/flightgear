// fg_sound.hxx -- Sound class implementation
//
// Started by Erik Hofman, February 2002
// (Reuses some code from  fg_fx.cxx created by David Megginson)
//
// Copyright (C) 2002  Curtis L. Olson - curt@flightgear.org
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

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>

#include "fg_sound.hxx"

FGSound::FGSound(const SGPropertyNode * node)
  : _name(""),
    _sample(0),
    _factor(1.0),
    _active(false),
    _mode(FGSound::ONCE),
    _type(FGSound::LEVEL),
    _node(node)
{
}

FGSound::~FGSound()
{
   delete _sample;
}

void
FGSound::init()
{
   vector<const SGPropertyNode *> kids;
   float p = 0.0;
   float v = 0.0;
   int i;

   _property = fgGetNode(_node->getStringValue("property"), true);

   //
   // seet sound global properties
   //
   _name = _node->getStringValue("name");

   if ((_factor = _node->getFloatValue("factor")) == 0.0)
      _factor = 1.0;

   SG_LOG(SG_GENERAL, SG_INFO,
    "Loading sound information for: " << _name );

   if (_node->getStringValue("mode") == "looped") {
       _mode = FGSound::LOOPED;
   } else {
      _mode = FGSound::ONCE;
      if (_node->getStringValue("mode") != (string)"once")
        SG_LOG( SG_GENERAL, SG_INFO, "Unknown sound mode, default to 'once'");
   }

   if (_node->getStringValue("type") == "flipflop") {
      _type = FGSound::FLIPFLOP;
   } else if (_node->getStringValue("type") == "inverted") {
      _type = FGSound::INVERTED;
   } else {
      _type = FGSound::LEVEL;
      if (_node->getStringValue("type") != (string)"level")
        SG_LOG( SG_GENERAL, SG_INFO, "Unknown sound type, default to 'level'");
   }


#if 0
   //
   // set position properties
   //
   _pos.dist = _node->getFloatValue("dist");
   _pos.hor = _node->getFloatValue("pos_hor");
   _pos.vert = _node->getFloatValue("pos_vert"); 
#endif

   //
   // set volume properties
   //
   kids = _node->getChildren("volume");
   for (i = 0; (i < kids.size()) && (i < FGSound::MAXPROP); i++) {
      _snd_prop volume;

      if ((volume.prop=fgGetNode(kids[i]->getStringValue("property"), true))
          == 0)
         volume.prop = fgGetNode("/null", true);

      if ((volume.factor = kids[i]->getFloatValue("factor")) == 0.0)
         volume.factor = 1.0;
      else 
         if (volume.factor < 0.0) {
            volume.factor = -volume.factor;
            volume.subtract = true;
         } else
            volume.subtract = false;

      if (kids[i]->getStringValue("type") == "log")
         volume.type = FGSound::LOG;

      else if (kids[i]->getStringValue("type") == "ln")
         volume.type = FGSound::LN;

      else  {
         volume.type = FGSound::LIN;
         if (kids[i]->getStringValue("type") != (string)"lin")
            SG_LOG( SG_GENERAL, SG_INFO,
             "Unknown volume type, default to 'lin'");
      }

      if ((volume.offset = kids[i]->getFloatValue("offset")) == 0.0)
         volume.offset = 0.0;

      if ((volume.min = kids[i]->getFloatValue("min")) < 0.0) {
         SG_LOG( SG_GENERAL, SG_WARN,
          "Volume minimum value below 0. Forced to 0.");
         volume.min = 0.0;
      }

      if ((volume.max = kids[i]->getFloatValue("max")) <= volume.min) {
         SG_LOG( SG_GENERAL, SG_ALERT,
          "Volume maximum value below minimum value. Forced above minimum.");
        volume.max = volume.min + 5.0;
      }

      _volume.push_back(volume);
      v += volume.offset;

   }


   //
   // set pitch properties
   //
   kids = _node->getChildren("pitch");

   for (i = 0; (i < kids.size()) && (i < FGSound::MAXPROP); i++) {
      _snd_prop pitch;

      if ((pitch.prop = fgGetNode(kids[i]->getStringValue("property"), true))
          == 0)
         pitch.prop = fgGetNode("/null", true);

      if ((pitch.factor = kids[i]->getFloatValue("factor")) == 0.0)
         pitch.factor = 1.0;

      if (kids[i]->getStringValue("type") == "log")
         pitch.type = FGSound::LOG;

      else if (kids[i]->getStringValue("type") == "ln")
         pitch.type = FGSound::LN;

      else {
         pitch.type = FGSound::LIN;
         if (kids[i]->getStringValue("type") != (string)"lin")
            SG_LOG( SG_GENERAL, SG_INFO,
             "Unknown pitch type, default to 'lin'");
      }

      if ((pitch.offset = kids[i]->getFloatValue("offset")) == 0.0)
         pitch.offset = 1.0;

      if ((pitch.min = kids[i]->getFloatValue("min")) < 0.0) {
         SG_LOG( SG_GENERAL, SG_WARN,
              "Pitch minimum value below 0. Forced to 0.");
         pitch.min = 0.0;
      }

      if ((pitch.max = kids[i]->getFloatValue("max")) <= pitch.min) {
         SG_LOG( SG_GENERAL, SG_ALERT,
              "Pitch maximum value below minimum value. Forced above minimum.");
         pitch.max = pitch.min + 5.0;
      }

      _pitch.push_back(pitch);
      p += pitch.offset;
   }

   //
   // Initialize the sample
   //
   FGSoundMgr * mgr = globals->get_soundmgr();
   _sample = mgr->add(_name, _node->getStringValue("path"));
   _sample->set_volume(v);
   _sample->set_pitch(p);
}

void
FGSound::bind ()
{
}

void
FGSound::unbind ()
{
}

void
FGSound::update (int dt)
{
   int i;
   FGSoundMgr * mgr = globals->get_soundmgr();

   //
   // Do we have something to do?
   //

   // if (!_property)
   //   return;

   if ((_type == FGSound::LEVEL)  || (_type == FGSound::INVERTED)) {

      //
      // If the sound is already playing we have nothing to do.
      //
      if (_active && (_mode == FGSound::ONCE))
         return;

      int check = _property->getFloatValue() * _factor;
      if (_type == FGSound::INVERTED)
         check = !check;

      //
      // If the state changes to false, stop playing.
      //
      if (check == 0) {
         _active = false;
         if (mgr->is_playing(_name)) {
            SG_LOG(SG_GENERAL, SG_INFO, "Stopping sound: " << _name);
            mgr->stop(_name);
         }

         return;
      }

   } else {		// FGSound::FLIPFLOP

      bool check = _property->getFloatValue() * _factor;
      if (check == _active)
         return;

      //
      // Check for state changes.
      // If the state changed, and the sound is still playing: stop playing.
      //
      if (mgr->is_playing(_name)) {
         SG_LOG(SG_GENERAL, SG_INFO, "Stopping sound: " << _name);
         mgr->stop(_name);
      }
   }

   //
   // Update the volume
   //
   int max = _volume.size();

   double volume = 1.0, volume_offset = 0.0;
   for(i = 0; i < max; i++) {
      double v = _volume[i].prop->getDoubleValue();

      if (_volume[i].type == FGSound::LOG)
         v = log10(1+v);
      else
         if (_volume[i].type == FGSound::LN)
            v = log(1+v);

      v *= _volume[i].factor;

      if (v > _volume[i].max)
         v = _volume[i].max;
      else
         if (v < _volume[i].min) 
            v = 0;  // v = _volume[i].min;

      if (_volume[i].subtract)				// Hack!
         volume = _volume[i].offset - v;
      else {
         volume_offset += _volume[i].offset;
         volume *= v;
      }
   }

   //
   // Update the pitch
   //
   max = _pitch.size();
   double pitch = 1.0, pitch_offset = 0.0;
   for(i = 0; i < max; i++) {
      double p = _pitch[i].prop->getDoubleValue();

      if (_pitch[i].type == FGSound::LOG)
         p = log10(1+p);
      else
         if (_pitch[i].type == FGSound::LN)
            p = log(1+p);

      p *= _pitch[i].factor;

      if (p > _pitch[i].max)
         p = _pitch[i].max;
      else
         if (p < _pitch[i].min) 
            p = _pitch[i].min;

      pitch *= p;
      pitch_offset += _pitch[i].offset;
   }

   //
   // Change sample state
   //
   _sample->set_pitch( pitch_offset + pitch );
   _sample->set_volume( volume_offset + volume );

   //
   // Do we need to start playing the sample?
   //
   if ((!_active) || (_type == FGSound::FLIPFLOP)) {
      //
      // This is needed for FGSound::FLIPFLOP and it works for 
      // FGSound::LEVEl. Doing it this way saves an extra 'if'.
      //
      _active = !_active;

      if (_mode == FGSound::ONCE)
         mgr->play_once(_name);
      else
         mgr->play_looped(_name);

      SG_LOG(SG_GENERAL, SG_INFO, "Starting audio playback for: " << _name);
      SG_LOG(SG_GENERAL, SG_BULK,
       "Playing " << ((_mode == ONCE) ? "once" : "looped"));
      SG_LOG(SG_GENERAL, SG_BULK, "Initial volume: " << volume_offset);
      SG_LOG(SG_GENERAL, SG_BULK, "Initial pitch: " << pitch_offset);
   }
}
