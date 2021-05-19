// Copyright (C) 2020  James Turner

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

#include "config.h"

#include <QOpenGLContext>

#include "OSGQtAdaption.hxx"

#include <memory>

#include <QWindow>
#include <QThread>

#include <simgear/compiler.h>

#if defined(SG_MAC)
    // defined in OSGCocoaAdaption.mm
#elif defined(SG_WINDOWS)
#  include <QtPlatformHeaders/QWGLNativeContext>
#  include <osgViewer/api/Win32/GraphicsWindowWin32>
#else
// assume X11 for now
#  include <osgViewer/api/X11/GraphicsWindowX11>
#  include <QtPlatformHeaders/QGLXNativeContext>
#endif

namespace flightgear
{

#if defined(SG_MAC)

// defined in CocoaUtils.mm

#elif defined(SG_WINDOWS)


QOpenGLContext* qtContextFromOSG(osg::GraphicsContext* context)
{
    auto win = dynamic_cast<osgViewer::GraphicsWindowWin32*>(context);
    if (!win) {
        return nullptr;
    }

    auto wglnc = QWGLNativeContext{win->getWGLContext(), win->getHWND()};
    QVariant nativeHandle = QVariant::fromValue(wglnc);

    if (nativeHandle.isNull())
        return nullptr;

    std::unique_ptr<QOpenGLContext> qtOpenGLContext(new QOpenGLContext);
    qtOpenGLContext->setNativeHandle(nativeHandle);
    if (!qtOpenGLContext->create()) {
        return nullptr;
    }

    return qtOpenGLContext.release();
}

QWindow* qtWindowFromOSG(osgViewer::GraphicsWindow* graphicsWindow)
{
    auto win = dynamic_cast<osgViewer::GraphicsWindowWin32*>(graphicsWindow);
    if (!win) {
        return nullptr;
    }

    return QWindow::fromWinId(reinterpret_cast<qlonglong>(win->getHWND()));
}

#else

// assume X11 for now

QOpenGLContext* qtContextFromOSG(osg::GraphicsContext* context)
{
    auto win = dynamic_cast<osgViewer::GraphicsWindowX11*>(context);
    if (!win) {
        return nullptr;
    }

    auto glxnc = QGLXNativeContext{win->getContext(), win->getDisplay(), win->getWindow(), 0};
    QVariant nativeHandle = QVariant::fromValue(glxnc);

    if (nativeHandle.isNull())
        return nullptr;

    std::unique_ptr<QOpenGLContext> qtOpenGLContext(new QOpenGLContext);
    qtOpenGLContext->setNativeHandle(nativeHandle);
    if (!qtOpenGLContext->create()) {
        return nullptr;
    }

    return qtOpenGLContext.release();
}

QWindow* qtWindowFromOSG(osgViewer::GraphicsWindow* graphicsWindow)
{
    auto win = dynamic_cast<osgViewer::GraphicsWindowX11*>(graphicsWindow);
    if (!win) {
        return nullptr;
    }

    return QWindow::fromWinId(static_cast<qlonglong>(win->getWindow()));
}

#endif

class GraphicsFunctorWrapper : public osg::GraphicsOperation
{
public:
     GraphicsFunctorWrapper(const std::string& name, GraphicsFunctor gf) :
        osg::GraphicsOperation(name, false),
        _functor(gf)
    {
        
    }
    
    void operator()(osg::GraphicsContext* gc) override
    {
        _functor(gc);
    }
    
private:
    GraphicsFunctor _functor;
};

osg::ref_ptr<osg::GraphicsOperation> makeGraphicsOp(const std::string& name, GraphicsFunctor func)
{
    osg::ref_ptr<osg::GraphicsOperation> r;
    r = new GraphicsFunctorWrapper(name, func);
    return r;
}

} // of namespace
