// fg_sound.cxx -- Sound class implementation
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
#include <string.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/condition.hxx>

#include <Main/fg_props.hxx>

#include "fg_sound.hxx"


// static double _fg_lin(double v)   { return v; }
static double _fg_inv(double v)   { return (v == 0) ? 1e99 : 1/v; }
static double _fg_abs(double v)   { return (v >= 0) ? v : -v; }
static double _fg_sqrt(double v)  { return (v < 0) ? sqrt(-v) : sqrt(v); }
static double _fg_log10(double v) { return (v < 1) ? 0 : log10(v); }
static double _fg_log(double v)   { return (v < 1) ? 0 : log(v); }
// static double _fg_sqr(double v)   { return pow(v, 2); }
// static double _fg_pow3(double v)  { return pow(v, 3); }

static const struct {
	char *name;
	double (*fn)(double);
} __fg_snd_fn[] = {
//	{"lin", _fg_lin},
	{"inv", _fg_inv},
	{"abs", _fg_abs},
	{"sqrt", _fg_sqrt},
	{"log", _fg_log10},
	{"ln", _fg_log},
//	{"sqr", _fg_sqr},
//	{"pow3", _fg_pow3},
	{"", NULL}
};

FGSound::FGSound()
  : _sample(NULL),
    _condition(NULL),
    _property(NULL),
    _active(false),
    _name(""),
    _mode(FGSound::ONCE),
    _prev_value(0),
    _dt_play(0.0),
    _dt_stop(0.0),
    _stopping(0.0)
{
}

FGSound::~FGSound()
{
   _mgr->get_scheduler()->stopSample(_sample->get_sample());

   if (_property)
      delete _property;

   if (_condition)
      delete _condition;

   _volume.clear();
   _pitch.clear();
   delete _sample;
}

void
FGSound::init(SGPropertyNode *node)
{

   //
   // set global sound properties
   //
   
   _name = node->getStringValue("name", "");
   SG_LOG(SG_GENERAL, SG_INFO, "Loading sound information for: " << _name );

   const char *mode_str = node->getStringValue("mode", "");
   if ( !strcmp(mode_str, "looped") ) {
       _mode = FGSound::LOOPED;

   } else if ( !strcmp(mode_str, "in-transit") ) {
       _mode = FGSound::IN_TRANSIT;

   } else {
      _mode = FGSound::ONCE;

      if ( strcmp(mode_str, "") )
         SG_LOG(SG_GENERAL,SG_INFO, "  Unknown sound mode, default to 'once'");
   }

   _property = fgGetNode(node->getStringValue("property", ""), true);
   SGPropertyNode *condition = node->getChild("condition");
   if (condition != NULL)
      _condition = fgReadCondition(globals->get_props(), condition);

   if (!_property && !_condition)
      SG_LOG(SG_GENERAL, SG_WARN,
             "  Neither a condition nor a property specified");

   //
   // set volume properties
   //
   unsigned int i;
   float v = 0.0;
   vector<SGPropertyNode_ptr> kids = node->getChildren("volume");
   for (i = 0; (i < kids.size()) && (i < FGSound::MAXPROP); i++) {
      _snd_prop volume = {NULL, NULL, NULL, 1.0, 0.0, 0.0, 0.0, false};

      if (strcmp(kids[i]->getStringValue("property"), ""))
         volume.prop = fgGetNode(kids[i]->getStringValue("property", ""), true);

      const char *intern_str = kids[i]->getStringValue("internal", "");
      if (!strcmp(intern_str, "dt_play"))
         volume.intern = &_dt_play;
      else if (!strcmp(intern_str, "dt_stop"))
         volume.intern = &_dt_stop;

      if ((volume.factor = kids[i]->getDoubleValue("factor", 1.0)) != 0.0)
         if (volume.factor < 0.0) {
            volume.factor = -volume.factor;
            volume.subtract = true;
         }

      const char *type_str = kids[i]->getStringValue("type", "");
      if ( strcmp(type_str, "") ) {

         for (int j=0; __fg_snd_fn[j].fn; j++)
           if ( !strcmp(type_str, __fg_snd_fn[j].name) ) {
               volume.fn = __fg_snd_fn[j].fn;
               break;
            }

         if (!volume.fn)
            SG_LOG(SG_GENERAL,SG_INFO,
                   "  Unknown volume type, default to 'lin'");
      }

      volume.offset = kids[i]->getDoubleValue("offset", 0.0);

      if ((volume.min = kids[i]->getDoubleValue("min", 0.0)) < 0.0)
         SG_LOG( SG_GENERAL, SG_WARN,
          "Volume minimum value below 0. Forced to 0.");

      volume.max = kids[i]->getDoubleValue("max", 0.0);
      if (volume.max && (volume.max < volume.min) )
         SG_LOG(SG_GENERAL,SG_ALERT,
                "  Volume maximum below minimum. Neglected.");

      _volume.push_back(volume);
      v += volume.offset;

   }


   //
   // set pitch properties
   //
   float p = 0.0;
   kids = node->getChildren("pitch");
   for (i = 0; (i < kids.size()) && (i < FGSound::MAXPROP); i++) {
      _snd_prop pitch = {NULL, NULL, NULL, 1.0, 1.0, 0.0, 0.0, false};

      if (strcmp(kids[i]->getStringValue("property", ""), ""))
         pitch.prop = fgGetNode(kids[i]->getStringValue("property", ""), true);

      const char *intern_str = kids[i]->getStringValue("internal", "");
      if (!strcmp(intern_str, "dt_play"))
         pitch.intern = &_dt_play;
      else if (!strcmp(intern_str, "dt_stop"))
         pitch.intern = &_dt_stop;

      if ((pitch.factor = kids[i]->getDoubleValue("factor", 1.0)) != 0.0)
         if (pitch.factor < 0.0) {
            pitch.factor = -pitch.factor;
            pitch.subtract = true;
         }

      const char *type_str = kids[i]->getStringValue("type", "");
      if ( strcmp(type_str, "") ) {

         for (int j=0; __fg_snd_fn[j].fn; j++) 
            if ( !strcmp(type_str, __fg_snd_fn[j].name) ) {
               pitch.fn = __fg_snd_fn[j].fn;
               break;
            }

         if (!pitch.fn)
            SG_LOG(SG_GENERAL,SG_INFO,
                   "  Unknown pitch type, default to 'lin'");
      }
     
      pitch.offset = kids[i]->getDoubleValue("offset", 1.0);

      if ((pitch.min = kids[i]->getDoubleValue("min", 0.0)) < 0.0)
         SG_LOG(SG_GENERAL,SG_WARN,
                "  Pitch minimum value below 0. Forced to 0.");

      pitch.max = kids[i]->getDoubleValue("max", 0.0);
      if (pitch.max && (pitch.max < pitch.min) )
         SG_LOG(SG_GENERAL,SG_ALERT,
                "  Pitch maximum below minimum. Neglected");

      _pitch.push_back(pitch);
      p += pitch.offset;
   }

   //
   // Initialize the sample
   //
   _mgr = globals->get_soundmgr();
   if ((_sample = _mgr->find(_name)) == NULL)
      _sample = _mgr->add(_name, node->getStringValue("path", ""));

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
FGSound::update (double dt)
{
   double curr_value = 0.0;

   //
   // If the state changes to false, stop playing.
   //
   if (_property)
       curr_value = _property->getDoubleValue();

   if (							// Lisp, anyone?
       (_condition && !_condition->test()) ||
       (!_condition && _property &&
        (
         !curr_value ||
         ( (_mode == FGSound::IN_TRANSIT) && (curr_value == _prev_value) )
         )
        )
       )
   {
       if ((_mode != FGSound::IN_TRANSIT) || (_stopping > MAX_TRANSIT_TIME)) {
           if (_sample->is_playing()) {
               SG_LOG(SG_GENERAL, SG_INFO, "Stopping audio after " << _dt_play
                      << " sec: " << _name );

               _sample->stop( _mgr->get_scheduler() );
           }

           _active = false;
           _dt_stop += dt;
           _dt_play = 0.0;
       } else {
           _stopping += dt;
       }

       return;
   }

   //
   // If the mode is ONCE and the sound is still playing,
   //  we have nothing to do anymore.
   //
   if (_active && (_mode == FGSound::ONCE)) {

      if (!_sample->is_playing()) {
         _dt_stop += dt;
         _dt_play = 0.0;
      } else {
         _dt_play += dt;
      }

      return;
   }

   //
   // Update the playing time, cache the current value and
   // clear the delay timer.
   //
   _dt_play += dt;
   _prev_value = curr_value;
   _stopping = 0.0;

   //
   // Update the volume
   //
   int i;
   int max = _volume.size();
   double volume = 1.0;
   double volume_offset = 0.0;

   for(i = 0; i < max; i++) {
      double v = 1.0;

      if (_volume[i].prop)
         v = _volume[i].prop->getDoubleValue();

      else if (_volume[i].intern)
         v = *_volume[i].intern;

      if (_volume[i].fn)
         v = _volume[i].fn(v);

      v *= _volume[i].factor;

      if (_volume[i].max && (v > _volume[i].max))
         v = _volume[i].max;

      else if (v < _volume[i].min)
         v = _volume[i].min;

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
   double pitch = 1.0;
   double pitch_offset = 0.0;

   for(i = 0; i < max; i++) {
      double p = 1.0;

      if (_pitch[i].prop)
         p = _pitch[i].prop->getDoubleValue();

      else if (_pitch[i].intern)
         p = *_pitch[i].intern;

      if (_pitch[i].fn)
         p = _pitch[i].fn(p);

      p *= _pitch[i].factor;

      if (_pitch[i].max && (p > _pitch[i].max))
         p = _pitch[i].max;

      else if (p < _pitch[i].min)
         p = _pitch[i].min;

      if (_pitch[i].subtract)				// Hack!
         pitch = _pitch[i].offset - p;

      else {
         pitch_offset += _pitch[i].offset;
         pitch *= p;
      }
   }

   //
   // Change sample state
   //
   _sample->set_pitch( pitch_offset + pitch );
   _sample->set_volume( volume_offset + volume );


   //
   // Do we need to start playing the sample?
   //
   if (!_active) {

      if (_mode == FGSound::ONCE)
         _sample->play(_mgr->get_scheduler(), false);

      else
         _sample->play(_mgr->get_scheduler(), true);

      SG_LOG(SG_GENERAL, SG_INFO, "Playing audio after " << _dt_stop 
                                   << " sec: " << _name);
      SG_LOG(SG_GENERAL, SG_BULK,
                         "Playing " << ((_mode == ONCE) ? "once" : "looped"));

      _active = true;
      _dt_stop = 0.0;
   }
}
