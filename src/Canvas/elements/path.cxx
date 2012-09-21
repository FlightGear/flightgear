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

  class Path;
  class PathDrawable:
    public osg::Drawable
  {
    public:
      PathDrawable(Path* path):
        _path_element(path),
        _path(VG_INVALID_HANDLE),
        _paint(VG_INVALID_HANDLE),
        _paint_fill(VG_INVALID_HANDLE),
        _attributes_dirty(~0),
        _mode(0),
        _stroke_width(1),
        _stroke_linecap(VG_CAP_BUTT)
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
      virtual osg::Object* cloneType() const { return new PathDrawable(_path_element); }
      virtual osg::Object* clone(const osg::CopyOp&) const { return new PathDrawable(_path_element); }

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
       * Set path fill paint ("none" if not filled)
       */
      void setFill(const std::string& fill)
      {
        if( fill == "none" )
        {
          _mode &= ~VG_FILL_PATH;
        }
        else
        {
          _fill_color = parseColor(fill);
          _mode |= VG_FILL_PATH;
          _attributes_dirty |= FILL_COLOR;
        }
      }

      /**
       * Set path stroke paint ("none" if no stroke)
       */
      void setStroke(const std::string& stroke)
      {
        if( stroke == "none" )
        {
          _mode &= ~VG_STROKE_PATH;
        }
        else
        {
          _stroke_color = parseColor(stroke);
          _mode |= VG_STROKE_PATH;
          _attributes_dirty |= STROKE_COLOR;
        }
      }

      /**
       * Set stroke width
       */
      void setStrokeWidth(float width)
      {
        _stroke_width = width;
        _attributes_dirty |= BOUNDING_BOX;
      }

      /**
       * Set stroke dash (line stipple)
       */
      void setStrokeDashArray(const std::string& dash)
      {
        _stroke_dash = splitAndConvert(",\t\n ", dash);
      }

      /**
       * Set stroke-linecap
       *
       * @see http://www.w3.org/TR/SVG/painting.html#StrokeLinecapProperty
       */
      void setStrokeLinecap(const std::string& linecap)
      {
        if( linecap == "round" )
          _stroke_linecap = VG_CAP_ROUND;
        else if( linecap == "square" )
          _stroke_linecap = VG_CAP_SQUARE;
        else
          _stroke_linecap = VG_CAP_BUTT;
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

          vgSetParameterfv(_paint, VG_PAINT_COLOR, 4, _stroke_color._v);

          _attributes_dirty &= ~STROKE_COLOR;
        }

        // Initialize/update fill paint
        if( _attributes_dirty & FILL_COLOR )
        {
          if( _paint_fill == VG_INVALID_HANDLE )
            _paint_fill = vgCreatePaint();

          vgSetParameterfv(_paint_fill, VG_PAINT_COLOR, 4, _fill_color._v);

          _attributes_dirty &= ~FILL_COLOR;
        }

        // Setup paint
        if( _mode & VG_STROKE_PATH )
        {
          vgSetPaint(_paint, VG_STROKE_PATH);

          vgSetf(VG_STROKE_LINE_WIDTH, _stroke_width);
          vgSeti(VG_STROKE_CAP_STYLE, _stroke_linecap);
          vgSetfv( VG_STROKE_DASH_PATTERN,
                   _stroke_dash.size(),
                   _stroke_dash.empty() ? 0 : &_stroke_dash[0] );
        }
        if( _mode & VG_FILL_PATH )
        {
          vgSetPaint(_paint_fill, VG_FILL_PATH);

          vgSetf(VG_FILL_RULE, VG_NON_ZERO); // TODO make user configurable
        }

        // And finally draw the path
        if( _mode )
          vgDrawPath(_path, _mode);

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

        osg::BoundingBox bb
        (
          min[0] - ext,           min[1] - ext,           -0.1,
          min[0] + size[0] + ext, min[1] + size[1] + ext,  0.1
        );
        _path_element->setBoundingBox(bb);

        return bb;
      }

    private:

      static bool _vg_initialized;

      enum Attributes
      {
        PATH            = 0x0001,
        STROKE_COLOR    = PATH << 1,
        FILL_COLOR      = STROKE_COLOR << 1,
        BOUNDING_BOX    = FILL_COLOR << 1
      };

      Path *_path_element;

      mutable VGPath    _path;
      mutable VGPaint   _paint;
      mutable VGPaint   _paint_fill;
      mutable uint32_t  _attributes_dirty;

      CmdList   _cmds;
      CoordList _coords;

      VGbitfield            _mode;
      osg::Vec4f            _fill_color;
      osg::Vec4f            _stroke_color;
      VGfloat               _stroke_width;
      std::vector<VGfloat>  _stroke_dash;
      VGCapStyle            _stroke_linecap;

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
  Path::Path(SGPropertyNode_ptr node, const Style& parent_style):
    Element(node, parent_style),
    _path( new PathDrawable(this) )
  {
    setDrawable(_path);
    PathDrawable *path = _path.get();

    addStyle("fill", &PathDrawable::setFill, path);
    addStyle("stroke", &PathDrawable::setStroke, path);
    addStyle("stroke-width", &PathDrawable::setStrokeWidth, path);
    addStyle("stroke-dasharray", &PathDrawable::setStrokeDashArray, path);
    addStyle("stroke-linecap", &PathDrawable::setStrokeLinecap, path);

    setupStyle();
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

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  void Path::childChanged(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    if( child->getNameString() == "cmd" )
      _attributes_dirty |= CMDS;
    else if( child->getNameString() == "coord" )
      _attributes_dirty |= COORDS;
  }

} // namespace canvas
