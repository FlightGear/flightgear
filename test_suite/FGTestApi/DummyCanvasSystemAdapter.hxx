#pragma once

#include <simgear/canvas/CanvasSystemAdapter.hxx>

namespace canvas
{
  class DummyCanvasSystemAdapter:
    public simgear::canvas::SystemAdapter
  {
    public:
      simgear::canvas::FontPtr getFont(const std::string& name) const override;
      void addCamera(osg::Camera* camera) const override;
      void removeCamera(osg::Camera* camera) const override;
      osg::ref_ptr<osg::Image> getImage(const std::string& path) const override;
      SGSubsystem* getSubsystem(const std::string& name) const override;
      simgear::HTTP::Client* getHTTPClient() const override;
  };
}