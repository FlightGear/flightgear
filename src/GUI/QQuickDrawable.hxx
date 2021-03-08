// QQuickDrawable.hxx - OSG Drawable using a QQuickRenderControl to draw
//
// Copyright (C) 2019  James Turner  <james@flightgear.org>
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

#ifndef FG_VIEWER_QUICK_DRAWABLE_HXX
#define FG_VIEWER_QUICK_DRAWABLE_HXX

#include <memory>
#include <osg/Drawable>

#include <osgViewer/GraphicsWindow>

namespace osgViewer {
class Viewer;
}

class QUrl;
class QQuickDrawablePrivate;

class QQuickDrawable : public osg::Drawable
{
public:
    QQuickDrawable();
    virtual ~QQuickDrawable();

  virtual osg::Object* cloneType() const { return 0; }
  virtual osg::Object* clone(const osg::CopyOp& copyop) const { return 0; }

  void drawImplementation(osg::RenderInfo& renderInfo) const override;

  void setup(osgViewer::GraphicsWindow* gw, osgViewer::Viewer* viewer);

    void setSourcePath(const std::string& path);
  void setSource(const QUrl& url);

  void reload(const QUrl& url);

  void resize(int width, int height);

private:
  std::unique_ptr<QQuickDrawablePrivate> d;
};

#endif
