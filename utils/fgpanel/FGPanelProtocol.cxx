//
//  Written and (c) Torsten Dreyer - Torsten(at)t3r_dot_de
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef WIN32
#define strtof strtod
#endif

#include "FGPanelProtocol.hxx"
#include "ApplicationProperties.hxx"
#include <simgear/io/sg_socket.hxx>
#include <simgear/misc/strutils.hxx>

using namespace std;

class PropertySetter {
public:
  PropertySetter( SGPropertyNode_ptr node ) : _node(node) {}
  virtual void setValue( const char * value ) = 0;
  virtual ~PropertySetter() {};
protected:
  SGPropertyNode_ptr _node;
};

class BoolPropertySetter : public PropertySetter {
public:
  BoolPropertySetter( SGPropertyNode_ptr node ) : PropertySetter(node) {}
  virtual void setValue( const char * value ) {
    _node->setBoolValue( atoi( value ) != 0 );
  }
};

class IntPropertySetter : public PropertySetter {
public:
  IntPropertySetter( SGPropertyNode_ptr node ) : PropertySetter(node) {}
  virtual void setValue( const char * value ) {
    _node->setIntValue( atol( value ) );
  }
};

class FloatPropertySetter : public PropertySetter {
public:
  FloatPropertySetter( SGPropertyNode_ptr node ) : PropertySetter(node) {}
  virtual void setValue( const char * value ) {
    _node->setFloatValue( strtof( value, NULL ) );
  }
};

class DoublePropertySetter : public PropertySetter {
public:
  DoublePropertySetter( SGPropertyNode_ptr node ) : PropertySetter(node) {}
  virtual void setValue( const char * value ) {
    _node->setDoubleValue( strtod( value, NULL ) );
  }
};

class StringPropertySetter : public PropertySetter {
public:
  StringPropertySetter( SGPropertyNode_ptr node ) : PropertySetter(node) {}
  virtual void setValue( const char * value ) {
    _node->setStringValue( value );
  }
};

FGPanelProtocol::FGPanelProtocol( SGPropertyNode_ptr aRoot )
  : SGSubsystem(),
   root(aRoot),
   io(NULL)
{
  SGPropertyNode_ptr outputNode = root->getNode( "protocol/generic/output" );
  if( outputNode ) {
    vector<SGPropertyNode_ptr> chunks = outputNode->getChildren( "chunk" );
    for( vector<SGPropertyNode_ptr>::size_type i = 0; i < chunks.size(); i++ ) {
      SGPropertyNode_ptr chunk = chunks[i];

      SGPropertyNode_ptr nodeNode = chunk->getNode("node", false );
      if( nodeNode == NULL )
        continue;

      SGPropertyNode_ptr node = ApplicationProperties::Properties->getNode( nodeNode->getStringValue(), true );

      string type = "";
      SGPropertyNode_ptr typeNode = chunk->getNode( "type", false );
      if( typeNode != NULL ) type = typeNode->getStringValue();
      if( type == "float" ) {
        propertySetterVector.push_back( new FloatPropertySetter( node ) );
      } else if( type == "double" || type == "fixed" ) {
        propertySetterVector.push_back( new DoublePropertySetter( node ) );
      } else if( type == "bool" || type == "boolean" ) {
        propertySetterVector.push_back( new BoolPropertySetter( node ) );
      } else if( type == "string" ) {
        propertySetterVector.push_back( new StringPropertySetter( node ) );
      } else {
        propertySetterVector.push_back( new IntPropertySetter( node ) );
      }
    }
  }
}

FGPanelProtocol::~FGPanelProtocol()
{
  for( PropertySetterVector::size_type i = 0; i < propertySetterVector.size(); i++ )
    delete propertySetterVector[i];
}

void FGPanelProtocol::update( double dt )
{
  char buf[2][8192];

  if( io == NULL )
    return;

  // read all available lines, keep last one
  int Page = 0;
  bool HaveData = false;
  while ( io->readline( buf[Page], sizeof(buf[Page])-1 ) > 0 )
  {
      HaveData = true;
      Page ^= 1;
  }

  if ( HaveData ) {
    // process most recent line of data
    Page ^= 1;
    buf[Page][sizeof(buf[Page])-1] = 0;
    vector<string> tokens = simgear::strutils::split( buf[Page], "," );
    for( vector<string>::size_type i = 0; i < tokens.size(); i++ ) {
      if( i < propertySetterVector.size() )
        propertySetterVector[i]->setValue( tokens[i].c_str() );
    }
  }
}

void FGPanelProtocol::init()
{
  SGPropertyNode_ptr listenNode = root->getNode( "listen" );
  if( listenNode == NULL ) {
    return;
  }

  string hostname = listenNode->getNode( "host", true )->getStringValue();
  string port = listenNode->getNode( "port", true )->getStringValue();
  string style = listenNode->getNode( "style", true )->getStringValue();

  if( io != NULL )
    delete io;

  io = new SGSocket( hostname, port, style );

  if( !io->open( SG_IO_IN ) ) {
    cerr << "can't open socket " << style << ":" << hostname << ":" << port << endl;
  }
}

void FGPanelProtocol::reinit()
{
  init();
}
