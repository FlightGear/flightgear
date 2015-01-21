/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGVIEWER_GRAPHICSWINDOWQT5
#define OSGVIEWER_GRAPHICSWINDOWQT5

#include <QtGui/QSurfaceFormat>
#include <QtGui/QWindow>

#include <osgViewer/GraphicsWindow>

class QInputEvent;
class QOpenGLContext;

namespace osgViewer {
    class ViewerBase;
}

namespace osgQt5
{

// forward declarations
class GraphicsWindowQt;

/// The function sets the WindowingSystem to Qt.
//void OSGQT_EXPORT initQtWindowingSystem();

class OSGQtWindow : public QWindow
{
    typedef QWindow inherited;
public:

    OSGQtWindow( QWindow* parent = NULL, bool forwardKeyEvents = false );
    OSGQtWindow( QOpenGLContext* context, QWindow* parent = NULL, bool forwardKeyEvents = false );
    OSGQtWindow( const QSurfaceFormat& format, QWindow* parent = NULL, bool forwardKeyEvents = false );
    virtual ~OSGQtWindow();

    inline void setGraphicsWindow( GraphicsWindowQt* gw ) { _gw = gw; }
    inline GraphicsWindowQt* getGraphicsWindow() { return _gw; }
    inline const GraphicsWindowQt* getGraphicsWindow() const { return _gw; }

    inline bool getForwardKeyEvents() const { return _forwardKeyEvents; }
    virtual void setForwardKeyEvents( bool f ) { _forwardKeyEvents = f; }

    void setKeyboardModifiers( QInputEvent* event );

    bool underMouse() const;

protected:

    friend class GraphicsWindowQt;
    GraphicsWindowQt* _gw;

    bool _forwardKeyEvents;

    virtual void resizeEvent( QResizeEvent* event );
    virtual void moveEvent( QMoveEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );
    virtual void keyReleaseEvent( QKeyEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent* event );
};

class GraphicsWindowQt : public osgViewer::GraphicsWindow
{
public:
    GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWindow* parent = NULL, Qt::WindowFlags f = 0 );
   // GraphicsWindowQt( GLWidget* widget );
    virtual ~GraphicsWindowQt();

    OSGQtWindow* getQtWindow() { return _qtWindow; }
    const OSGQtWindow* getQtWindow() const { return _qtWindow; }

    QOpenGLContext* getQtContext() const { return _qtContext; }

    bool init( QWindow* parent, Qt::WindowFlags f );

    static QSurfaceFormat traits2QSurfaceFormat( const osg::GraphicsContext::Traits* traits );
    static void QSurfaceFormat2traits( const QSurfaceFormat& format, osg::GraphicsContext::Traits* traits );
    static osg::GraphicsContext::Traits* createTraits( const QWindow* window );

    virtual bool setWindowRectangleImplementation( int x, int y, int width, int height );
    virtual void getWindowRectangle( int& x, int& y, int& width, int& height );
    virtual bool setWindowDecorationImplementation( bool windowDecoration );
    virtual bool getWindowDecoration() const;
    virtual void grabFocus();
    virtual void grabFocusIfPointerInWindow();
    virtual void raiseWindow();
    virtual void setWindowName( const std::string& name );
    virtual std::string getWindowName();
    virtual void useCursor( bool cursorOn );
    virtual void setCursor( MouseCursor cursor );

    virtual bool valid() const;
    virtual bool realizeImplementation();
    virtual bool isRealizedImplementation() const;
    virtual void closeImplementation();
    virtual bool makeCurrentImplementation();
    virtual bool releaseContextImplementation();
    virtual void swapBuffersImplementation();
    virtual void runOperations();
    virtual void resizedImplementation(int x, int y, int width, int height);
    virtual void requestWarpPointer( float x, float y );

protected:
    friend class OSGQtWindow;

    OSGQtWindow* _qtWindow;
    QOpenGLContext* _qtContext;

    QCursor _currentCursor;
    bool _realized;
};

} // of namespace osgQt5

#endif
