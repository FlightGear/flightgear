// FGMacOSXEventInput.hxx -- handle event driven input devices for Mac OS X
//
// Written by Tatsuhiro Nishioka, started Aug. 2009.
//
// Copyright (C) 2009 Tasuhiro Nishioka, tat <dot> fgmacosx <at> gmail <dot> com
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

#ifndef __FGMACOSXEVENTINPUT_HXX_
#define __FGMACOSXEVENTINPUT_HXX_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "FGEventInput.hxx"

class FGMacOSXEventInputPrivate;

struct FGMacOSXEventData : public FGEventData {
  FGMacOSXEventData(std::string name, double value, double dt, int modifiers) :
    FGEventData(value, dt, modifiers), name(name) {}
  std::string name;
};


//
// Mac OS X specific FGEventInput
//
class FGMacOSXEventInput : public FGEventInput {
public:
    FGMacOSXEventInput();

  virtual ~FGMacOSXEventInput();
  virtual void update(double dt);
  virtual void init();
    virtual void postinit();
    virtual void shutdown();
private:
    friend class FGMacOSXEventInputPrivate;
    
  std::unique_ptr<FGMacOSXEventInputPrivate> d;
};

#endif
