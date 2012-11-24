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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AIManager.hxx"

#include <cassert>

#include "HLAAirVehicleClass.hxx"
#include "HLAAircraftClass.hxx"
#include "HLABaloonClass.hxx"
#include "HLAMPAircraftClass.hxx"

#include "AIObject.hxx"

namespace fgai {

AIManager::AIManager() :
    _maxStep(SGTimeStamp::fromSecMSec(0, 200))
{
    // Set sensible defaults
    setFederationExecutionName("rti:///FlightGear");
    setFederateType("AIFederate");
    /// The hla ai module is running ahead of the simulation time of the federation.
    /// This way we are sure that all required data has arrived at the client when it is needed.
    /// This is the amount of simulation time the ai module leads the federations simulation time.
    setLeadTime(SGTimeStamp::fromSec(10));
    setTimeConstrainedByLocalClock(true);
}

AIManager::~AIManager()
{
}

simgear::HLAObjectClass*
AIManager::createObjectClass(const std::string& name)
{
    // Just there for demonstration.
    if (name == "MPAircraft")
        return new HLAMPAircraftClass(name, this);

    // These should be the future objects
    // The air vehicle should be the one an atc looks at
    if (name == "AirVehicle")
        return new HLAAirVehicleClass(name, this);
    // An aircraft with wings and that
    if (name == "Aircraft")
        return new HLAAircraftClass(name, this);
    // A hot air baloon ...
    if (name == "Baloon")
        return new HLABaloonClass(name, this);

    return 0;
}

bool
AIManager::init()
{
    if (!simgear::HLAFederate::init())
        return false;

    SGTimeStamp federateTime;
    queryFederateTime(federateTime);
    _simTime = federateTime + getLeadTime();

    _pager.start();

    return true;
}

bool
AIManager::update()
{
    // Mark newly requested paged nodes with the current simulation time.
    _pager.setUseStamp((unsigned)_simTime.toSecs());

    while (!_initObjectList.empty()) {
        assert(_currentObject.empty());
        _currentObject.splice(_currentObject.end(), _initObjectList, _initObjectList.begin());
        _currentObject.front()->init(*this);
        // If it did not reschedule itself, immediately delete it
        if (_currentObject.empty())
            continue;
        _currentObject.front()->shutdown(*this);
        _currentObject.clear();
    }

    // Find the first time slot we have anything scheduled for
    TimeStampObjectListIteratorMap::iterator i;
    i = _timeStampObjectListIteratorMap.begin();

    if (i == _timeStampObjectListIteratorMap.end() || _simTime + _maxStep < i->first) {
        // If the time slot is too far away, do a _maxStep time advance.

        _simTime += _maxStep;

    } else {
        // Process the list object updates scheduled for this time slot

        _simTime = i->first;

        // Call the updates
        while (_objectList.begin() != i->second) {
            assert(_currentObject.empty());
            _currentObject.splice(_currentObject.end(), _objectList, _objectList.begin());
            _currentObject.front()->update(*this, _simTime);
            // If it did not reschedule itself, immediately delete it
            if (_currentObject.empty())
                continue;
            _currentObject.front()->shutdown(*this);
            _currentObject.clear();
        }

        // get rid of the null element
        assert(!_objectList.front().valid());
        _objectList.pop_front();

        // The timestep has passed now
        _timeStampObjectListIteratorMap.erase(i);
    }
    
    if (!timeAdvance(_simTime - getLeadTime()))
        return false;

    // Expire bounding volume nodes older than 120 seconds
    _pager.update(120);
    
    return true;
}

bool
AIManager::shutdown()
{
    // don't care anmore
    _timeStampObjectListIteratorMap.clear();
    // Nothing has ever happened with these, just get rid of them
    _initObjectList.clear();
    // Call shutdown on them
    while (!_objectList.empty()) {
        SGSharedPtr<AIObject> object;
        object.swap(_objectList.front());
        _objectList.pop_front();
        if (!object.valid())
            continue;
        object->shutdown(*this);
    }

    // Then do the hla shutdown part
    if (!simgear::HLAFederate::shutdown())
        return false;

    // Expire bounding volume nodes
    _pager.update(0);
    _pager.stop();

    return true;
}

void
AIManager::insert(const SGSharedPtr<AIObject>& object)
{
    if (!object.valid())
        return;
    /// Note that this iterator is consistently spliced through the various lists,
    /// This must stay stable.
    object->_objectListIterator = _initObjectList.insert(_initObjectList.end(), object);
}

void
AIManager::schedule(AIObject& object, const SGTimeStamp& simTime)
{
    if (simTime <= _simTime)
        return;
    if (_currentObject.empty())
        return;

    TimeStampObjectListIteratorMap::iterator i;
    i = _timeStampObjectListIteratorMap.lower_bound(simTime);
    if (i == _timeStampObjectListIteratorMap.end()) {
        ObjectList::iterator j = _objectList.insert(_objectList.end(), 0);
        typedef TimeStampObjectListIteratorMap::value_type value_type;
        i = _timeStampObjectListIteratorMap.insert(value_type(simTime, j)).first;
    } else if (i->first != simTime) {
        if (i == _timeStampObjectListIteratorMap.begin()) {
            ObjectList::iterator j = _objectList.insert(_objectList.begin(), 0);
            typedef TimeStampObjectListIteratorMap::value_type value_type;
            i = _timeStampObjectListIteratorMap.insert(value_type(simTime, j)).first;
        } else {
            --i;
            ObjectList::iterator k = i->second;
            ObjectList::iterator j = _objectList.insert(++k, 0);
            typedef TimeStampObjectListIteratorMap::value_type value_type;
            i = _timeStampObjectListIteratorMap.insert(value_type(simTime, j)).first;
        }
    }
    // Note that the iterator stays stable
    _objectList.splice(i->second, _currentObject, _currentObject.begin());
}

const AIBVHPager&
AIManager::getPager() const
{
    return _pager;
}

AIBVHPager&
AIManager::getPager()
{
    return _pager;
}

} // namespace fgai
