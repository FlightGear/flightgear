// -*- coding: utf-8 -*-
//
// contacts.hxx --- FlightGear classes holding add-on contact metadata
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

#ifndef FG_ADDON_CONTACTS_HXX
#define FG_ADDON_CONTACTS_HXX

#include <string>

#include <simgear/structure/SGReferenced.hxx>

#include "addon_fwd.hxx"

namespace nasal
{
  class Hash;                   // forward declaration
};

namespace flightgear
{

namespace addons
{

enum class ContactType {
  author,
  maintainer
};

// Class used to store info about an author or maintainer (possibly also a
// mailing-list, things like that)
class Contact : public SGReferenced
{
public:
  Contact(ContactType type, std::string name, std::string email = "",
          std::string url = "");
  virtual ~Contact() = default;

  ContactType getType() const;
  std::string getTypeString() const;

  std::string getName() const;
  void setName(const std::string& name);

  std::string getEmail() const;
  void setEmail(const std::string& email);

  std::string getUrl() const;
  void setUrl(const std::string& url);

  static void setupGhost(nasal::Hash& addonsModule);

private:
  const ContactType _type;
  std::string _name;
  std::string _email;
  std::string _url;
};

class Author : public Contact
{
public:
  Author(std::string name, std::string email = "", std::string url = "");

  static void setupGhost(nasal::Hash& addonsModule);
};

class Maintainer : public Contact
{
public:
  Maintainer(std::string name, std::string email = "", std::string url = "");

  static void setupGhost(nasal::Hash& addonsModule);
};

// ***************************************************************************
// *                              contact_traits                             *
// ***************************************************************************

template <typename T>
struct contact_traits;

template<>
struct contact_traits<Author>
{
  using contact_type = Author;
  using strong_ref = AuthorRef;

  static std::string xmlNodeName()
  {
    return "author";
  }
};

template<>
struct contact_traits<Maintainer>
{
  using contact_type = Maintainer;
  using strong_ref = MaintainerRef;

  static std::string xmlNodeName()
  {
    return "maintainer";
  }
};

} // of namespace addons

} // of namespace flightgear

#endif  // of FG_ADDON_CONTACTS_HXX
