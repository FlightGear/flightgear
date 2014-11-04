// PropertyUriHandler.cxx -- a web form interface to the property tree
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


#include "PropertyUriHandler.hxx"
#include <Main/fg_props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/strutils.hxx>
#include <boost/lexical_cast.hpp>

#include <vector>
#include <map>
#include <algorithm> 
#include <cstring>

using std::string;
using std::map;
using std::vector;

namespace flightgear {
namespace http {

// copied from http://stackoverflow.com/a/24315631
static void ReplaceAll(std::string & str, const std::string & from, const std::string & to) 
{
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

static const std::string specialChars[][2] = {
  { "&",  "&amp;" },
  { "\"", "&quot;" },
  { "'", "&#039;" },
  { "<", "&lt;" },
  { ">", "&gt;" },
};

static inline std::string htmlSpecialChars( const std::string & s )
{
  string reply = s;
  for( size_t i = 0; i < sizeof(specialChars)/sizeof(specialChars[0]); ++i )
    ReplaceAll( reply, specialChars[i][0], specialChars[i][1] );
  return reply;
}

class DOMElement {
public:
  virtual ~DOMElement() {}
  virtual std::string render() const = 0;
};

class DOMTextElement : public DOMElement {
public:
  DOMTextElement( const string & text ) : _text(text) {}
  virtual std::string render() const { return _text; }

private:
  string _text;
};


class DOMNode : public DOMElement {
public:
  DOMNode( const string & name ) : _name(name) {}
  virtual ~DOMNode();

  virtual std::string render() const;
  virtual void addChild( DOMElement * child );
  virtual void setAttribute( const string & name, const string & value );
protected:
  string _name;
  typedef vector<const DOMElement*> Children_t;
  typedef map<string,string> Attributes_t;
  Children_t   _children;
  Attributes_t _attributes;
};

DOMNode::~DOMNode()
{
  for( Children_t::const_iterator it = _children.begin(); it != _children.end(); ++it ) 
    delete *it;
}

string DOMNode::render() const
{
  string reply;
  reply.append( "<" ).append( _name );
  for( Attributes_t::const_iterator it = _attributes.begin(); it != _attributes.end(); ++it ) {
    reply.append( " " );
    reply.append( it->first );
    reply.append( "=\"" );
    reply.append( it->second );
    reply.append( "\"" );
  }

  if( _children.empty() ) {
    reply.append( " />\r" );
    return reply;
  } else {
    reply.append( ">" );
  }

  for( Children_t::const_iterator it = _children.begin(); it != _children.end(); ++it ) {
    reply.append( (*it)->render() );
  }

  reply.append( "</" ).append( _name ).append( ">\r" );

  return reply;
}

void DOMNode::addChild( DOMElement * child )
{
  _children.push_back( child );
}

void DOMNode::setAttribute( const string & name, const string & value )
{
  _attributes[name] = value;
}

class SortedChilds : public simgear::PropertyList {
public:
  SortedChilds( SGPropertyNode_ptr node ) {
    for (int i = 0; i < node->nChildren(); i++)
      push_back(node->getChild(i));
    std::sort(begin(), end(), CompareNodes());
  }
private:
  class CompareNodes {
  public:
    bool operator() (const SGPropertyNode *a, const SGPropertyNode *b) const {
        int r = strcmp(a->getName(), b->getName());
        return r ? r < 0 : a->getIndex() < b->getIndex();
    }
  };
};

static const char * getPropertyTypeString( simgear::props::Type type )
{
  switch( type ) {
    case simgear::props::NONE: return "";
    case simgear::props::ALIAS: return "alias";
    case simgear::props::BOOL: return "bool";
    case simgear::props::INT: return "int";
    case simgear::props::LONG: return "long";
    case simgear::props::FLOAT: return "float";
    case simgear::props::DOUBLE: return "double";
    case simgear::props::STRING: return "string";
    case simgear::props::UNSPECIFIED: return "unspecified";
    case simgear::props::EXTENDED: return "extended";
    case simgear::props::VEC3D: return "vec3d";
    case simgear::props::VEC4D: return "vec4d";
    default: return "?";
  }
}

DOMElement * createHeader( const string & prefix, const string & propertyPath )
{
  using namespace simgear::strutils;

  string path = prefix;

  DOMNode * root = new DOMNode( "div" );
  root->setAttribute( "id", "breadcrumb" );

  DOMNode * headline = new DOMNode( "h3" );
  root->addChild( headline );
  headline->addChild( new DOMTextElement("FlightGear Property Browser") );

  DOMNode * breadcrumb = new DOMNode("ul");
  root->addChild( breadcrumb );

  DOMNode * li = new DOMNode("li");
  breadcrumb->addChild( li );
  DOMNode * a = new DOMNode("a");
  li->addChild( a );
  a->setAttribute( "href", path );
  a->addChild( new DOMTextElement( "[root]" ) );

  string_list items = split( propertyPath, "/" );
  for( string_list::iterator it = items.begin(); it != items.end(); ++it ) {
    if( (*it).empty() ) continue;
    path.append( *it ).append( "/" );

    li = new DOMNode("li");
    breadcrumb->addChild( li );
    a = new DOMNode("a");
    li->addChild( a );
    a->setAttribute( "href", path );
    a->addChild( new DOMTextElement( (*it)  ) );
  }

  return root;
}

static DOMElement * renderPropertyValueElement( SGPropertyNode_ptr node )
{
    string value = node->getStringValue();
    int len = value.length();

    if( len < 15 ) len = 15;

    DOMNode * root;

    if( len < 60 ) {
        root = new DOMNode( "input" );
        root->setAttribute( "type", "text" );
        root->setAttribute( "name", node->getDisplayName() );
        root->setAttribute( "value", htmlSpecialChars(value) );
        root->setAttribute( "size", boost::lexical_cast<std::string>( len ) );
        root->setAttribute( "maxlength", "2047" );
    } else {
        int rows = (len / 60)+1;
        int cols = 60;
        root = new DOMNode( "textarea" );
        root->setAttribute( "name", node->getDisplayName() );
        root->setAttribute( "cols", boost::lexical_cast<std::string>( cols ) );
        root->setAttribute( "rows", boost::lexical_cast<std::string>( rows ) );
        root->setAttribute( "maxlength", "2047" );
        root->addChild( new DOMTextElement( htmlSpecialChars(value) ) );
    }

    return root;
}

bool PropertyUriHandler::handleGetRequest( const HTTPRequest & request, HTTPResponse & response, Connection * connection )
{

  string propertyPath = request.Uri;

  // strip the uri prefix of our handler
  propertyPath = propertyPath.substr( getUri().size() );

  // strip the querystring
  size_t pos = propertyPath.find( '?' );
  if( pos != string::npos ) {
    propertyPath = propertyPath.substr( 0, pos-1 );
  }

  // skip trailing '/' - not very efficient but shouldn't happen too often
  while( false == propertyPath.empty() && propertyPath[ propertyPath.length()-1 ] == '/' )
    propertyPath = propertyPath.substr(0,propertyPath.length()-1);

  if( request.RequestVariables.get("submit") == "update" ) {
    // update leaf
    string value = request.RequestVariables.get("value");
    SG_LOG(SG_NETWORK,SG_INFO, "httpd: setting " << propertyPath << " to '" << value << "'" );
    try {
      fgSetString( propertyPath.c_str(), value );
    }
    catch( string & s ) { 
      SG_LOG(SG_NETWORK,SG_WARN, "httpd: setting " << propertyPath << " to '" << value << "' failed: " << s );
    }
  }

  if( request.RequestVariables.get("submit") == "set" ) {
    for( HTTPRequest::StringMap::const_iterator it = request.RequestVariables.begin(); it != request.RequestVariables.end(); ++it ) {
      if( it->first == "submit" ) continue;
      string pp = propertyPath + "/" + it->first;
      SG_LOG(SG_NETWORK,SG_INFO, "httpd: setting " << pp << " to '" << it->second << "'" );
      try {
        fgSetString( pp, it->second );
      }
      catch( string & s ) { 
        SG_LOG(SG_NETWORK,SG_WARN, "httpd: setting " << pp << " to '" << it->second << "' failed: " << s );
      }
    }
  }
  
  // build the response
  DOMNode * html = new DOMNode( "html" );
  html->setAttribute( "lang", "en" );

  DOMNode * head = new DOMNode( "head" );
  html->addChild( head );

  DOMNode * e;
  e = new DOMNode( "title" );
  head->addChild( e );
  e->addChild( new DOMTextElement( string("FlightGear Property Browser - ") + propertyPath ) );

  e = new DOMNode( "link" );
  head->addChild( e );
  e->setAttribute( "href", "/css/props.css" );
  e->setAttribute( "rel", "stylesheet" );
  e->setAttribute( "type", "text/css" );

  DOMNode * body = new DOMNode( "body" );
  html->addChild( body );

  SGPropertyNode_ptr node;
  try {
    node = fgGetNode( string("/") + propertyPath );
  }
  catch( string & s ) { 
    SG_LOG(SG_NETWORK,SG_WARN, "httpd: reading '" << propertyPath  << "' failed: " << s );
  }
  if( false == node.valid() ) {
    DOMNode * headline = new DOMNode( "h3" );
    body->addChild( headline );
    headline->addChild( new DOMTextElement( "Non-existent node requested!" ) );
    e = new DOMNode( "b" );
    e->addChild( new DOMTextElement( propertyPath ) );
    // does not exist
    body->addChild( e );
    response.StatusCode = 404;

  } else if( node->nChildren() > 0 ) {
    // Render the list of children
    body->addChild( createHeader( getUri(), propertyPath ));

    DOMNode * table = new DOMNode("table");
    body->addChild( table );

    DOMNode * tr = new DOMNode( "tr" );
    table->addChild( tr );

    DOMNode * th = new DOMNode( "th" );
    tr->addChild( th );
    th->addChild( new DOMTextElement( "&nbsp;" ) );

    th = new DOMNode( "th" );
    tr->addChild( th );
    th->addChild( new DOMTextElement( "Property" ) );
    th->setAttribute( "id", "property" );

    th = new DOMNode( "th" );
    tr->addChild( th );
    th->addChild( new DOMTextElement( "Value" ) );
    th->setAttribute( "id", "value" );

    th = new DOMNode( "th" );
    tr->addChild( th );
    th->addChild( new DOMTextElement( "Type" ) );
    th->setAttribute( "id", "type" );

    SortedChilds sortedChilds( node );
    for(SortedChilds::iterator it = sortedChilds.begin(); it != sortedChilds.end(); ++it ) {
      tr = new DOMNode( "tr" );
      table->addChild( tr );

      SGPropertyNode_ptr child = *it;
      string name = child->getDisplayName(true);

      DOMNode * td;

      // Expand Link
      td = new DOMNode("td");
      tr->addChild( td );
      td->setAttribute( "id", "expand" );
      if ( child->nChildren() > 0 ) {
        DOMNode * a = new DOMNode("a");
        td->addChild( a );
        a->setAttribute( "href", getUri() + propertyPath + "/" + name );
        a->addChild( new DOMTextElement( "(+)" ));
      }

      // Property Name
      td = new DOMNode("td");
      tr->addChild( td );
      td->setAttribute( "id", "property" );
      DOMNode * a = new DOMNode("a");
      td->addChild( a );
      a->setAttribute( "href", getUri() + propertyPath + "/" + name );
      a->addChild( new DOMTextElement( name ) );

      // Value
      td = new DOMNode("td");
      tr->addChild( td );
      td->setAttribute( "id", "value" );
      if ( child->nChildren() == 0 ) {
        DOMNode * form = new DOMNode("form");
        td->addChild( form );
        form->setAttribute( "method", "GET" );
        form->setAttribute( "action", getUri() + propertyPath );

        e = new DOMNode( "input" );
        form->addChild( e );
        e->setAttribute( "type", "submit" );
        e->setAttribute( "value", "set" );
        e->setAttribute( "name", "submit" );

        form->addChild( renderPropertyValueElement( node->getNode( name ) ) );

      } else {
        td->addChild( new DOMTextElement( "&nbsp;" ) );
      }

      // Property Type
      td = new DOMNode("td");
      tr->addChild( td );
      td->setAttribute( "id", "type" );
      td->addChild( 
        new DOMTextElement( getPropertyTypeString(node->getNode( name )->getType()) ) );

    }
  } else {
    // Render a single property
    body->addChild( createHeader( getUri(), propertyPath ));
    e = new DOMNode( "div" );
    body->addChild( e );

    e->setAttribute( "id", "currentvalue" );
    e->addChild( new DOMTextElement( "Current Value: " ) );
    e->addChild( new DOMTextElement( htmlSpecialChars(node->getStringValue()) ) );

    DOMNode * form = new DOMNode("form");
    body->addChild( form );
    form->setAttribute( "method", "GET" );
    form->setAttribute( "action", getUri() + propertyPath );

    e = new DOMNode( "input" );
    form->addChild( e );
    e->setAttribute( "type", "submit" );
    e->setAttribute( "value", "update" );
    e->setAttribute( "name", "submit" );

    form->addChild( renderPropertyValueElement( node ) );
  }

  // Send the response 
  response.Content = "<!DOCTYPE html>";
  response.Content.append( html->render() );
  delete html;
  response.Header["Content-Type"] = "text/html; charset=UTF-8";

  return true;

}

} // namespace http
} // namespace flightgear

