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

#ifndef AIManager_hxx
#define AIManager_hxx

#include <simgear/hla/HLAFederate.hxx>
#include "AIBVHPager.hxx"

namespace fgai {

class AIObject;

class AIManager : public simgear::HLAFederate {
public:
    AIManager();
    virtual ~AIManager();

    virtual simgear::HLAObjectClass* createObjectClass(const std::string& name);

    virtual bool init();
    virtual bool update();
    virtual bool shutdown();

    void insert(const SGSharedPtr<AIObject>& aiObject);
    void schedule(AIObject& object, const SGTimeStamp& simTime);

    const SGTimeStamp& getSimTime() const
    { return _simTime; }

    const AIBVHPager& getPager() const;
    AIBVHPager& getPager();

    /// For the list of ai object that are active
    typedef std::list<SGSharedPtr<AIObject> > ObjectList;
    /// For the schedule of the next update of an ai object
    typedef std::map<SGTimeStamp, ObjectList> TimeStampObjectListMap;

    typedef std::map<SGTimeStamp, ObjectList::iterator> TimeStampObjectListIteratorMap;

private:
    /// The current simulation time
    SGTimeStamp _simTime;
    /// The maximum time advance step size that is taken
    SGTimeStamp _maxStep;

    /// Single element list that is used to store the currently worked on
    /// ai object. This is a helper for any work method in the object.
    ObjectList _currentObject;

    /// List of objects that are inserted and need to be initialized
    ObjectList _initObjectList;
    /// List of running and ready to use objects, is in execution order
    ObjectList _objectList;
    /// Map of insert points for specific simulation times
    TimeStampObjectListIteratorMap _timeStampObjectListIteratorMap;

    /// for paging bounding volume trees
    AIBVHPager _pager;
};

} // namespace fgai

#endif
