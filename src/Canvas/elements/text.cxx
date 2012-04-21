// A text on the canvas
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

#include "text.hxx"
#include <osgText/Text>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

namespace canvas
{

  //----------------------------------------------------------------------------
  Text::Text(SGPropertyNode* node):
    Element(node, COLOR | COLOR_FILL | BOUNDING_BOX),
    _text( new osgText::Text )
  {
    _drawable = _text;
    _text->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);

    // font size and property node
    float character_size = _node->getFloatValue("size", 32);
    _text->setCharacterSize( character_size );
    _node->setFloatValue("size", character_size);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(_text);
    _transform->addChild(geode);

    _node->tie
    (
      "alignment",
      SGRawValueMethods<Text, const char*>
      (
        *this,
        &Text::getAlignment,
        &Text::setAlignment
      )
    );
    _node->tie
    (
      "padding",
      SGRawValueMethods<osgText::Text, float>
      (
        *_text.get(),
        &osgText::Text::getBoundingBoxMargin,
        &osgText::Text::setBoundingBoxMargin
      )
    );
    typedef SGRawValueMethods<osgText::Text, int> TextIntMethods;
    _node->tie
    (
      "draw-mode",
      //  TEXT              = 1 default
      //  BOUNDINGBOX       = 2
      //  FILLEDBOUNDINGBOX = 4
      //  ALIGNMENT         = 8
      TextIntMethods
      (
        *_text.get(),
        (TextIntMethods::getter_t)&osgText::Text::getDrawMode,
        (TextIntMethods::setter_t)&osgText::Text::setDrawMode
      )
    );
  }

  //----------------------------------------------------------------------------
  Text::~Text()
  {

  }

  //----------------------------------------------------------------------------
  void Text::setFont(const char* name)
  {
    _text->setFont( getFont(name) );
  }

  //----------------------------------------------------------------------------
  void Text::setAlignment(const char* align)
  {
    const std::string align_string(align);
    if( 0 ) return;
#define ENUM_MAPPING(enum_val, string_val) \
    else if( align_string == string_val )\
      _text->setAlignment( osgText::Text::enum_val );
#include "text-alignment.hxx"
#undef ENUM_MAPPING
    else
      _text->setAlignment(osgText::Text::LEFT_BASE_LINE);
  }

  //----------------------------------------------------------------------------
  const char* Text::getAlignment() const
  {
    switch( _text->getAlignment() )
    {
#define ENUM_MAPPING(enum_val, string_val) \
      case osgText::Text::enum_val:\
        return string_val;
#include "text-alignment.hxx"
#undef ENUM_MAPPING
      default:
        return "unknown";
    }
  }

  //----------------------------------------------------------------------------
  void Text::childChanged(SGPropertyNode* child)
  {
    if( child->getNameString() == "text" )
      _text->setText(child->getStringValue());
    else if( child->getNameString() == "size" )
      _text->setCharacterSize( child->getFloatValue() );
    else if( child->getNameString() == "font" )
      setFont( child->getStringValue() );
    else
      return;

    _attributes_dirty |= BOUNDING_BOX;
  }

  //----------------------------------------------------------------------------
  void Text::colorChanged(const osg::Vec4& color)
  {
    _text->setColor(color);
  }

  //----------------------------------------------------------------------------
  void Text::colorFillChanged(const osg::Vec4& color)
  {
    _text->setBoundingBoxColor(color);
  }

  //----------------------------------------------------------------------------
  Text::font_ptr Text::getFont(const std::string& name)
  {
    SGPath path = globals->resolve_ressource_path("Fonts/" + name);
    if( path.isNull() )
    {
      SG_LOG
      (
        SG_GL,
        SG_ALERT,
        "canvas::Text: No such font: " << name
      );
      return font_ptr();
    }

    SG_LOG
    (
      SG_GL,
      SG_INFO,
      "canvas::Text: using font file " << path.str()
    );

    font_ptr font = osgText::readFontFile(path.c_str());
    if( !font )
      SG_LOG
      (
        SG_GL,
        SG_ALERT,
        "canvas::Text: Failed to open font file " << path.c_str()
      );

    return font;
  }

} // namespace canvas
