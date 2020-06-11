// FGJoystickInput.hxx -- handle user input from joystick devices
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

#ifndef _FGJOYSTICKINPUT_HXX
#define _FGJOYSTICKINPUT_HXX

#include "FGCommonInput.hxx"
#include "FGButton.hxx"

#include <memory> // for std::unique_ptr
#include <simgear/structure/subsystem_mgr.hxx>

#include "FlightGear_js.h"

////////////////////////////////////////////////////////////////////////
// The Joystick Input Class
////////////////////////////////////////////////////////////////////////
class FGJoystickInput : public SGSubsystem,
                        FGCommonInput
{
public:
    FGJoystickInput();
    virtual ~FGJoystickInput();

    // Subsystem API.
    void init() override;
    void postinit() override;
    void reinit() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "input-joystick"; }

    static const int MAX_JOYSTICKS        = 16;
    static const int MAX_JOYSTICK_AXES    = _JS_MAX_AXES;
    static const int MAX_JOYSTICK_BUTTONS = 32;

private:
    /**
     * @brief computeDeviceIndexName - compute the name including the index, based
     * on the number of identically named devices. This is used to allow multiple
     * different files for identical hardware, especially throttles
     * @param name - the base joystick name
     * @param lastIndex - don't check names at this index or above. Needed to
     * ensure we only check as far as the joystick we are currently processing
     * @return
     */
    std::string computeDeviceIndexName(const std::string &name, int lastIndex) const;

    void _remove(bool all);
    SGPropertyNode_ptr status_node;

    /**
     * Settings for a single joystick axis.
     */
    struct axis {
        axis ();
        virtual ~axis ();
        float last_value;
        float tolerance;
        binding_list_t bindings[KEYMOD_MAX];
        float low_threshold;
        float high_threshold;
        FGButton low;
        FGButton high;
        float interval_sec, delay_sec, release_delay_sec;
        double last_dt;
    };

    /**
     * Settings for a joystick.
     */
    struct joystick {
      joystick ();
      virtual ~joystick ();
      int jsnum;
      std::unique_ptr<jsJoystick> plibJS;
      int naxes;
      int nbuttons;
      axis * axes;
      FGButton * buttons;
      bool predefined;
      bool initializing = true;
      bool initialized = false;
      float values[MAX_JOYSTICK_AXES];
      double init_dt = 0.0f;

      void clearAxesAndButtons();
    };

    joystick joysticks[MAX_JOYSTICKS];
    void updateJoystick(int index, joystick* joy, double dt);
};

#endif
