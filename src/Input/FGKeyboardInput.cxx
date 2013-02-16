// FGKeyboardInput.cxx -- handle user input from keyboard devices
//
// Written by Torsten Dreyer, started August 2009
// Based on work from David Megginson, started May 2001.
//
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
// Copyright (C) 2001 David Megginson, david@megginson.com
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

#include "FGKeyboardInput.hxx"
#include <Main/fg_props.hxx>
#include <Scripting/NasalSys.hxx>
#include <plib/pu.h>

using simgear::PropertyList;

static int getModifiers ()
{
  return fgGetKeyModifiers() >> 1;
}

static bool getModShift ()
{
  return (fgGetKeyModifiers() & KEYMOD_SHIFT) != 0;
}

static bool getModCtrl ()
{
  return (fgGetKeyModifiers() & KEYMOD_CTRL) != 0;
}

static bool getModAlt ()
{
  return (fgGetKeyModifiers() & KEYMOD_ALT) != 0;
}

static bool getModMeta ()
{
  return (fgGetKeyModifiers() & KEYMOD_META) != 0;
}

static bool getModSuper ()
{
  return (fgGetKeyModifiers() & KEYMOD_SUPER) != 0;
}

static bool getModHyper ()
{
  return (fgGetKeyModifiers() & KEYMOD_HYPER) != 0;
}

FGKeyboardInput * FGKeyboardInput::keyboardInput = NULL;

FGKeyboardInput::FGKeyboardInput() :
    _key_event(fgGetNode("/devices/status/keyboard/event", true)),
    _key_code(0),
    _key_modifiers(0),
    _key_pressed(0),
    _key_shift(0),
    _key_ctrl(0),
    _key_alt(0),
    _key_meta(0),
    _key_super(0),
    _key_hyper(0)
{
  if( keyboardInput == NULL )
    keyboardInput = this;
}

FGKeyboardInput::~FGKeyboardInput()
{
  if( keyboardInput == this )
    keyboardInput = NULL;
}


void FGKeyboardInput::init()
{
  fgRegisterKeyHandler(keyHandler);
}

void FGKeyboardInput::postinit()
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing key bindings");
  string module = "__kbd";
  SGPropertyNode * key_nodes = fgGetNode("/input/keyboard");
  if (key_nodes == NULL) {
    SG_LOG(SG_INPUT, SG_WARN, "No key bindings (/input/keyboard)!!");
    key_nodes = fgGetNode("/input/keyboard", true);
  }

  FGNasalSys *nasalsys = (FGNasalSys *)globals->get_subsystem("nasal");
  PropertyList nasal = key_nodes->getChildren("nasal");
  for (unsigned int j = 0; j < nasal.size(); j++) {
    nasal[j]->setStringValue("module", module.c_str());
    nasalsys->handleCommand(nasal[j]);
  }

  PropertyList keys = key_nodes->getChildren("key");
  for (unsigned int i = 0; i < keys.size(); i++) {
    int index = keys[i]->getIndex();
    SG_LOG(SG_INPUT, SG_DEBUG, "Binding key " << index);
    if( index >= MAX_KEYS ) {
      SG_LOG(SG_INPUT, SG_WARN, "Key binding " << index << " out of range");
      continue;
    }

    bindings[index].bindings->clear();
    bindings[index].is_repeatable = keys[i]->getBoolValue("repeatable");
    bindings[index].last_state = 0;
    read_bindings(keys[i], bindings[index].bindings, KEYMOD_NONE, module );
  }
}

void FGKeyboardInput::bind()
{
  _tiedProperties.setRoot(fgGetNode("/devices/status", true));
  _tiedProperties.Tie("keyboard",       getModifiers);
  _tiedProperties.Tie("keyboard/shift", getModShift);
  _tiedProperties.Tie("keyboard/ctrl",  getModCtrl);
  _tiedProperties.Tie("keyboard/alt",   getModAlt);
  _tiedProperties.Tie("keyboard/meta",  getModMeta);
  _tiedProperties.Tie("keyboard/super", getModSuper);
  _tiedProperties.Tie("keyboard/hyper", getModHyper);

  _tiedProperties.Tie(_key_event->getNode("key", true),            SGRawValuePointer<int>(&_key_code));
  _tiedProperties.Tie(_key_event->getNode("pressed", true),        SGRawValuePointer<bool>(&_key_pressed));
  _tiedProperties.Tie(_key_event->getNode("modifier", true),       SGRawValuePointer<int>(&_key_modifiers));
  _tiedProperties.Tie(_key_event->getNode("modifier/shift", true), SGRawValuePointer<bool>(&_key_shift));
  _tiedProperties.Tie(_key_event->getNode("modifier/ctrl", true),  SGRawValuePointer<bool>(&_key_ctrl));
  _tiedProperties.Tie(_key_event->getNode("modifier/alt", true),   SGRawValuePointer<bool>(&_key_alt));
  _tiedProperties.Tie(_key_event->getNode("modifier/meta", true),  SGRawValuePointer<bool>(&_key_meta));
  _tiedProperties.Tie(_key_event->getNode("modifier/super", true), SGRawValuePointer<bool>(&_key_super));
  _tiedProperties.Tie(_key_event->getNode("modifier/hyper", true), SGRawValuePointer<bool>(&_key_hyper));
}

void FGKeyboardInput::unbind()
{
  _tiedProperties.Untie();
}

void FGKeyboardInput::update( double dt )
{
  // nothing to do
}

const FGCommonInput::binding_list_t & FGKeyboardInput::_find_key_bindings (unsigned int k, int modifiers)
{
  unsigned char kc = (unsigned char)k;
  FGButton &b = bindings[k];

                                // Try it straight, first.
  if (b.bindings[modifiers].size() > 0)
    return b.bindings[modifiers];

                                // Alt-Gr is CTRL+ALT
  else if (modifiers&(KEYMOD_CTRL|KEYMOD_ALT))
    return _find_key_bindings(k, modifiers&~(KEYMOD_CTRL|KEYMOD_ALT));

                                // Try removing the control modifier
                                // for control keys.
  else if ((modifiers&KEYMOD_CTRL) && iscntrl(kc))
    return _find_key_bindings(k, modifiers&~KEYMOD_CTRL);

                                // Try removing shift modifier 
                                // for upper case or any punctuation
                                // (since different keyboards will
                                // shift different punctuation types)
  else if ((modifiers&KEYMOD_SHIFT) && (isupper(kc) || ispunct(kc)))
    return _find_key_bindings(k, modifiers&~KEYMOD_SHIFT);

                                // Try removing alt modifier for
                                // high-bit characters.
  else if ((modifiers&KEYMOD_ALT) && k >= 128 && k < 256)
    return _find_key_bindings(k, modifiers&~KEYMOD_ALT);

                                // Give up and return the empty vector.
  else
    return b.bindings[modifiers];
}

void FGKeyboardInput::doKey (int k, int modifiers, int x, int y)
{
  // Sanity check.
  if (k < 0 || k >= MAX_KEYS) {
    // normal for unsupported keys (i.e. left/right shift key press events)
    SG_LOG(SG_INPUT, SG_DEBUG, "Key value " << k << " out of range");
    return;
  }

  _key_code = k;
  _key_modifiers = modifiers >> 1;
  _key_pressed = (modifiers & KEYMOD_RELEASED) == 0;
  _key_shift = getModShift();
  _key_ctrl = getModCtrl();
  _key_alt = getModAlt();
  _key_meta = getModMeta();
  _key_super = getModSuper();
  _key_hyper = getModHyper();
  _key_event->fireValueChanged();
  if (_key_code < 0)
    return;

  k = _key_code;
  modifiers = _key_modifiers << 1;
  if (!_key_pressed)
      modifiers |= KEYMOD_RELEASED;
  FGButton &b = bindings[k];

                                // Key pressed.
  if (!(modifiers & KEYMOD_RELEASED)) {
    SG_LOG( SG_INPUT, SG_DEBUG, "User pressed key " << k << " with modifiers " << modifiers );
    if (!b.last_state || b.is_repeatable) {
      const binding_list_t &bindings = _find_key_bindings(k, modifiers);

      for (unsigned int i = 0; i < bindings.size(); i++)
        bindings[i]->fire();
      b.last_state = 1;
    }
  }
                                // Key released.
  else {
    SG_LOG(SG_INPUT, SG_DEBUG, "User released key " << k << " with modifiers " << modifiers);
    if (b.last_state) {
      const binding_list_t &bindings = _find_key_bindings(k, modifiers);
      for (unsigned int i = 0; i < bindings.size(); i++)
        bindings[i]->fire();
      b.last_state = 0;
    }
  }
}

void FGKeyboardInput::keyHandler(int key, int keymod, int mousex, int mousey)
{
  if((keymod & KEYMOD_RELEASED) == 0)
      if(puKeyboard(key, PU_DOWN))
          return;

  if(keyboardInput)
      keyboardInput->doKey(key, keymod, mousex, mousey);
}
