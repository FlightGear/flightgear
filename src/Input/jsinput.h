// jsinput.h -- wait for and identify input from joystick
//
// Written by Tony Peden, started May 2001
//
// Copyright (C) 2001  Tony Peden (apeden@earthlink.net)
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



#ifndef _JSINPUT_H
#define _JSINPUT_H

#include "jssuper.h"

class jsInput {
  private:
    jsSuper *jss;
    bool pretty_display;
    float axes[_JS_MAX_AXES];
    float axes_iv[MAX_JOYSTICKS][_JS_MAX_AXES];
    int button_iv[MAX_JOYSTICKS];

    int joystick,axis,button;
    bool axis_positive;

    float axis_threshold;

  public:
    jsInput(jsSuper *jss);
    ~jsInput(void);

    inline void displayValues(bool bb) { pretty_display=bb; }

    int getInput(void);
    void findDeadBand(void);

    inline int getInputJoystick(void) { return joystick; }
    inline int getInputAxis(void)     { return axis; }
    inline int getInputButton(void)   { return button; }
    inline bool getInputAxisPositive(void) { return axis_positive; }

    inline float getReturnThreshold(void) { return axis_threshold; }
    inline void setReturnThreshold(float ff)
              { if(fabs(ff) <= 1.0) axis_threshold=ff; }
};


#endif
