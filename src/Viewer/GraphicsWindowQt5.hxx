// Copyright (C) 2017 James Turner
// derived from OSG GraphicsWindowQt by Wang Rui
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

#ifndef FGFS_GRAPHICSWINDOWQT
#define FGFS_GRAPHICSWINDOWQT

#include <QWindow>

#include <osgViewer/GraphicsWindow>

#include <QMutex>
#include <QEvent>
#include <QQueue>
#include <QSet>

class QInputEvent;
class QGestureEvent;
class QOpenGLContext;

namespace osgViewer {
    class ViewerBase;
}

namespace flightgear
{

// forward declarations
class GraphicsWindowQt5;

/// The function sets the WindowingSystem to Qt.
void initQtWindowingSystem();

class GLWindow : public QWindow
{
    Q_OBJECT
    typedef QWindow inherited;

public:
    GLWindow();

    virtual ~GLWindow();

    virtual void setGraphicsWindow( GraphicsWindowQt5* gw ) { _gw = gw; }
    virtual GraphicsWindowQt5* getGraphicsWindow() { return _gw; }
    virtual const GraphicsWindowQt5* getGraphicsWindow() const { return _gw; }

    inline bool getForwardKeyEvents() const { return _forwardKeyEvents; }
    virtual void setForwardKeyEvents( bool f ) { _forwardKeyEvents = f; }

    void setKeyboardModifiers( QInputEvent* event );

    virtual void keyPressEvent( QKeyEvent* event );
    virtual void keyReleaseEvent( QKeyEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseReleaseEvent( QMouseEvent* event );
    virtual void mouseDoubleClickEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void wheelEvent( QWheelEvent* event );

#if QT_VERSION < 0x050500
    void requestUpdate();
#endif
signals:
    void beforeRendering();
    void afterRendering();

private slots:
    void onScreenChanged();
    
protected:
    void syncGeometryWithOSG();
    
  
    friend class GraphicsWindowQt5;
    GraphicsWindowQt5* _gw = nullptr; // back-pointer

    bool _forwardKeyEvents = false;
    qreal _devicePixelRatio = 1.0;
    
    // is this the primary (GUI) window
    bool _isPrimaryWindow = false;

    virtual void resizeEvent( QResizeEvent* event );
    virtual void moveEvent( QMoveEvent* event );
    virtual bool event( QEvent* event );
};

class GraphicsWindowQt5 : public osgViewer::GraphicsWindow
{
public:
    GraphicsWindowQt5( osg::GraphicsContext::Traits* traits = 0);
    virtual ~GraphicsWindowQt5();

    inline GLWindow* getGLWindow() { return _window.get(); }
    inline const GLWindow* getGLWindow() const { return _window.get(); }

    struct WindowData : public osg::Referenced
    {
        WindowData( GLWindow* win = NULL ): _window(win) {}
        GLWindow* _window;
        
        bool createFullscreen = false;
        
        // is this the main window, corresponding to the /sim/startup
        // properties? If so we will drive them in some cases.
        bool isPrimaryWindow = false;
    };

    bool init( Qt::WindowFlags f );

    static QSurfaceFormat traits2qSurfaceFormat( const osg::GraphicsContext::Traits* traits );
    static void qSurfaceFormat2traits( const QSurfaceFormat& format, osg::GraphicsContext::Traits* traits );
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
  //  inline bool getTouchEventsEnabled() const { return _widget->getTouchEventsEnabled(); }
  //  virtual void setTouchEventsEnabled( bool e ) { _widget->setTouchEventsEnabled(e); }


    virtual bool valid() const;
    virtual bool realizeImplementation();
    virtual bool isRealizedImplementation() const;
    virtual void closeImplementation();
    virtual bool makeCurrentImplementation();
    virtual bool releaseContextImplementation();
    virtual void swapBuffersImplementation();
    virtual void runOperations();
    virtual bool checkEvents();

    virtual void requestRedraw();
    virtual void requestContinuousUpdate(bool needed=true);

    virtual void requestWarpPointer( float x, float y );

    /**
      * set the viewer for this window. This will be refreshed when then
      * QGuiApplication event loop runs (via exec), or when the window
      * needs to be updated.
      */
    void setViewer( osgViewer::ViewerBase *viewer );

    virtual void contextInitalised() { ; }
    
    void setFullscreen(bool isFullscreen);
protected:
    virtual void viewerChanged(osgViewer::ViewerBase*);

    friend class GLWindow;
    std::unique_ptr<GLWindow> _window;
    std::unique_ptr<QOpenGLContext> _context;
    QOpenGLContext* _shareContext = nullptr;
    bool _ownsWidget;
    QCursor _currentCursor;
    bool _realized = false;
    bool _updateContextNeeded = false;
    bool _continousUpdate = false;
    osg::observer_ptr< osgViewer::ViewerBase > _viewer;
    
    // if true, we will generate a resize event on the next
    // call to check events. Ths works around OSG clearing the
    // event queue on us.
    bool _sendResizeOnEventCheck = false;
};

} // of namespace flightgear

#endif
