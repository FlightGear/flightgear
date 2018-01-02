// -*- coding: utf-8 -*-
//
// contacts.cxx --- FlightGear classes holding add-on contact metadata
// Copyright (C) 2018  Florent Rougon
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <string>
#include <utility>

#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

#include "addon_fwd.hxx"
#include "contacts.hxx"

using std::string;
using simgear::enumValue;

namespace flightgear
{

namespace addons
{

// ***************************************************************************
// *                                 Contact                                 *
// ***************************************************************************

Contact::Contact(ContactType type, string name, string email, string url)
  : _type(type),
    _name(std::move(name)),
    _email(std::move(email)),
    _url(std::move(url))
{ }

ContactType Contact::getType() const
{ return _type; }

string Contact::getTypeString() const
{
  switch (getType()) {
  case ContactType::author:
    return "author";
  case ContactType::maintainer:
    return "maintainer";
  default:
    throw sg_error("unexpected value for member of "
                   "flightgear::addons::ContactType: " +
                   std::to_string(enumValue(getType())));
  }
}

string Contact::getName() const
{ return _name; }

void Contact::setName(const string& name)
{ _name = name; }

string Contact::getEmail() const
{ return _email; }

void Contact::setEmail(const string& email)
{ _email = email; }

string Contact::getUrl() const
{ return _url; }

void Contact::setUrl(const string& url)
{ _url = url; }

// Static method
void Contact::setupGhost(nasal::Hash& addonsModule)
{
  nasal::Ghost<ContactRef>::init("addons.Contact")
    .member("name", &Contact::getName)
    .member("email", &Contact::getEmail)
    .member("url", &Contact::getUrl);
}

// ***************************************************************************
// *                                 Author                                  *
// ***************************************************************************

Author::Author(string name, string email, string url)
  : Contact(ContactType::author, name, email, url)
{ }

// Static method
void Author::setupGhost(nasal::Hash& addonsModule)
{
  nasal::Ghost<AuthorRef>::init("addons.Author")
    .bases<ContactRef>();
}

// ***************************************************************************
// *                               Maintainer                                *
// ***************************************************************************

Maintainer::Maintainer(string name, string email, string url)
  : Contact(ContactType::maintainer, name, email, url)
{ }

// Static method
void Maintainer::setupGhost(nasal::Hash& addonsModule)
{
  nasal::Ghost<MaintainerRef>::init("addons.Maintainer")
    .bases<ContactRef>();
}

} // of namespace addons

} // of namespace flightgear
