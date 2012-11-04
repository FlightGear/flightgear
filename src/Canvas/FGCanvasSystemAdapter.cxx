#include "FGCanvasSystemAdapter.hxx"

#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

#include <osgDB/ReadFile>

namespace canvas
{
  //----------------------------------------------------------------------------
  simgear::canvas::FontPtr
  FGCanvasSystemAdapter::getFont(const std::string& name) const
  {
    SGPath path = globals->resolve_resource_path("Fonts/" + name);
    if( path.isNull() )
    {
      SG_LOG
      (
        SG_GL,
        SG_ALERT,
        "canvas::Text: No such font: " << name
      );
      return simgear::canvas::FontPtr();
    }

    SG_LOG
    (
      SG_GL,
      SG_INFO,
      "canvas::Text: using font file " << path.str()
    );

    simgear::canvas::FontPtr font = osgText::readFontFile(path.c_str());
    if( !font )
      SG_LOG
      (
        SG_GL,
        SG_ALERT,
        "canvas::Text: Failed to open font file " << path.c_str()
      );

    return font;
  }

  //----------------------------------------------------------------------------
  void FGCanvasSystemAdapter::addCamera(osg::Camera* camera) const
  {
    globals->get_renderer()->addCamera(camera, false);
  }

  //----------------------------------------------------------------------------
  void FGCanvasSystemAdapter::removeCamera(osg::Camera* camera) const
  {
    globals->get_renderer()->removeCamera(camera);
  }

  //----------------------------------------------------------------------------
  osg::Image* FGCanvasSystemAdapter::getImage(const std::string& path) const
  {
    SGPath tpath = globals->resolve_resource_path(path);
    if( tpath.isNull() || !tpath.exists() )
    {
      SG_LOG(SG_GL, SG_ALERT, "canvas::Image: No such image: " << path);
      return 0;
    }

    return osgDB::readImageFile(tpath.c_str());
  }

}
