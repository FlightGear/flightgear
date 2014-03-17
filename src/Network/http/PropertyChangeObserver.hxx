// PropertyChangeObserver.hxx -- Watch properties for changes
//
// Written by Torsten Dreyer, started April 2014.
//
// Copyright (C) 2014  Torsten Dreyer
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

#ifndef PROPERTYCHANGEOBSERVER_HXX_
#define PROPERTYCHANGEOBSERVER_HXX_

#include <simgear/props/props.hxx>
#include <string>
#include <vector>

namespace flightgear {
namespace http {

struct PropertyChangeObserverEntry : public SGReferenced {
  PropertyChangeObserverEntry()
      : _changed(true)
  {
  }
  SGPropertyNode_ptr _node;
  std::string _prevValue;
  bool _changed;
};

typedef SGSharedPtr<PropertyChangeObserverEntry> PropertyChangeObserverEntryRef;

class PropertyChangeObserver {
public:
  PropertyChangeObserver();
  virtual ~PropertyChangeObserver();

  const SGPropertyNode_ptr addObservation( const std::string propertyName);
  bool isChangedValue(const SGPropertyNode_ptr node);

  void check();
  void uncheck();

  void clear() { _entries.clear(); }

private:
  typedef std::vector<PropertyChangeObserverEntryRef> Entries_t;
  Entries_t _entries;

};
}  // namespace http
}  // namespace flightgear

#endif /* PROPERTYCHANGEOBSERVER_HXX_ */
