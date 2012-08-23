// Some helper functions for accessing the property tree
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

#include "property_helper.hxx"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include <cassert>

namespace canvas
{
  //----------------------------------------------------------------------------
  std::vector<float> splitAndConvert(const char del[], const std::string& str)
  {
    std::vector<float> values;
    size_t pos = 0;
    for(;;)
    {
      pos = str.find_first_not_of(del, pos);
      if( pos == std::string::npos )
        break;

      char *end = 0;
      float val = strtod(&str[pos], &end);
      if( end == &str[pos] || !end )
        break;

      values.push_back(val);
      pos = end - &str[0];
    }
    return values;
  }

  //----------------------------------------------------------------------------
  osg::Vec4 parseColor(std::string str)
  {
    boost::trim(str);
    osg::Vec4 color(0,0,0,1);

    if( str.empty() )
      return color;

    // #rrggbb
    if( str[0] == '#' )
    {
      const int offsets[] = {2,2,2};
      const boost::offset_separator hex_separator( boost::begin(offsets),
                                                   boost::end(offsets) );
      typedef boost::tokenizer<boost::offset_separator> offset_tokenizer;
      offset_tokenizer tokens(str.begin() + 1, str.end(), hex_separator);

      int comp = 0;
      for( offset_tokenizer::const_iterator tok = tokens.begin();
           tok != tokens.end() && comp < 4;
           ++tok, ++comp )
      {
        color._v[comp] = strtol(std::string(*tok).c_str(), 0, 16) / 255.f;
      }
    }
    // rgb(r,g,b)
    // rgba(r,g,b,a)
    else if( boost::ends_with(str, ")") )
    {
      const std::string RGB = "rgb(",
                        RGBA = "rgba(";
      size_t pos;
      if( boost::starts_with(str, RGB) )
        pos = RGB.length();
      else if( boost::starts_with(str, RGBA) )
        pos = RGBA.length();
      else
        return color;

      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
      const boost::char_separator<char> del(", \t\n");

      tokenizer tokens(str.begin() + pos, str.end() - 1, del);
      int comp = 0;
      for( tokenizer::const_iterator tok = tokens.begin();
           tok != tokens.end() && comp < 4;
           ++tok, ++comp )
      {
        color._v[comp] = boost::lexical_cast<float>(*tok)
                       // rgb = [0,255], a = [0,1]
                       / (comp < 3 ? 255 : 1);
      }
    }
    else
      SG_LOG(SG_GENERAL, SG_WARN, "Unknown color: " << str);

    return color;
  }

  //----------------------------------------------------------------------------
  void linkColorNodes( const char* name,
                       SGPropertyNode* parent,
                       std::vector<SGPropertyNode_ptr>& nodes,
                       const osg::Vec4& def )
  {
    static const char* channels[] = {"red", "green", "blue", "alpha"};
    static const size_t num_channels = sizeof(channels)/sizeof(channels[0]);

    assert(name);
    assert(parent);

    // Don't tie to allow the usage of aliases
    SGPropertyNode_ptr color = parent->getChild(name, 0, true);

    // We need to be carefull do not get any unitialized nodes or null pointers
    // because while creating the node a valueChanged event will be triggered.
    nodes.clear();
    nodes.reserve(num_channels);

    for( size_t i = 0; i < num_channels; ++i )
      nodes.push_back( getChildDefault(color, channels[i], def[i]) );
  }

  //----------------------------------------------------------------------------
  void triggerChangeRecursive(SGPropertyNode* node)
  {
    node->getParent()->fireChildAdded(node);

    if( node->nChildren() == 0 && node->getType() != simgear::props::NONE )
      return node->fireValueChanged();

    for( int i = 0; i < node->nChildren(); ++i )
      triggerChangeRecursive( node->getChild(i) );
  }
}
