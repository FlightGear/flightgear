// An OpenVG path on the canvas
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

#include "path.hxx"
#include <Canvas/property_helper.hxx>

#include <vg/openvg.h>

#include <osg/Drawable>
#include <osg/BlendFunc>

#include <cassert>

namespace canvas
{
  typedef std::vector<VGubyte>  CmdList;
  typedef std::vector<VGfloat>  CoordList;

  class PathDrawable:
    public osg::Drawable
  {
    public:
      PathDrawable():
        _path(VG_INVALID_HANDLE),
        _paint(VG_INVALID_HANDLE),
      _attributes_dirty(~0),
      _stroke_width(1)
      {
        setSupportsDisplayList(false);
        setDataVariance(Object::DYNAMIC);

        _paint_color[0] = 0;
        _paint_color[1] = 1;
        _paint_color[2] = 1;
        _paint_color[3] = 1;
      }

      virtual ~PathDrawable()
      {
        vgDestroyPath(_path);
        vgDestroyPaint(_paint);
      }

      virtual const char* className() const { return "PathDrawable"; }
      virtual osg::Object* cloneType() const { return new PathDrawable; }
      virtual osg::Object* clone(const osg::CopyOp&) const { return new PathDrawable; }

      /**
       * Replace the current path segments with the new ones
       *
       * @param cmds    List of OpenVG path commands
       * @param coords  List of coordinates/parameters used by #cmds
       */
      void setSegments(const CmdList& cmds, const CoordList& coords)
      {
        _cmds = cmds;
        _coords = coords;

        _attributes_dirty |= PATH;
      }

      /**
       * Set stroke width and dash (line stipple)
       */
      void setStroke( float width,
                      const std::vector<float> dash = std::vector<float>() )
      {
        _stroke_width = width;
        _stroke_dash = dash;

        _attributes_dirty |= STROKE;
      }

      /**
       * Set the line color
       */
      void setColor(const osg::Vec4& color)
      {
        for( size_t i = 0; i < 4; ++i )
          _paint_color[i] = color[i];
        _attributes_dirty |= PAINT_COLOR;
      }

      /**
       * Draw callback
       */
      virtual void drawImplementation(osg::RenderInfo& renderInfo) const
      {
        osg::State* state = renderInfo.getState();
        assert(state);

        state->setActiveTextureUnit(0);
        state->setClientActiveTextureUnit(0);
        state->disableAllVertexArrays();

        glPushAttrib(~0u); // Don't use GL_ALL_ATTRIB_BITS as on my machine it
                           // eg. doesn't include GL_MULTISAMPLE_BIT
        glPushClientAttrib(~0u);

        // Initialize OpenVG itself
        if( !_vg_initialized )
        {
          GLint vp[4];
          glGetIntegerv(GL_VIEWPORT, vp);

          vgCreateContextSH(vp[2], vp[3]);
          _vg_initialized = true;
        }

        // Initialize/Update the path
        if( _attributes_dirty & PATH )
        {
          const VGbitfield caps = VG_PATH_CAPABILITY_APPEND_TO
                                | VG_PATH_CAPABILITY_MODIFY;

          if( _path == VG_INVALID_HANDLE )
            _path = vgCreatePath(
              VG_PATH_FORMAT_STANDARD,
              VG_PATH_DATATYPE_F,
              1.f, 0.f, // scale,bias
              _cmds.size(), _coords.size(),
              caps
            );
          else
            vgClearPath(_path, caps);

          if( !_cmds.empty() && !_coords.empty() )
            vgAppendPathData(_path, _cmds.size(), &_cmds[0], &_coords[0]);

          _attributes_dirty &= ~PATH;
        }

        // Initialize/Update the paint
        if( _attributes_dirty & (PAINT_COLOR | STROKE) )
        {
          if( _paint == VG_INVALID_HANDLE )
          {
            _paint = vgCreatePaint();
            vgSetPaint(_paint, VG_STROKE_PATH);
          }

          if( _attributes_dirty & PAINT_COLOR )
          {
            vgSetParameterfv(_paint, VG_PAINT_COLOR, 4, _paint_color);
          }
          if( _attributes_dirty & STROKE )
          {
            vgSetf(VG_STROKE_LINE_WIDTH, _stroke_width);

            vgSetfv( VG_STROKE_DASH_PATTERN,
                     _stroke_dash.size(),
                     _stroke_dash.empty() ? 0 : &_stroke_dash[0] );
          }

          _attributes_dirty &= ~(PAINT_COLOR | STROKE);
        }

        // And finally draw the path
        vgDrawPath(_path, VG_STROKE_PATH);

        VGErrorCode err = vgGetError();
        if( err != VG_NO_ERROR )
          std::cout << "vgError: " << err << std::endl;

        glPopAttrib();
        glPopClientAttrib();
      }

    private:

      static bool _vg_initialized;

      enum Attributes
      {
        PATH            = 0x0001,
        PAINT_COLOR     = 0x0002,
        STROKE          = 0x0004
      };

      mutable VGPath    _path;
      mutable VGPaint   _paint;
      mutable uint32_t  _attributes_dirty;

      CmdList   _cmds;
      CoordList _coords;

      VGfloat               _paint_color[4];
      VGfloat               _stroke_width;
      std::vector<VGfloat>  _stroke_dash;
  };

  bool PathDrawable::_vg_initialized = false;

  //----------------------------------------------------------------------------
  Path::Path(SGPropertyNode_ptr node):
    Element(node, COLOR /*| COLOR_FILL*/), // TODO fill color
    _path( new PathDrawable() )
  {
    setDrawable(_path);
  }

  //----------------------------------------------------------------------------
  Path::~Path()
  {

  }

  //----------------------------------------------------------------------------
  void Path::update(double dt)
  {
    if( _attributes_dirty & (CMDS | COORDS) )
    {
      _path->setSegments
      (
        getVectorFromChildren<VGubyte, int>(_node, "cmd"),
        getVectorFromChildren<VGfloat, float>(_node, "coord")
      );

      _attributes_dirty &= ~(CMDS | COORDS);
    }
    if( _attributes_dirty & STROKE )
    {
      _path->setStroke
      (
        _node->getFloatValue("stroke-width", 1),
        getVectorFromChildren<VGfloat, float>(_node, "stroke-dasharray")
      );

      _attributes_dirty &= ~STROKE;
    }

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  void Path::childChanged(SGPropertyNode* child)
  {
    if( child->getNameString() == "cmd" )
      _attributes_dirty |= CMDS;
    else if( child->getNameString() == "coord" )
      _attributes_dirty |= COORDS;
    else if(    child->getNameString() == "stroke-width"
             || child->getNameString() == "stroke-dasharray" )
      _attributes_dirty |= STROKE;
  }

  //----------------------------------------------------------------------------
  void Path::colorChanged(const osg::Vec4& color)
  {
    _path->setColor(color);
  }

  //----------------------------------------------------------------------------
  void Path::colorFillChanged(const osg::Vec4& color)
  {

  }

} // namespace canvas
