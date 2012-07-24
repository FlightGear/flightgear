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
#include <Canvas/property_helper.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <osgText/Text>

namespace canvas
{
  class Text::TextOSG:
    public osgText::Text
  {
    public:
      osg::Vec2 handleHit(float x, float y);
  };

  //----------------------------------------------------------------------------
  osg::Vec2 Text::TextOSG::handleHit(float x, float y)
  {
    float line_height = _characterHeight + _lineSpacing;

    // TODO check with align other than TOP
    float first_line_y = -0.5 * _lineSpacing;//_offset.y() - _characterHeight;
    size_t line = std::max<int>(0, (y - first_line_y) / line_height);

    if( _textureGlyphQuadMap.empty() )
      return osg::Vec2(-1, -1);

    // TODO check when it can be larger
    assert( _textureGlyphQuadMap.size() == 1 );

    const GlyphQuads& glyphquad = _textureGlyphQuadMap.begin()->second;
    const GlyphQuads::Glyphs& glyphs = glyphquad._glyphs;
    const GlyphQuads::Coords2& coords = glyphquad._coords;
    const GlyphQuads::LineNumbers& line_numbers = glyphquad._lineNumbers;

    const float HIT_FRACTION = 0.6;
    const float character_width = getCharacterHeight()
                                * getCharacterAspectRatio();

    y = (line + 0.5) * line_height;

    bool line_found = false;
    for(size_t i = 0; i < line_numbers.size(); ++i)
    {
      if( line_numbers[i] != line )
      {
        if( !line_found )
        {
          if( line_numbers[i] < line )
            // Wait for the correct line...
            continue;

          // We have already passed the correct line -> It's empty...
          return osg::Vec2(0, y);
        }

        // Next line and not returned -> not before any character
        // -> return position after last character of line
        return osg::Vec2(coords[(i - 1) * 4 + 2].x(), y);
      }

      line_found = true;

      // Get threshold for mouse x position for setting cursor before or after
      // current character
      float threshold = coords[i * 4].x()
                      + HIT_FRACTION * glyphs[i]->getHorizontalAdvance()
                                     * character_width;

      if( x <= threshold )
      {
        if( i == 0 || line_numbers[i - 1] != line )
          // first character of line
          x = coords[i * 4].x();
        else if( coords[(i - 1) * 4].x() == coords[(i - 1) * 4 + 2].x() )
          // If previous character width is zero set to begin of next character
          // (Happens eg. with spaces)
          x = coords[i * 4].x();
        else
          // position at center between characters
          x = 0.5 * (coords[(i - 1) * 4 + 2].x() + coords[i * 4].x());

        return osg::Vec2(x, y);
      }
    }

    // Nothing found -> return position after last character
    return osg::Vec2
    (
      coords.back().x(),
      (_lineCount - 0.5) * line_height
    );
  }

  //----------------------------------------------------------------------------
  Text::Text(SGPropertyNode_ptr node):
    Element(node, COLOR | COLOR_FILL | BOUNDING_BOX),
    _text( new Text::TextOSG() ),
    _font_size( 0 ),
    _font_aspect( 0 )
  {
    setDrawable(_text);
    _text->setCharacterSizeMode(osgText::Text::OBJECT_COORDS);
    _text->setAxisAlignment(osgText::Text::USER_DEFINED_ROTATION);
    _text->setRotation(osg::Quat(osg::PI, osg::X_AXIS));

    _font_size = getChildDefault<float>(_node, "character-size", 32);
    _font_aspect = getChildDefault<float>(_node, "character-aspect-ratio", 1);

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
    if( _node )
    {
      _node->untie("alignment");
      _node->untie("padding");
      _node->untie("draw-mode");
    }
    _node = 0;
  }

  //----------------------------------------------------------------------------
  void Text::update(double dt)
  {
    Element::update(dt);

    if( _attributes_dirty & FONT_SIZE )
    {
      _text->setCharacterSize
      (
        _font_size->getFloatValue(),
        _font_aspect->getFloatValue()
      );

      _attributes_dirty &= ~FONT_SIZE;
    }
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
    const std::string& name = child->getNameString();

    if( name == "hit-y" )
      handleHit
      (
        _node->getFloatValue("hit-x"),
        _node->getFloatValue("hit-y")
      );
    else if( _font_size == child || _font_aspect == child )
      _attributes_dirty |= FONT_SIZE;
    else if( name == "text" )
      _text->setText
      (
        osgText::String( child->getStringValue(),
                         osgText::String::ENCODING_UTF8 )
      );
    else if( name == "max-width" )
      _text->setMaximumWidth( child->getFloatValue() );
    else if( name == "font" )
      setFont( child->getStringValue() );
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
  void Text::handleHit(float x, float y)
  {
    const osg::Vec2& pos = _text->handleHit(x, y);
    _node->setFloatValue("cursor-x", pos.x());
    _node->setFloatValue("cursor-y", pos.y());
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
