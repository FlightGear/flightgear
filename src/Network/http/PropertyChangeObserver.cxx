// PropertyChangeObserver.cxx -- Watch properties for changes
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

#include "PropertyChangeObserver.hxx"

#include <Main/fg_props.hxx>
using std::string;
namespace flightgear {
namespace http {



PropertyChangeObserver::PropertyChangeObserver()
{
}

PropertyChangeObserver::~PropertyChangeObserver()
{
  //
}

void PropertyChangeObserver::check()
{

  for (Entries_t::iterator it = _entries.begin(); it != _entries.end(); ++it) {
    if (!(*it)->_node.isShared()) {
      // node is no longer used but by us - remove the entry
      it = _entries.erase(it);
      continue;
    }

    if(!(*it)->_changed ) {
      (*it)->_changed = (*it)->_prevValue != (*it)->_node->getStringValue();
      if ((*it)->_changed)
        (*it)->_prevValue = (*it)->_node->getStringValue();

    }
  }
}

void PropertyChangeObserver::uncheck()
{
  for (Entries_t::iterator it = _entries.begin(); it != _entries.end(); ++it) {
    (*it)->_changed = false;
  }
}

const SGPropertyNode_ptr PropertyChangeObserver::addObservation( const string propertyName)
{
  for (Entries_t::iterator it = _entries.begin(); it != _entries.end(); ++it) {
    if (propertyName == (*it)->_node->getPath(true) ) {
      // if a new observer is added to a property, mark it as changed to ensure the observer
      // gets notified on initial call. This also causes a notification for all other observers of this
      // property.
      (*it)->_changed = true;
      return (*it)->_node;
    }
  }

  try {
    PropertyChangeObserverEntryRef entry = new PropertyChangeObserverEntry();
    entry->_node = fgGetNode( propertyName, true );
    _entries.push_back( entry );
    return entry->_node;
  }
  catch( string & s ) {
    SG_LOG(SG_NETWORK,SG_WARN,"httpd: can't observer '" << propertyName << "'. Invalid name." );
  }

  SGPropertyNode_ptr empty;
  return empty;
}

bool PropertyChangeObserver::isChangedValue(const SGPropertyNode_ptr node)
{
  for (Entries_t::iterator it = _entries.begin(); it != _entries.end(); ++it) {
    PropertyChangeObserverEntryRef entry = *it;

    if( entry->_node == node && entry->_changed ) {
      return true;
    }
  }

  return false;
}

}  // namespace http
}  // namespace flightgear
