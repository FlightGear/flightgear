// Canvas with 2D rendering api
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#include "canvas_mgr.hxx"
#include "canvas.hxx"

#include <Main/fg_props.hxx>

#include <osg/Camera>
#include <osg/Texture2D>

#include <stdexcept>
#include <string>

//------------------------------------------------------------------------------
CanvasMgr::CanvasMgr():
  _props( fgGetNode("/canvas", true) )
{

}

//------------------------------------------------------------------------------
CanvasMgr::~CanvasMgr()
{

}

//------------------------------------------------------------------------------
void CanvasMgr::init()
{
  _props->addChangeListener(this);
  triggerChangeRecursive(_props);
}

//------------------------------------------------------------------------------
void CanvasMgr::reinit()
{

}

//------------------------------------------------------------------------------
void CanvasMgr::shutdown()
{
  _props->removeChangeListener(this);
}

//------------------------------------------------------------------------------
void CanvasMgr::bind()
{
}

//------------------------------------------------------------------------------
void CanvasMgr::unbind()
{
}

//------------------------------------------------------------------------------
void CanvasMgr::update(double delta_time_sec)
{
 for( size_t i = 0; i < _canvases.size(); ++i )
   _canvases[i].update(delta_time_sec);
}

//------------------------------------------------------------------------------
void CanvasMgr::childAdded( SGPropertyNode * parent,
                            SGPropertyNode * child )
{
  if( parent != _props )
    return;

  if( child->getNameString() == "texture" )
    textureAdded(child);
  else
    std::cout << "CanvasMgr::childAdded: " << child->getPath() << std::endl;
}

//------------------------------------------------------------------------------
void CanvasMgr::childRemoved( SGPropertyNode * parent,
                              SGPropertyNode * child )
{
  if( parent != _props )
    return;

  std::cout << "CanvasMgr::childRemoved: " << child->getPath() << std::endl;
}

//------------------------------------------------------------------------------
void CanvasMgr::textureAdded(SGPropertyNode* node)
{
  size_t index = node->getIndex();

  if( index >= _canvases.size() )
  {
    if( index > _canvases.size() )
      SG_LOG(SG_GL, SG_WARN, "Skipping unused texture slot(s)!");
    SG_LOG(SG_GL, SG_INFO, "Add new texture[" << index << "]");

    _canvases.resize(index + 1);
    _canvases[index];
  }
  else
  {
    SG_LOG(SG_GL, SG_WARN, "texture[" << index << "] already exists!");
  }

  _canvases[index].reset(node);
}

//------------------------------------------------------------------------------
void CanvasMgr::triggerChangeRecursive(SGPropertyNode* node)
{
  node->getParent()->fireChildAdded(node);

  if( node->nChildren() == 0 && node->getType() != simgear::props::NONE )
    return node->fireValueChanged();

  for( int i = 0; i < node->nChildren(); ++i )
    triggerChangeRecursive( node->getChild(i) );
}

//------------------------------------------------------------------------------
template<class T>
T CanvasMgr::getParam(const SGPropertyNode* node, const char* prop)
{
  const SGPropertyNode* child = node->getChild(prop);
  if( !child )
    throw std::runtime_error(std::string("Missing property ") + prop);
  return getValue<T>(child);
}
