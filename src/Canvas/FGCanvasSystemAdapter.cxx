// Integrate Canvas into FlightGear
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

#include "FGCanvasSystemAdapter.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Network/HTTPClient.hxx>
#include <Viewer/renderer.hxx>

#include <osg/Version>
#include <osgDB/ReadFile>
#include <stdexcept>

#include <simgear/scene/util/SGReaderWriterOptions.hxx>

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
      SG_DEBUG,
      "canvas::Text: using font file " << path
    );

    simgear::canvas::FontPtr font = osgText::readFontFile(path.utf8Str());
    if( !font )
      SG_LOG
      (
        SG_GL,
        SG_ALERT,
        "canvas::Text: Failed to open font file " << path
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
    if( globals->get_renderer() )
      globals->get_renderer()->removeCamera(camera);
  }

  //----------------------------------------------------------------------------
  osg::ref_ptr<osg::Image> FGCanvasSystemAdapter::getImage(const std::string& path) const
  {
    SGPath p(SGPath::fromUtf8(path));
    osg::ref_ptr<simgear::SGReaderWriterOptions> localReaderWriterOptions = simgear::SGReaderWriterOptions::copyOrCreate(osgDB::Registry::instance()->getOptions());
    localReaderWriterOptions->setLoadOriginHint(simgear::SGReaderWriterOptions::LoadOriginHint::ORIGIN_CANVAS);

    if( p.isAbsolute() )
    {
      SGPath valid_path = fgValidatePath(p, false);
      if( !valid_path.isNull() )
          return osgDB::readRefImageFile(valid_path.utf8Str(), localReaderWriterOptions);

      SG_LOG(SG_IO, SG_ALERT, "canvas::Image: reading '" << path << "' denied");
    }
    else
    {
      SGPath tpath = globals->resolve_resource_path(path);
      if( !tpath.isNull() )
          return osgDB::readRefImageFile(tpath.utf8Str(), localReaderWriterOptions);

      SG_LOG(SG_IO, SG_ALERT, "canvas::Image: No such image: '" << path << "'");
    }

    return 0;
  }

  //----------------------------------------------------------------------------
  SGSubsystem*
  FGCanvasSystemAdapter::getSubsystem(const std::string& name) const
  {
    return globals->get_subsystem(name.c_str());
  }

  //----------------------------------------------------------------------------
  simgear::HTTP::Client* FGCanvasSystemAdapter::getHTTPClient() const
  {
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();

    if( http )
      return http->client();

    SG_LOG( SG_IO,
            SG_ALERT,
            "FGCanvasSystemAdapter: Failed to get HTTP subsystem" );
    return nullptr;
  }

}
