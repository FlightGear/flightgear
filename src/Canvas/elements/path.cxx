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
        _paint_fill(VG_INVALID_HANDLE),
        _attributes_dirty(~0),
        _stroke_width(1),
        _fill(false)
      {
        setSupportsDisplayList(false);
        setDataVariance(Object::DYNAMIC);

        setUpdateCallback(new PathUpdateCallback());
        setCullCallback(new NoCullCallback());
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

        _attributes_dirty |= BOUNDING_BOX;
      }

      /**
       * Set the line color
       */
      void setColor(const osg::Vec4& color)
      {
        for( size_t i = 0; i < 4; ++i )
          _stroke_color[i] = color[i];
        _attributes_dirty |= STROKE_COLOR;
      }

      /**
       * Enable/Disable filling of the path
       */
      void enableFill(bool enable)
      {
        _fill = enable;
      }

      /**
       * Set the line color
       */
      void setColorFill(const osg::Vec4& color)
      {
        for( size_t i = 0; i < 4; ++i )
          _fill_color[i] = color[i];
        _attributes_dirty |= FILL_COLOR;
      }

      /**
       * Draw callback
       */
      virtual void drawImplementation(osg::RenderInfo& renderInfo) const
      {
        if( (_attributes_dirty & PATH) && _vg_initialized )
          return;

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
          return;
        }

        // Initialize/Update the paint
        if( _attributes_dirty & STROKE_COLOR )
        {
          if( _paint == VG_INVALID_HANDLE )
            _paint = vgCreatePaint();

          vgSetParameterfv(_paint, VG_PAINT_COLOR, 4, _stroke_color);

          _attributes_dirty &= ~STROKE_COLOR;
        }

        // Initialize/update fill paint
        if( _attributes_dirty & (FILL_COLOR | FILL) )
        {
          if( _paint_fill == VG_INVALID_HANDLE )
            _paint_fill = vgCreatePaint();

          if( _attributes_dirty & FILL_COLOR )
            vgSetParameterfv(_paint_fill, VG_PAINT_COLOR, 4, _fill_color);

          _attributes_dirty &= ~(FILL_COLOR | FILL);
        }

        // Detect draw mode
        VGbitfield mode = 0;
        if( _stroke_width > 0 )
        {
          mode |= VG_STROKE_PATH;
          vgSetPaint(_paint, VG_STROKE_PATH);

          vgSetf(VG_STROKE_LINE_WIDTH, _stroke_width);
          vgSetfv( VG_STROKE_DASH_PATTERN,
                   _stroke_dash.size(),
                   _stroke_dash.empty() ? 0 : &_stroke_dash[0] );
        }
        if( _fill )
        {
          mode |= VG_FILL_PATH;
          vgSetPaint(_paint_fill, VG_FILL_PATH);
        }

        // And finally draw the path
        if( mode )
          vgDrawPath(_path, mode);

        VGErrorCode err = vgGetError();
        if( err != VG_NO_ERROR )
          SG_LOG(SG_GL, SG_ALERT, "vgError: " << err);

        glPopAttrib();
        glPopClientAttrib();
      }

      /**
       * Compute the bounding box
       */
      virtual osg::BoundingBox computeBound() const
      {
        if( _path == VG_INVALID_HANDLE )
          return osg::BoundingBox();

        VGfloat min[2], size[2];
        vgPathBounds(_path, &min[0], &min[1], &size[0], &size[1]);

        _attributes_dirty &= ~BOUNDING_BOX;

        // vgPathBounds doesn't take stroke width into account
        float ext = 0.5 * _stroke_width;

        return osg::BoundingBox
        (
          min[0] - ext,           min[1] - ext,           -0.1,
          min[0] + size[0] + ext, min[1] + size[1] + ext,  0.1
        );
      }

    private:

      static bool _vg_initialized;

      enum Attributes
      {
        PATH            = 0x0001,
        STROKE_COLOR    = PATH << 1,
        FILL_COLOR      = STROKE_COLOR << 1,
        FILL            = FILL_COLOR << 1,
        BOUNDING_BOX    = FILL << 1
      };

      mutable VGPath    _path;
      mutable VGPaint   _paint;
      mutable VGPaint   _paint_fill;
      mutable uint32_t  _attributes_dirty;

      CmdList   _cmds;
      CoordList _coords;

      VGfloat               _stroke_color[4];
      VGfloat               _stroke_width;
      std::vector<VGfloat>  _stroke_dash;

      bool      _fill;
      VGfloat   _fill_color[4];

      /**
       * Initialize/Update the OpenVG path
       */
      void update()
      {
        if( _attributes_dirty & PATH )
        {
          const VGbitfield caps = VG_PATH_CAPABILITY_APPEND_TO
                                | VG_PATH_CAPABILITY_MODIFY
                                | VG_PATH_CAPABILITY_PATH_BOUNDS;

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
          _attributes_dirty |= BOUNDING_BOX;
        }

        if( _attributes_dirty & BOUNDING_BOX )
          dirtyBound();
      }

      /**
       * Updating the path before drawing is needed to enable correct bounding
       * box calculations and make culling work.
       */
      struct PathUpdateCallback:
        public osg::Drawable::UpdateCallback
      {
        virtual void update(osg::NodeVisitor*, osg::Drawable* drawable)
        {
          if( !_vg_initialized )
            return;
          static_cast<PathDrawable*>(drawable)->update();
        }
      };

      /**
       * Callback used to prevent culling as long as OpenVG is not initialized.
       * This is needed because OpenVG needs an active OpenGL context for
       * initialization which is only available in #drawImplementation.
       * As soon as OpenVG is correctly initialized the callback automatically
       * removes itself from the node, so that the normal culling can get
       * active.
       */
      struct NoCullCallback:
        public osg::Drawable::CullCallback
      {
        virtual bool cull( osg::NodeVisitor*,
                           osg::Drawable* drawable,
                           osg::State* ) const
        {
          if( _vg_initialized )
            drawable->setCullCallback(0);
          return false;
        }
      };
  };

  bool PathDrawable::_vg_initialized = false;

  //----------------------------------------------------------------------------
  Path::Path(SGPropertyNode_ptr node):
    Element(node, COLOR | COLOR_FILL | BOUNDING_BOX),
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
    else if( child->getNameString() == "fill" )
      _path->enableFill( child->getBoolValue() );
  }

  //----------------------------------------------------------------------------
  void Path::colorChanged(const osg::Vec4& color)
  {
    _path->setColor(color);
  }

  //----------------------------------------------------------------------------
  void Path::colorFillChanged(const osg::Vec4& color)
  {
    _path->setColorFill(color);
  }

} // namespace canvas
