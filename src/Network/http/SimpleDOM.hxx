// SimpleDOM.hxx -- poor man's DOM
//
// Written by Torsten Dreyer
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

#ifndef __FG_SIMPLE_DOM_HXX
#define __FG_SIMPLE_DOM_HXX

#include <string>
#include <map>
#include <vector>

namespace flightgear {
namespace http {

class DOMElement {
public:
  virtual ~DOMElement() {}
  virtual std::string render() const = 0;
};

class DOMTextElement : public DOMElement {
public:
  DOMTextElement( const std::string & text ) : _text(text) {}
  virtual std::string render() const { return _text; }

private:
  std::string _text;
};

class DOMNode : public DOMElement {
public:
  DOMNode( const std::string & name ) : _name(name) {}
  virtual ~DOMNode();

  virtual std::string render() const;
  virtual DOMNode * addChild( DOMElement * child );
  virtual DOMNode * setAttribute( const std::string & name, const std::string & value );
protected:
  std::string _name;
  typedef std::vector<const DOMElement*> Children_t;
  typedef std::map<std::string,std::string> Attributes_t;
  Children_t   _children;
  Attributes_t _attributes;
};

}
}
#endif // __FG_SIMPLE_DOM_HXX
