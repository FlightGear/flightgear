// fg_sound.hxx -- Sound class implementation
//
// Started by Erik Hofman, February 2002
//
// Copyright (C) 2002  Erik Hofman - erik@ehofman.com
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

#ifndef __FGSOUND_HXX
#define __FGSOUND_HXX 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <Main/fgfs.hxx>
#include <Main/globals.hxx>

#include "soundmgr.hxx"

/**
 * Class for handling one sound.
 *
 */
class FGSound : public FGSubsystem
{

public:

  enum { MAXPROP=5 };
  enum { LIN=0, LOG, LN };
  enum { ONCE=0, LOOPED };
  enum { LEVEL=0, INVERTED, FLIPFLOP };

  FGSound(const SGPropertyNode *);
  virtual ~FGSound();

  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (int dt);

private:

  const SGPropertyNode * _node;

  FGSimpleSound * _sample;
  const SGPropertyNode * _property;

  bool _active;

  int _mode;
  int _type;
  string _name;
  float _factor;

#if 0
  // Sound source (distance, horizontal position in degrees and 
  // vertical position in degrees)
  struct {
 	float dist;
	float hor;
	float vert;
  } _pos;
#endif

  // Sound properties
  typedef struct {
        const SGPropertyNode * prop;
        float factor;
        int type;
        float offset;
        float min;
        float max;
	bool subtract;
  } _snd_prop;

  vector<_snd_prop> _volume;
  vector<_snd_prop> _pitch;

};

#endif
