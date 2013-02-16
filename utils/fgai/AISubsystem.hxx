// Copyright (C) 2009 - 2012  Mathias Froehlich
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

#ifndef AISubsystem_hxx
#define AISubsystem_hxx

#include <list>
#include <simgear/structure/SGWeakReferenced.hxx>
#include <simgear/timing/timestamp.hxx>

namespace fgai {

class AIObject;

class AISubsystem : public SGWeakReferenced {
public:
    AISubsystem()
    { }
    virtual ~AISubsystem()
    { }
    
    virtual void update(AIObject& object, const SGTimeStamp& dt) = 0;
};

class AISubsystemGroup : public AISubsystem {
public:
    AISubsystemGroup()
    { }
    virtual ~AISubsystemGroup()
    { }
    
    virtual void update(AIObject& object, const SGTimeStamp& dt)
    {
        for (SubsystemList::iterator i = _subsystemList.begin();
             i != _subsystemList.end(); ++i) {
            (*i)->update(object, dt);
        }
    }

private:
    typedef std::list<SGSharedPtr<AISubsystem> > SubsystemList;
    SubsystemList _subsystemList;
};

} // namespace fgai

#endif
