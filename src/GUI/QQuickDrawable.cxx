#include "config.h"

#include "QQuickDrawable.hxx"

#include <QQuickRenderControl>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>

#include <QThread>
#include <QOpenGLContext>
#include <QQuickWindow>
#include <QOpenGLFunctions>
#include <QQuickItem>
#include <QCoreApplication>

#include <osg/GraphicsContext>
#include <osgGA/GUIEventHandler>
#include <osgGA/GUIEventAdapter>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <GUI/FGQmlInstance.hxx>
#include <GUI/FGQmlPropertyNode.hxx>
#include <GUI/FGQQWindowManager.hxx>

#include <Viewer/GraphicsWindowQt5.hxx>

#include <simgear/structure/commands.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#if defined(HAVE_PUI)
    #include <plib/pu.h>
#endif

using namespace osgGA;

struct QtKey {
    QtKey(int _o, int _q, QString _s = {}) : osg(_o), qt(_q), s(_s) {}
    
    int osg;
    int qt;
    QString s;
};

const std::initializer_list<QtKey> keymapInit = {
    // contains all the key mappings except 0..9 and A..Z which are generated
    // programatically
    
    {GUIEventAdapter::KEY_Space, Qt::Key_Space, " "},
    {GUIEventAdapter::KEY_Escape, Qt::Key_Escape, "\x1B"},
    {GUIEventAdapter::KEY_Return, Qt::Key_Return, "\r"},
    {GUIEventAdapter::KEY_Tab, Qt::Key_Tab, "\t"},
    {GUIEventAdapter::KEY_BackSpace, Qt::Key_Backspace, "\x08"},
    {GUIEventAdapter::KEY_Delete, Qt::Key_Delete, "\x7f"},

    {GUIEventAdapter::KEY_Period, Qt::Key_Period, "."},
    {GUIEventAdapter::KEY_Comma, Qt::Key_Comma, ","},
    {GUIEventAdapter::KEY_Colon, Qt::Key_Colon, ":"},
    {GUIEventAdapter::KEY_Quote, Qt::Key_QuoteLeft, "'"},
    {GUIEventAdapter::KEY_Quotedbl, Qt::Key_QuoteDbl, "\""},
    {GUIEventAdapter::KEY_Underscore, Qt::Key_Underscore, "_"},
    {GUIEventAdapter::KEY_Plus, Qt::Key_Plus, "+"},
    {GUIEventAdapter::KEY_Minus, Qt::Key_Minus, "-"},
    {GUIEventAdapter::KEY_Asterisk, Qt::Key_Asterisk, "*"},
    {GUIEventAdapter::KEY_Equals, Qt::Key_Equal, "="},
    {GUIEventAdapter::KEY_Slash, Qt::Key_Slash, "/"},
    
    {GUIEventAdapter::KEY_Left, Qt::Key_Left},
    {GUIEventAdapter::KEY_Right, Qt::Key_Right},
    {GUIEventAdapter::KEY_Up, Qt::Key_Up},
    {GUIEventAdapter::KEY_Down, Qt::Key_Down},
    
    {GUIEventAdapter::KEY_Shift_L, Qt::Key_Shift},
    {GUIEventAdapter::KEY_Shift_R, Qt::Key_Shift},
    {GUIEventAdapter::KEY_Control_L, Qt::Key_Control},
    {GUIEventAdapter::KEY_Control_R, Qt::Key_Control},
    {GUIEventAdapter::KEY_Meta_L, Qt::Key_Meta},
    {GUIEventAdapter::KEY_Meta_R, Qt::Key_Meta},



};

std::vector<QtKey> global_keymap;

/**
 * Helper run on Graphics thread to retrive its Qt wrapper
 */
class RetriveGraphicsThreadOperation : public osg::GraphicsOperation
{
public:
    RetriveGraphicsThreadOperation()
    : osg::GraphicsOperation("RetriveGraphcisThread", false)
    {}
    
    QThread* thread() const { return _result; }
    QOpenGLContext* context() const { return _context; }
    
    virtual void operator () (osg::GraphicsContext* context) override
    {
        _result = QThread::currentThread();
        _context = QOpenGLContext::currentContext();
        if (!_context) {
            qFatal("Use a Qt-based GraphicsWindow/Context");
            // TODO: use ability to create a QOpenGLContext from native
            // however, this will also need event-handling to be re-written
            //_context = qtContextFromOSG(context);
        }
    }
private:
    QThread* _result = nullptr;
    QOpenGLContext* _context = nullptr;
};

#if 0
class SyncPolishOperation : public osg::Operation
{
public:
    SyncPolishOperation(QQuickRenderControl* rc) :
    renderControl(rc)
    {
        setKeep(true);
    }
    
    virtual void operator () (osg::Object*) override
    {
        if (syncRequired) {
            //syncRequired = false;

        }

        renderControl->polishItems();
    }
    
    bool syncRequired = true;
    QQuickRenderControl* renderControl;
};
#endif

class CustomRenderControl : public QQuickRenderControl
{
public:
    CustomRenderControl(QWindow* win)
    : _qWindow(win)
    {
        Q_ASSERT(win);
    }
    
    QWindow* renderWindow(QPoint* offset) override
    {
        if (offset) {
            *offset = QPoint(0,0);
        }
        return _qWindow;
    }
private:
    QWindow* _qWindow = nullptr;
};

class QQuickDrawablePrivate : public QObject
{
    Q_OBJECT
public:
    
    CustomRenderControl* renderControl = nullptr;
    
    // the window our osgViewer is using
    flightgear::GraphicsWindowQt5* graphicsWindow = nullptr;
    
    QQmlComponent* qmlComponent = nullptr;
    QQmlEngine* qmlEngine = nullptr;
    bool syncRequired = true;
    
    QQuickItem* rootItem = nullptr;
    
    // this window is neither created()-ed nor shown but is needed by
    // QQuickRenderControl for historical reasons, and gives us a place to
    // inject events from the OSG side.
    QQuickWindow* quickWindow = nullptr;
    
    QOpenGLContext* qtContext = nullptr;
    
public slots:
    void onComponentLoaded()
    {
        if (qmlComponent->isError()) {
            QList<QQmlError> errorList = qmlComponent->errors();
            Q_FOREACH (const QQmlError &error, errorList) {
                qWarning() << error.url() << error.line() << error;
            }
            return;
        }
        
        QObject *rootObject = qmlComponent->create();
        if (qmlComponent->isError()) {
            QList<QQmlError> errorList = qmlComponent->errors();
            Q_FOREACH (const QQmlError &error, errorList) {
                qWarning() << error.url() << error.line() << error;
            }
            return;
        }
        
        rootItem = qobject_cast<QQuickItem *>(rootObject);
        if (!rootItem) {
            qWarning() << Q_FUNC_INFO << "root object not a QQuickItem" << rootObject;
            delete rootObject;
            return;
        }
        
        // The root item is ready. Associate it with the window.
        rootItem->setParentItem(quickWindow->contentItem());
        syncRequired = true;
        rootItem->setWidth(quickWindow->width());
        rootItem->setHeight(quickWindow->height());
    }
    
    void onSceneChanged()
    {
        syncRequired = true;
    }
    
    void onRenderRequested()
    {
        qWarning() << Q_FUNC_INFO;
    }
    
    bool eventFilter(QObject* obj, QEvent* event) override
    {
#if 0
        if (obj != graphicsWindow->getGLWindow()) {
            return false;
        }
        
        const auto eventType = event->type();
        switch (eventType) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            bool result = QCoreApplication::sendEvent(quickWindow, event);
            // result always seems to be true here, not useful
            if (result && event->isAccepted()) {
                return true;
            }
            break;
        }

        default:
            break;
        }
#endif
        return false;
    }
};

static QObject* fgqmlinstance_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    FGQmlInstance* instance = new FGQmlInstance;
    return instance;
}

static QObject* fgqq_windowManager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)

    FGQQWindowManager* instance = new FGQQWindowManager(engine);
    return instance;
}

class ReloadCommand : public SGCommandMgr::Command
{
public:
    ReloadCommand(QQuickDrawable* qq) :
        _drawable(qq)
    {}

    bool operator() (const SGPropertyNode* aNode, SGPropertyNode * root) override
    {
        SG_UNUSED(aNode);
        SG_UNUSED(root);
        std::string rootQMLPath = fgGetString("/sim/gui/qml-root-path");
        _drawable->reload(QUrl::fromLocalFile(QString::fromStdString(rootQMLPath)));
        return true;
    }

private:
    QQuickDrawable* _drawable;
};

class QuickEventHandler : public GUIEventHandler
{
public:
    QuickEventHandler(QQuickDrawablePrivate* p) :
        _drawable(p)
    {
        populateKeymap();
    }

    bool handle(const GUIEventAdapter &ea, GUIActionAdapter &aa, osg::Object *, osg::NodeVisitor*) override
    {
        Q_UNUSED(aa);

        if (ea.getHandled()) return false;

        // frame event, ho hum ...
        if (ea.getEventType() == GUIEventAdapter::FRAME) {
            if (_drawable->syncRequired) {
                _drawable->renderControl->polishItems();
                _drawable->syncRequired = false;
                _drawable->renderControl->sync();
            }

            return false;
        }
        
     //   qInfo() << "GA:" << ea.getX() << "," << ea.getY() << ", window:" << ea.getWindowX() << "," << ea.getWindowY();
        
        // Qt expects increasing downward mouse coords
        const float fixedY = (ea.getMouseYOrientation() == GUIEventAdapter::Y_INCREASING_UPWARDS) ?
                            ea.getWindowHeight() - ea.getY() : ea.getY();
        
       // qInfo() << "\tfixed Y" << fixedY;
        
        const double pixelRatio = _drawable->graphicsWindow->getGLWindow()->devicePixelRatio();

        QPointF pointInWindow{ea.getX() / pixelRatio, fixedY / pixelRatio};
        QPointF screenPt = pointInWindow +
                QPointF{ea.getWindowX() / pixelRatio, ea.getWindowY() / pixelRatio};

      //  const int scaledX = static_cast<int>(ea.getX() / static_pixelRatio);
       // const int scaledY = static_cast<int>(fixedY / static_pixelRatio);

        switch(ea.getEventType())
        {
            case(GUIEventAdapter::DRAG):
            case(GUIEventAdapter::MOVE):
            {
                QMouseEvent m(QEvent::MouseMove, pointInWindow, pointInWindow, screenPt,
                              Qt::NoButton,
                              osgButtonMaskToQt(ea), osgModifiersToQt(ea));
                QCoreApplication::sendEvent(_drawable->quickWindow, &m);
                return m.isAccepted();
            }

            case(GUIEventAdapter::PUSH):
            case(GUIEventAdapter::RELEASE):
            {
                const bool isUp = (ea.getEventType() == GUIEventAdapter::RELEASE);
                QMouseEvent m(isUp ? QEvent::MouseButtonRelease : QEvent::MouseButtonPress,
                               pointInWindow, pointInWindow, screenPt,
                          osgButtonToQt(ea),
                          osgButtonMaskToQt(ea), osgModifiersToQt(ea));
                QCoreApplication::sendEvent(_drawable->quickWindow, &m);
                
                if (!isUp) {
                    if (m.isAccepted()) {
                        // deactivate PUI
                        auto active = puActiveWidget();
                        if (active) {
                            active->invokeDownCallback();
                        }
                    }
                
                    // on mouse downs which we don't accept, take focus back
//                    qInfo() << "Clearing QQ focus";
//                    auto focused = _drawable->quickWindow->activeFocusItem();
//                    if (focused) {
//                        focused->setFocus(false, Qt::MouseFocusReason);
//                    }
                }
                
                return m.isAccepted();
            }

            case(GUIEventAdapter::KEYDOWN):
            case(GUIEventAdapter::KEYUP):
            {
                const bool isKeyRelease = (ea.getEventType() == GUIEventAdapter::KEYUP);
                const auto& key = osgKeyToQt(ea.getKey());
                QString s = key.s;
                
                QKeyEvent k(isKeyRelease ? QEvent::KeyRelease : QEvent::KeyPress,
                            key.qt, osgModifiersToQt(ea), s);
                QCoreApplication::sendEvent(_drawable->quickWindow, &k);
                return k.isAccepted();
            }

//            case osgGA::GUIEventAdapter::SCROLL:
//            {
//                const int button = buttonForScrollEvent(ea);
//                if (button != PU_NOBUTTON) {
//                    // sent both down and up events for a single scroll, for
//                    // compatability
//                    bool handled = puMouse(button, PU_DOWN, scaledX, scaledY);
//                    handled |= puMouse(button, PU_UP, scaledX, scaledY);
//                    return handled;
//                }
//                return false;
//            }

//            case (osgGA::GUIEventAdapter::RESIZE):
//                _puiCamera->resizeUi(ea.getWindowWidth(), ea.getWindowHeight());
//                break;

            default:
                return false;
        }

        return false;
    }
private:
    Qt::MouseButtons osgButtonMaskToQt(const osgGA::GUIEventAdapter &ea) const
    {
        const int mask = ea.getButtonMask();
        Qt::MouseButtons result = Qt::NoButton;
        if (mask & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
           result |= Qt::LeftButton;

        if (mask & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
           result |= Qt::MiddleButton;

        if (mask & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
           result |= Qt::RightButton;

        return result;
    }

    Qt::MouseButton osgButtonToQt(const osgGA::GUIEventAdapter &ea) const
    {
        switch (ea.getButton()) {
        case osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON:     return Qt::LeftButton;
        case osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON:   return Qt::MiddleButton;
        case osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON:    return Qt::RightButton;

        default:
            break;
        }

        return Qt::NoButton;
    }

    Qt::KeyboardModifiers osgModifiersToQt(const osgGA::GUIEventAdapter &ea) const
    {
        Qt::KeyboardModifiers result = Qt::NoModifier;
        const int mask = ea.getModKeyMask();
        if (mask & osgGA::GUIEventAdapter::MODKEY_ALT) result |= Qt::AltModifier;
        if (mask & osgGA::GUIEventAdapter::MODKEY_CTRL) result |= Qt::ControlModifier;
        if (mask & osgGA::GUIEventAdapter::MODKEY_META) result |= Qt::MetaModifier;
        if (mask & osgGA::GUIEventAdapter::MODKEY_SHIFT) result |= Qt::ShiftModifier;
        return result;
    }

    QtKey osgKeyToQt(int code) const
    {
        auto it = std::lower_bound(global_keymap.begin(), global_keymap.end(),
                                     QtKey{code, 0, {}},
                                     [](const QtKey& a, const QtKey& b) { return a.osg < b.osg; });
        if ((it == global_keymap.end()) || (it->osg != code)) {
            qWarning() << "no mapping defined for OSG key:" << code;
            return {0, 0, ""};
        }
        
        return *it;
    }
    
    void populateKeymap()
    {
        if (!global_keymap.empty())
            return;
        
        // regular keymappsing for A..Z and 0..9
        for (int i=0; i<10; ++i) {
            global_keymap.emplace_back(GUIEventAdapter::KEY_0 + i, Qt::Key_0 + i, QString::number(i));
        }
        
        for (int i=0; i<26; ++i) {
            global_keymap.emplace_back(GUIEventAdapter::KEY_A + i, Qt::Key_A + i, QChar::fromLatin1('a' + i));
        }
        
        for (int i=0; i<26; ++i) {
            global_keymap.emplace_back('A' + i, Qt::Key_A + i, QChar::fromLatin1('A' + i));
        }
        
        // custom key mappsing
        global_keymap.insert(global_keymap.end(), keymapInit);
        
        // sort by OSG code for fast lookups
        std::sort(global_keymap.begin(), global_keymap.end(),
                  [](const QtKey& a, const QtKey& b) { return a.osg < b.osg; });
    }
    
    QQuickDrawablePrivate* _drawable;
};

QQuickDrawable::QQuickDrawable() :
    d(new QQuickDrawablePrivate)
{
    setUseDisplayList(false);
    setDataVariance(Object::DYNAMIC);
    
    osg::StateSet* stateSet = getOrCreateStateSet();
    stateSet->setRenderBinDetails(1001, "RenderBin");
#if 0
    // speed optimization?
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    // We can do translucent menus, so why not. :-)
    stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);
    
    stateSet->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
    
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
#endif

    static bool doneQmlRegistration = false;
    if (!doneQmlRegistration) {
        doneQmlRegistration = true;

        // singleton system object
        qmlRegisterSingletonType<FGQmlInstance>("FlightGear", 1, 0, "System", fgqmlinstance_provider);
        qmlRegisterSingletonType<FGQmlInstance>("FlightGear", 1, 0, "WindowManager", fgqq_windowManager_provider);

        // QML types
        qmlRegisterType<FGQmlPropertyNode>("FlightGear", 1, 0, "Property");
    }

    globals->get_commands()->addCommandObject("reload-quick-gui", new ReloadCommand(this));
}

QQuickDrawable::~QQuickDrawable()
{
    delete d->qmlEngine;
    delete d->renderControl;
    delete d;
}

void QQuickDrawable::setup(osgViewer::GraphicsWindow* gw, osgViewer::Viewer* viewer)
{
    osg::GraphicsContext* gc = gw;
    
    // install operations on the graphics context to get the context
    osg::ref_ptr<RetriveGraphicsThreadOperation> op(new RetriveGraphicsThreadOperation);
    
    gc->add(op);
    gc->runOperations();
    
    // hopefully done now!
    
    d->qtContext = op->context();
    d->graphicsWindow = dynamic_cast<flightgear::GraphicsWindowQt5*>(gw);
    if (!d->graphicsWindow) {
        qFatal("QQuick drawable requires a GraphicsWindowQt5 for now");
    }
    d->renderControl = new CustomRenderControl(d->graphicsWindow->getGLWindow());
    
    d->quickWindow = new QQuickWindow(d->renderControl);
    d->quickWindow->setClearBeforeRendering(false);
    
    d->qmlEngine = new QQmlEngine;
    
    SGPath rootQMLPath = SGPath::fromUtf8(fgGetString("/sim/gui/qml-root-path"));
    SG_LOG(SG_GENERAL, SG_INFO, "Root QML dir:" << rootQMLPath.dir());
    d->qmlEngine->addImportPath(QString::fromStdString(rootQMLPath.dir()));
   // d->qmlEngine->addImportPath(QStringLiteral("qrc:///"));

    if (!d->qmlEngine->incubationController())
        d->qmlEngine->setIncubationController(d->quickWindow->incubationController());
    
    d->renderControl->initialize(d->qtContext);

#if QT_VERSION < 0x050600
    SG_LOG(SG_GUI, SG_ALERT, "Qt < 5.6 was used to build FlightGear, multi-threading of QtQuick is not safe");
#else
    d->renderControl->prepareThread(op->thread());
#endif
    QObject::connect(d->renderControl, &QQuickRenderControl::sceneChanged,
                     d, &QQuickDrawablePrivate::onSceneChanged);
    QObject::connect(d->renderControl, &QQuickRenderControl::renderRequested,
                     d, &QQuickDrawablePrivate::onRenderRequested);
  
#if 0
    // insert ourselves as a filter on the GraphicsWindow, so we can intercept
    // events directly and pass on to our window
    d->graphicsWindow->getGLWindow()->installEventFilter(d);
#endif

    viewer->getEventHandlers().push_front(new QuickEventHandler(d));
}

void QQuickDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{

    
    QOpenGLFunctions* glFuncs = d->qtContext->functions();
    // prepare any state QQ2 needs
    d->quickWindow->resetOpenGLState();
    
    // and reset these manually
    glFuncs->glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glFuncs->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFuncs->glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glFuncs->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    d->renderControl->render();
    
    // otherwise the PUI camera gets confused
    d->quickWindow->resetOpenGLState();

  //  glFuncs->glFlush();
    
}

void QQuickDrawable::reload(QUrl url)
{
    d->qmlEngine->clearComponentCache();
    setSource(url);
}

void QQuickDrawable::setSource(QUrl url)
{
    if (d->rootItem)
        delete d->rootItem;
    if (d->qmlComponent)
        delete d->qmlComponent;
    d->rootItem = nullptr;

    d->qmlComponent = new QQmlComponent(d->qmlEngine, url);
    if (d->qmlComponent->isLoading()) {
        QObject::connect(d->qmlComponent, &QQmlComponent::statusChanged,
                         d, &QQuickDrawablePrivate::onComponentLoaded);
    } else {
        d->onComponentLoaded();
    }
}

void QQuickDrawable::resize(int width, int height)
{
    // we need to unscale from physical pixels back to logical, otherwise we end up double-scaled.
    const float currentPixelRatio = static_cast<float>(d->graphicsWindow->getGLWindow()->devicePixelRatio());
    const int logicalWidth = static_cast<int>(width / currentPixelRatio);
    const int logicalHeight = static_cast<int>(height / currentPixelRatio);

    if (d->rootItem) {
        d->rootItem->setWidth(logicalWidth);
        d->rootItem->setHeight(logicalHeight);
    }
    
    d->quickWindow->setGeometry(0, 0, logicalWidth, logicalHeight);
#
}

#include "QQuickDrawable.moc"
