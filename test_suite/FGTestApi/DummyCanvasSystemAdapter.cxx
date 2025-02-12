
#include "DummyCanvasSystemAdapter.hxx"

#include <Main/globals.hxx>

#include <stdexcept>


namespace canvas
{
  //----------------------------------------------------------------------------
  simgear::canvas::FontPtr
  DummyCanvasSystemAdapter::getFont(const std::string& name) const
  {
    return simgear::canvas::FontPtr();
  }

  //----------------------------------------------------------------------------
  void DummyCanvasSystemAdapter::addCamera(osg::Camera* camera) const
  {
    SG_UNUSED(camera);
  }

  //----------------------------------------------------------------------------
  void DummyCanvasSystemAdapter::removeCamera(osg::Camera* camera) const
  {
    SG_UNUSED(camera);
  }

  //----------------------------------------------------------------------------
  osg::ref_ptr<osg::Image> DummyCanvasSystemAdapter::getImage(const std::string& path) const
  {
    return {};
  }

  //----------------------------------------------------------------------------
  SGSubsystem*
  DummyCanvasSystemAdapter::getSubsystem(const std::string& name) const
  {
    return globals->get_subsystem_mgr()->get_subsystem(name);
  }

  //----------------------------------------------------------------------------
  simgear::HTTP::Client* DummyCanvasSystemAdapter::getHTTPClient() const
  {
    return nullptr;
  }

}
