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

#include <osg/DeleteHandler>
#include <osgViewer/ViewerBase>

#include <QDebug>
#include <QThread>

#include <QtGui/QInputEvent>
#include <QtGui/QOpenGLContext>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include "GraphicsWindowQt5.hxx"

#include <osg/Camera>
#include <osg/Viewport>

#include <simgear/debug/logstream.hxx>

using namespace osgQt5;

class QtKeyboardMap
{

public:
    QtKeyboardMap()
    {
        mKeyMap[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
        mKeyMap[Qt::Key_Delete   ] = osgGA::GUIEventAdapter::KEY_Delete;
        mKeyMap[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
        mKeyMap[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
        mKeyMap[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
        mKeyMap[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
        mKeyMap[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
        mKeyMap[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
        mKeyMap[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
        mKeyMap[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
        mKeyMap[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
        mKeyMap[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
        mKeyMap[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
        mKeyMap[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
        mKeyMap[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
        mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
        mKeyMap[Qt::Key_Alt      ] = osgGA::GUIEventAdapter::KEY_Alt_L;
        mKeyMap[Qt::Key_Shift    ] = osgGA::GUIEventAdapter::KEY_Shift_L;
        mKeyMap[Qt::Key_Control  ] = osgGA::GUIEventAdapter::KEY_Control_L;
        mKeyMap[Qt::Key_Meta     ] = osgGA::GUIEventAdapter::KEY_Meta_L;

        mKeyMap[Qt::Key_F1             ] = osgGA::GUIEventAdapter::KEY_F1;
        mKeyMap[Qt::Key_F2             ] = osgGA::GUIEventAdapter::KEY_F2;
        mKeyMap[Qt::Key_F3             ] = osgGA::GUIEventAdapter::KEY_F3;
        mKeyMap[Qt::Key_F4             ] = osgGA::GUIEventAdapter::KEY_F4;
        mKeyMap[Qt::Key_F5             ] = osgGA::GUIEventAdapter::KEY_F5;
        mKeyMap[Qt::Key_F6             ] = osgGA::GUIEventAdapter::KEY_F6;
        mKeyMap[Qt::Key_F7             ] = osgGA::GUIEventAdapter::KEY_F7;
        mKeyMap[Qt::Key_F8             ] = osgGA::GUIEventAdapter::KEY_F8;
        mKeyMap[Qt::Key_F9             ] = osgGA::GUIEventAdapter::KEY_F9;
        mKeyMap[Qt::Key_F10            ] = osgGA::GUIEventAdapter::KEY_F10;
        mKeyMap[Qt::Key_F11            ] = osgGA::GUIEventAdapter::KEY_F11;
        mKeyMap[Qt::Key_F12            ] = osgGA::GUIEventAdapter::KEY_F12;
        mKeyMap[Qt::Key_F13            ] = osgGA::GUIEventAdapter::KEY_F13;
        mKeyMap[Qt::Key_F14            ] = osgGA::GUIEventAdapter::KEY_F14;
        mKeyMap[Qt::Key_F15            ] = osgGA::GUIEventAdapter::KEY_F15;
        mKeyMap[Qt::Key_F16            ] = osgGA::GUIEventAdapter::KEY_F16;
        mKeyMap[Qt::Key_F17            ] = osgGA::GUIEventAdapter::KEY_F17;
        mKeyMap[Qt::Key_F18            ] = osgGA::GUIEventAdapter::KEY_F18;
        mKeyMap[Qt::Key_F19            ] = osgGA::GUIEventAdapter::KEY_F19;
        mKeyMap[Qt::Key_F20            ] = osgGA::GUIEventAdapter::KEY_F20;

        mKeyMap[Qt::Key_hyphen         ] = '-';
        mKeyMap[Qt::Key_Equal         ] = '=';

        mKeyMap[Qt::Key_division      ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
        mKeyMap[Qt::Key_multiply      ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
        mKeyMap[Qt::Key_Minus         ] = '-';
        mKeyMap[Qt::Key_Plus          ] = '+';
        //mKeyMap[Qt::Key_H              ] = osgGA::GUIEventAdapter::KEY_KP_Home;
        //mKeyMap[Qt::Key_                    ] = osgGA::GUIEventAdapter::KEY_KP_Up;
        //mKeyMap[92                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
        //mKeyMap[86                    ] = osgGA::GUIEventAdapter::KEY_KP_Left;
        //mKeyMap[87                    ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
        //mKeyMap[88                    ] = osgGA::GUIEventAdapter::KEY_KP_Right;
        //mKeyMap[83                    ] = osgGA::GUIEventAdapter::KEY_KP_End;
        //mKeyMap[84                    ] = osgGA::GUIEventAdapter::KEY_KP_Down;
        //mKeyMap[85                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
        mKeyMap[Qt::Key_Insert        ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
        //mKeyMap[Qt::Key_Delete        ] = osgGA::GUIEventAdapter::KEY_KP_Delete;
    }

    ~QtKeyboardMap()
    {
    }

    int remapKey(QKeyEvent* event)
    {
        KeyMap::iterator itr = mKeyMap.find(event->key());
        if (itr == mKeyMap.end())
        {
            return int(*(event->text().toLocal8Bit().data()));
        }
        else
            return itr->second;
    }

    private:
    typedef std::map<unsigned int, int> KeyMap;
    KeyMap mKeyMap;
};

static QtKeyboardMap s_QtKeyboardMap;

OSGQtWindow::OSGQtWindow( QWindow* parent, bool forwardKeyEvents )
: QWindow(parent),
    _gw( NULL ),
    _forwardKeyEvents( forwardKeyEvents )
{
    setSurfaceType(QSurface::OpenGLSurface);
}

OSGQtWindow::OSGQtWindow( QOpenGLContext* context, QWindow* parent, bool forwardKeyEvents )
: QWindow(parent),
    _gw( NULL ),
    _forwardKeyEvents( forwardKeyEvents )
{
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(context->format());
}

OSGQtWindow::OSGQtWindow( const QSurfaceFormat& format, QWindow* parent, bool forwardKeyEvents )
: QWindow(parent),
_gw( NULL ),
_forwardKeyEvents( forwardKeyEvents )
{
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(format);
}

OSGQtWindow::~OSGQtWindow()
{
    // close GraphicsWindowQt and remove the reference to us
    if( _gw )
    {
        _gw->close();
        _gw->_qtWindow = NULL;
        _gw = NULL;
    }
}

void OSGQtWindow::setKeyboardModifiers( QInputEvent* event )
{
    int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int mask = 0;
    if ( modkey & Qt::ShiftModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if ( modkey & Qt::ControlModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if ( modkey & Qt::AltModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    _gw->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
}

bool OSGQtWindow::underMouse() const
{
    QPoint globalMouse = QCursor::pos();
    return frameGeometry().contains(globalMouse);
}

void OSGQtWindow::resizeEvent( QResizeEvent* event )
{
    const QSize& size = event->size();

    SG_LOG(SG_VIEW, SG_ALERT, "Qt resizeEvent:" << x() << "," << y() << " w:" << size.width()
           << " h:" << size.height());

    _gw->resized( x(), y(), size.width(), size.height() );
    _gw->getEventQueue()->windowResize( x(), y(),
                                       size.width(), size.height(),
                                       _gw->getEventQueue()->getTime());

    QWindow::resizeEvent(event);
}

void OSGQtWindow::moveEvent( QMoveEvent* event )
{
        const QPoint& pos = event->pos();
    SG_LOG(SG_VIEW, SG_ALERT, "Qt moveEvent:" << pos.x() << "," << pos.y() << " w:" << width()
           << " h:" << height());


    _gw->resized( pos.x(), pos.y(), width(), height() );
    _gw->getEventQueue()->windowResize( pos.x(), pos.y(),
                                       width(), height(),
                                        _gw->getEventQueue()->getTime());
}

void OSGQtWindow::keyPressEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    int value = s_QtKeyboardMap.remapKey( event );
    _gw->getEventQueue()->keyPress( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    if( _forwardKeyEvents )
        inherited::keyPressEvent( event );
}

void OSGQtWindow::keyReleaseEvent( QKeyEvent* event )
{
    setKeyboardModifiers( event );
    int value = s_QtKeyboardMap.remapKey( event );
    _gw->getEventQueue()->keyRelease( value );

    // this passes the event to the regular Qt key event processing,
    // among others, it closes popup windows on ESC and forwards the event to the parent widgets
    if( _forwardKeyEvents )
        inherited::keyReleaseEvent( event );
}

void OSGQtWindow::mousePressEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    _gw->getEventQueue()->mouseButtonPress( event->x(), event->y(), button );
}

void OSGQtWindow::mouseReleaseEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    _gw->getEventQueue()->mouseButtonRelease( event->x(), event->y(), button );
}

void OSGQtWindow::mouseDoubleClickEvent( QMouseEvent* event )
{
    int button = 0;
    switch ( event->button() )
    {
        case Qt::LeftButton: button = 1; break;
        case Qt::MidButton: button = 2; break;
        case Qt::RightButton: button = 3; break;
        case Qt::NoButton: button = 0; break;
        default: button = 0; break;
    }
    setKeyboardModifiers( event );
    _gw->getEventQueue()->mouseDoubleButtonPress( event->x(), event->y(), button );
}

void OSGQtWindow::mouseMoveEvent( QMouseEvent* event )
{
    setKeyboardModifiers( event );
    _gw->getEventQueue()->mouseMotion( event->x(), event->y() );
}

void OSGQtWindow::wheelEvent( QWheelEvent* event )
{
    setKeyboardModifiers( event );
    _gw->getEventQueue()->mouseScroll(
        event->orientation() == Qt::Vertical ?
            (event->delta()>0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN) :
            (event->delta()>0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT) );
}

GraphicsWindowQt::GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWindow* parent, Qt::WindowFlags f )
    : _qtWindow(NULL),
  _qtContext(NULL),
  _realized(false)
{
    qDebug() << "creating GraphicsWindowQt";
    _traits = traits;
    init( parent, f );
}

#if 0
GraphicsWindowQt::GraphicsWindowQt( GLWidget* widget )
:   _qtContext(NULL),
  _realized(false)
{
    _qtWindow = widget;
    _traits = _qtWindow ? createTraits( _qtWindow ) : new osg::GraphicsContext::Traits;
    init( NULL, NULL, 0 );
}
#endif
GraphicsWindowQt::~GraphicsWindowQt()
{
    close();

    // remove reference from GLWidget
    if ( _qtWindow )
        _qtWindow->_gw = NULL;
}

bool GraphicsWindowQt::init( QWindow* parent, Qt::WindowFlags f )
{
    // create window if it does not exist
    _qtWindow = NULL;
    if ( !_qtWindow )
    {
        // WindowFlags
        Qt::WindowFlags flags = f | Qt::Window | Qt::CustomizeWindowHint;
        if ( _traits->windowDecoration )
            flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint;

        // create QWindow
        _qtWindow = new OSGQtWindow( traits2QSurfaceFormat( _traits.get() ), parent );
        _qtWindow->setFlags(flags);
    }

    _qtWindow->setTitle( _traits->windowName.c_str() );
    _qtWindow->setPosition( _traits->x, _traits->y );
    _qtWindow->resize( _traits->width, _traits->height );

    // initialize widget properties
   // _qtWindow->setMouseTracking( true );
   // _qtWindow->setFocusPolicy( Qt::WheelFocus );
    _qtWindow->setGraphicsWindow( this );
    useCursor( _traits->useCursor );

    // initialize State
    setState( new osg::State );
    getState()->setGraphicsContext(this);

    // initialize contextID
    if ( _traits.valid() && _traits->sharedContext.valid() )
    {
        getState()->setContextID( _traits->sharedContext->getState()->getContextID() );
        incrementContextIDUsageCount( getState()->getContextID() );
    }
    else
    {
        getState()->setContextID( osg::GraphicsContext::createNewContextID() );
    }

    // make sure the event queue has the correct window rectangle size and input range
    //getEventQueue()->syncWindowRectangleWithGraphcisContext();
    return true;
}

QSurfaceFormat GraphicsWindowQt::traits2QSurfaceFormat( const osg::GraphicsContext::Traits* traits )
{
    QSurfaceFormat format;

    format.setAlphaBufferSize( traits->alpha );
    format.setRedBufferSize( traits->red );
    format.setGreenBufferSize( traits->green );
    format.setBlueBufferSize( traits->blue );
    format.setDepthBufferSize( traits->depth );
    format.setStencilBufferSize( traits->stencil );
   // format.setSampleBuffers( traits->sampleBuffers );
    format.setSamples( traits->samples );

    format.setSwapBehavior( traits->doubleBuffer ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
  //  format.setSwapInterval( traits->vsync ? 1 : 0 );
    format.setStereo( traits->quadBufferStereo ? 1 : 0 );

    return format;
}

void GraphicsWindowQt::QSurfaceFormat2traits( const QSurfaceFormat& format, osg::GraphicsContext::Traits* traits )
{
    traits->red = format.redBufferSize();
    traits->green = format.greenBufferSize();
    traits->blue = format.blueBufferSize();
    traits->alpha = format.alphaBufferSize();
    traits->depth = format.depthBufferSize();
    traits->stencil = format.stencilBufferSize();

    //traits->sampleBuffers = format.sampleBuffers() ? 1 : 0;
    traits->samples = format.samples();

    traits->quadBufferStereo = format.stereo();
    traits->doubleBuffer = format.swapBehavior() == QSurfaceFormat::DoubleBuffer;

    //traits->vsync = format.swapInterval() >= 1;
}

osg::GraphicsContext::Traits* GraphicsWindowQt::createTraits( const QWindow* window )
{
    osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits;

    QSurfaceFormat2traits( window->format(), traits );

    QRect r = window->geometry();
    traits->x = r.x();
    traits->y = r.y();
    traits->width = r.width();
    traits->height = r.height();

    traits->windowName = window->title().toLocal8Bit().data();
    Qt::WindowFlags f = window->flags();
    traits->windowDecoration = ( f & Qt::WindowTitleHint ) &&
                            ( f & Qt::WindowMinMaxButtonsHint ) &&
                            ( f & Qt::WindowSystemMenuHint );
  //  QSizePolicy sp = widget->sizePolicy();
   // traits->supportsResize = sp.horizontalPolicy() != QSizePolicy::Fixed ||
    //                        sp.verticalPolicy() != QSizePolicy::Fixed;

    return traits;
}

bool GraphicsWindowQt::setWindowRectangleImplementation( int x, int y, int width, int height )
{
    if ( _qtWindow == NULL )
        return false;

    _qtWindow->setGeometry( x, y, width, height );
    return true;
}

void GraphicsWindowQt::getWindowRectangle( int& x, int& y, int& width, int& height )
{
    if ( _qtWindow )
    {
        const QRect& geom = _qtWindow->geometry();
        x = geom.x();
        y = geom.y();
        width = geom.width();
        height = geom.height();
    }
}

bool GraphicsWindowQt::setWindowDecorationImplementation( bool windowDecoration )
{
    Qt::WindowFlags flags = Qt::Window|Qt::CustomizeWindowHint;//|Qt::WindowStaysOnTopHint;
    if ( windowDecoration )
        flags |= Qt::WindowTitleHint|Qt::WindowMinMaxButtonsHint|Qt::WindowSystemMenuHint;
    _traits->windowDecoration = windowDecoration;

    if ( _qtWindow )
    {
        _qtWindow->setFlags( flags );
        return true;
    }

    return false;
}

bool GraphicsWindowQt::getWindowDecoration() const
{
    return _traits->windowDecoration;
}

void GraphicsWindowQt::grabFocus()
{
    if ( _qtWindow )
        _qtWindow->requestActivate();
}

void GraphicsWindowQt::grabFocusIfPointerInWindow()
{
    if ( _qtWindow && _qtWindow->underMouse() )
        _qtWindow->requestActivate();
}

void GraphicsWindowQt::raiseWindow()
{
    if ( _qtWindow )
        _qtWindow->raise();
}

void GraphicsWindowQt::setWindowName( const std::string& name )
{
    if ( _qtWindow )
        _qtWindow->setTitle( QString::fromLocal8Bit(name.c_str()) );
}

std::string GraphicsWindowQt::getWindowName()
{
    return _qtWindow ? _qtWindow->title().toStdString() : "";
}

void GraphicsWindowQt::useCursor( bool cursorOn )
{
    if ( _qtWindow )
    {
        _traits->useCursor = cursorOn;
        if ( !cursorOn ) _qtWindow->setCursor( Qt::BlankCursor );
        else _qtWindow->setCursor( _currentCursor );
    }
}

void GraphicsWindowQt::setCursor( MouseCursor cursor )
{
    if ( cursor==InheritCursor && _qtWindow )
    {
        _qtWindow->unsetCursor();
    }

    switch ( cursor )
    {
    case NoCursor: _currentCursor = Qt::BlankCursor; break;
    case RightArrowCursor: case LeftArrowCursor: _currentCursor = Qt::ArrowCursor; break;
    case InfoCursor: _currentCursor = Qt::SizeAllCursor; break;
    case DestroyCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case HelpCursor: _currentCursor = Qt::WhatsThisCursor; break;
    case CycleCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case SprayCursor: _currentCursor = Qt::SizeAllCursor; break;
    case WaitCursor: _currentCursor = Qt::WaitCursor; break;
    case TextCursor: _currentCursor = Qt::IBeamCursor; break;
    case CrosshairCursor: _currentCursor = Qt::CrossCursor; break;
    case HandCursor: _currentCursor = Qt::OpenHandCursor; break;
    case UpDownCursor: _currentCursor = Qt::SizeVerCursor; break;
    case LeftRightCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopSideCursor: case BottomSideCursor: _currentCursor = Qt::UpArrowCursor; break;
    case LeftSideCursor: case RightSideCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    default: break;
    };
    if ( _qtWindow ) _qtWindow->setCursor( _currentCursor );
}

bool GraphicsWindowQt::valid() const
{
    return _qtWindow; // && _qtWindow->isValid();
}

bool GraphicsWindowQt::realizeImplementation()
{
    if (_realized) {
        return false;
    }

    if (!_qtWindow) {
        qWarning() << "no window created";
        return false;
    }

    _qtWindow->create();
    _realized = true;
    // make sure the event queue has the correct window rectangle size and input range
    //getEventQueue()->syncWindowRectangleWithGraphcisContext();

    _qtWindow->show();
    return true;
}

bool GraphicsWindowQt::isRealizedImplementation() const
{
    return _realized;
}

void GraphicsWindowQt::closeImplementation()
{
    if ( _qtWindow )
        _qtWindow->close();
    _realized = false;
}

void GraphicsWindowQt::runOperations()
{
#if 0
    // While in graphics thread this is last chance to do something useful before
    // graphics thread will execute its operations.
    if (_qtWindow->getNumDeferredEvents() > 0)
        _qtWindow->processDeferredEvents();
#endif
    QGuiApplication::processEvents();

    if (QOpenGLContext::currentContext() != _qtContext) {
        _qtContext->makeCurrent(_qtWindow);
    }

    GraphicsWindow::runOperations();
}

bool GraphicsWindowQt::makeCurrentImplementation()
{
#if 0
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();
#endif

    if (!_qtContext) {
        _qtContext = new QOpenGLContext;
        _qtContext->setFormat(_qtWindow->format());
        _qtContext->create();
    }

    bool ok = _qtContext->makeCurrent(_qtWindow);
    if (!ok) {
        qWarning() << "failed to make context current:" << _qtContext << _qtWindow;
    }
    return ok;
}

bool GraphicsWindowQt::releaseContextImplementation()
{
    _qtContext->doneCurrent();
    return true;
}

void GraphicsWindowQt::swapBuffersImplementation()
{
    // QOpenGLContext complains if we swap on an non-exposed QWindow
    if (!_qtWindow || !_qtWindow->isExposed()) {
        return;
    }

    // OSG calls this on the draw thread, hopefully that's okay for Qt
    _qtContext->swapBuffers(_qtWindow);
}

void GraphicsWindowQt::requestWarpPointer( float x, float y )
{
    if ( _qtWindow ) {
        QPoint pt = QPointF(x,y).toPoint();
        QCursor::setPos( _qtWindow->mapToGlobal(pt));
    }
}

void GraphicsWindowQt::resizedImplementation(int x, int y, int width, int height)
{
#if 0
    //GraphicsContext::resizedImplementation(0, 0, width, height);

    for(Cameras::iterator itr = _cameras.begin();
        itr != _cameras.end();
        ++itr)
    {
        osg::Camera* camera = (*itr);
        osg::Viewport* viewport = camera->getViewport();
        viewport->setViewport(0, 0, width, height);
    }
#endif
}


class Qt5WindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:

    Qt5WindowingSystem()
    {
        OSG_INFO << "Qt5WindowingSystemInterface()" << std::endl;
    }

    ~Qt5WindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
    }

    // Access the Qt windowing system through this singleton class.
    static Qt5WindowingSystem* getInterface()
    {
        static Qt5WindowingSystem* qtInterface = new Qt5WindowingSystem;
        return qtInterface;
    }

    // Return the number of screens present in the system
    virtual unsigned int getNumScreens( const osg::GraphicsContext::ScreenIdentifier& si )
    {
        Q_UNUSED(si);
        return QGuiApplication::screens().count();
    }

    // Return the resolution of specified screen
    // (0,0) is returned if screen is unknown
    virtual void getScreenSettings( const osg::GraphicsContext::ScreenIdentifier& si, osg::GraphicsContext::ScreenSettings & resolution )
    {
        QScreen* qtScreen = qtScreenForIdentifier(si);
        resolution.height = qtScreen->geometry().height();
        resolution.width = qtScreen->geometry().width();
        resolution.colorDepth = qtScreen->depth();
        resolution.refreshRate = qtScreen->refreshRate();
    }

    // Set the resolution for given screen
    virtual bool setScreenSettings( const osg::GraphicsContext::ScreenIdentifier& si, const osg::GraphicsContext::ScreenSettings & resolution )
    {
        OSG_WARN << "osgQt: setScreenSettings() not implemented yet." << std::endl;
        return false;
    }

    // Enumerates available resolutions
    virtual void enumerateScreenSettings( const osg::GraphicsContext::ScreenIdentifier& screenIdentifier, osg::GraphicsContext::ScreenSettingsList & resolution )
    {
        OSG_WARN << "osgQt: enumerateScreenSettings() not implemented yet." << std::endl;
    }

    // Create a graphics context with given traits
    virtual osg::GraphicsContext* createGraphicsContext( osg::GraphicsContext::Traits* traits )
    {
        qDebug() << "creating graphics context";
        if (traits->pbuffer)
        {
            OSG_WARN << "osgQt: createGraphicsContext - pbuffer not implemented yet." << std::endl;
            return NULL;
        }
        else
        {
            osg::ref_ptr< GraphicsWindowQt > window = new GraphicsWindowQt( traits );
            if (window->valid()) return window.release();
            else return NULL;
        }
    }

private:
    QScreen* qtScreenForIdentifier(const osg::GraphicsContext::ScreenIdentifier& screenIdentifier)
    {
        // FIXME
        return QGuiApplication::screens().front();
    }

    // No implementation for these
    Qt5WindowingSystem( const Qt5WindowingSystem& );
    Qt5WindowingSystem& operator=( const Qt5WindowingSystem& );
};


// declare C entry point for static compilation.
void graphicswindow_Qt5(void)
{
    osg::GraphicsContext::setWindowingSystemInterface(Qt5WindowingSystem::getInterface());
}

