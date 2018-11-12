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

#pragma once

#include <osgViewer/GraphicsWindow>
#include <osg/GraphicsContext>

class QWindow;
class QOpenGLContext;
class QThread;

namespace flightgear
{

QOpenGLContext* qtContextFromOSG(osg::GraphicsContext* context);
QWindow* qtWindowFromOSG(osgViewer::GraphicsWindow* graphicsWindow);

/**
 * Helper run on Graphics thread to retrive its Qt wrapper
 */
class RetriveGraphicsThreadOperation : public osg::GraphicsOperation
{
public:
    RetriveGraphicsThreadOperation()
        : osg::GraphicsOperation("RetriveGraphicsThread", false)
    {
    }

    QThread* thread() const { return _result; }
    QOpenGLContext* context() const { return _context; }

    void operator()(osg::GraphicsContext* context) override;
private:
    QThread* _result = nullptr;
    QOpenGLContext* _context = nullptr;
};


}