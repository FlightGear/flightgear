#include "SimpleDOM.hxx"
using std::string;

namespace flightgear {
namespace http {

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

DOMNode * DOMNode::addChild( DOMElement * child )
{
  _children.push_back( child );
  return dynamic_cast<DOMNode*>(child);
}

DOMNode * DOMNode::setAttribute( const string & name, const string & value )
{
  _attributes[name] = value;
  return this;
}

}
}
