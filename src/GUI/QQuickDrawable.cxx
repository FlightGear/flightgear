// QQuickDrawable.cxx - OSG Drawable using a QQuickRenderControl to draw
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

#include "config.h"

#include <simgear/compiler.h>
#include <atomic>

#include <QOpenGLContext>

#include "QQuickDrawable.hxx"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickRenderControl>

#include <QTimer>
#include <QCoreApplication>
#include <QOpenGLFunctions>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QThread>


// private Qt headers, needed to make glue work between Qt and OSG
// graphics window unfortunately.
#include <private/qopenglcontext_p.h>

#if defined(SG_MAC)
#  include "fake_qguiapp_p.h"
#else
#  include <private/qguiapplication_p.h>
#endif

#include <osg/GraphicsContext>
#include <osgGA/GUIEventAdapter>
#include <osgGA/GUIEventHandler>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/Viewer>

#include <GUI/DialogStateController.hxx>
#include <GUI/FGQQWindowManager.hxx>
#include <GUI/FGQmlInstance.hxx>
#include <GUI/FGQmlPropertyNode.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <GUI/OSGQtAdaption.hxx>
#include <simgear/structure/commands.hxx>

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

class CustomRenderControl : public QQuickRenderControl
{
public:
    CustomRenderControl(QWindow* win)
        : _qWindow(win)
    {
        // Q_ASSERT(win);
    }

    QWindow* renderWindow(QPoint* offset) override
    {
        if (offset) {
            *offset = QPoint(0, 0);
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
    QQuickDrawablePrivate() :
        renderControlInited(false)
    {
        
    }
    
    ~QQuickDrawablePrivate()
    {

    }
    
    CustomRenderControl* renderControl = nullptr;

    QQmlComponent* qmlComponent = nullptr;
    QQmlEngine* qmlEngine = nullptr;
    bool syncRequired = true;

    QQuickItem* rootItem = nullptr;

    // this window is neither created()-ed nor shown but is needed by
    // QQuickRenderControl for historical reasons, and gives us a place to
    // inject events from the OSG side.
    QQuickWindow* quickWindow = nullptr;

    // window representing the OSG window, needed
    // for making our adpoted context current
    QWindow* foreignOSGWindow = nullptr;
    QOpenGLContext* qtContext = nullptr;
    osg::GraphicsContext* osgContext = nullptr;
    
    std::atomic_int renderControlInited;
    std::atomic_int syncPending;
    
    void frameEvent()
    {
        if (syncRequired) {
            renderControl->polishItems();
            syncRequired = false;
            syncPending = true;
            
            osgContext->add(flightgear::makeGraphicsOp("Sync QQ2 Render control", [this](osg::GraphicsContext*) {
                QOpenGLContextPrivate::setCurrentContext(qtContext);
                renderControl->sync();
                syncPending = false;
            }));
        }
    }
public slots:
    void onComponentLoaded()
    {
        if (qmlComponent->isError()) {
            QList<QQmlError> errorList = qmlComponent->errors();
            Q_FOREACH (const QQmlError& error, errorList) {
                qWarning() << error.url() << error.line() << error;
            }
            return;
        }

        QObject* rootObject = qmlComponent->create();
        if (qmlComponent->isError()) {
            QList<QQmlError> errorList = qmlComponent->errors();
            Q_FOREACH (const QQmlError& error, errorList) {
                qWarning() << error.url() << error.line() << error;
            }
            return;
        }

        rootItem = qobject_cast<QQuickItem*>(rootObject);
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
    
    void initRenderControl()
    {
        qtContext = flightgear::qtContextFromOSG(osgContext);
        renderControl->prepareThread(QThread::currentThread());                
        QOpenGLContextPrivate::setCurrentContext(qtContext);
        QOpenGLContextPrivate::get(qtContext)->surface = foreignOSGWindow;
        renderControl->initialize(qtContext);
        
        renderControlInited = true;
    }

    void onSceneChanged()
    {
        syncRequired = true;
    }

    void onRenderRequested()
    {
        qWarning() << Q_FUNC_INFO;
    }
    
    void onWindowActiveFocusItemChanged()
    {
        if (quickWindow->activeFocusItem())
            qInfo() << Q_FUNC_INFO << "Active focus item is now:" << quickWindow->activeFocusItem();
        else
            qInfo() << Q_FUNC_INFO << "Active focus cleared";
    }
};


static QObject* fgqmlinstance_provider(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    FGQmlInstance* instance = new FGQmlInstance;
    return instance;
}

static QObject* fgqq_windowManager_provider(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(scriptEngine)

    FGQQWindowManager* instance = new FGQQWindowManager(engine);
    return instance;
}

class ReloadCommand : public SGCommandMgr::Command
{
public:
    ReloadCommand(QQuickDrawable* qq) : _drawable(qq)
    {
    }

    bool operator()(const SGPropertyNode* aNode, SGPropertyNode* root) override
    {
        SG_UNUSED(aNode);
        SG_UNUSED(root);
        
        QTimer::singleShot(0, [this]() {
            std::string rootQMLPath = fgGetString("/sim/gui/qml-root-path");
            _drawable->reload(QUrl::fromLocalFile(QString::fromStdString(rootQMLPath)));
        });
        return true;
    }

private:
    QQuickDrawable* _drawable;
};

class QuickEventHandler : public osgGA::GUIEventHandler
{
public:
    QuickEventHandler(QQuickDrawablePrivate* p) : _drawable(p)
    {
        populateKeymap();
    }

    bool handle(const GUIEventAdapter& ea, GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor*) override
    {
        Q_UNUSED(aa);

        if (ea.getHandled()) return false;

        // frame event, ho hum ...
        if (ea.getEventType() == GUIEventAdapter::FRAME) {
            _drawable->frameEvent();
            return false;
        }

        // Qt expects increasing downward mouse coords
        const float fixedY = (ea.getMouseYOrientation() == GUIEventAdapter::Y_INCREASING_UPWARDS) ? ea.getWindowHeight() - ea.getY() : ea.getY();
        const double pixelRatio = _drawable->foreignOSGWindow->devicePixelRatio();

        QPointF pointInWindow{ea.getX() / pixelRatio, fixedY / pixelRatio};
        QPointF screenPt = pointInWindow +
                           QPointF{ea.getWindowX() / pixelRatio, ea.getWindowY() / pixelRatio};

        //  const int scaledX = static_cast<int>(ea.getX() / static_pixelRatio);
        // const int scaledY = static_cast<int>(fixedY / static_pixelRatio);

        switch (ea.getEventType()) {
        case (GUIEventAdapter::DRAG):
        case (GUIEventAdapter::MOVE): {
            QMouseEvent m(QEvent::MouseMove, pointInWindow, pointInWindow, screenPt,
                          Qt::NoButton,
                          osgButtonMaskToQt(ea), osgModifiersToQt(ea));
            QCoreApplication::sendEvent(_drawable->quickWindow, &m);
            return m.isAccepted();
        }

        case (GUIEventAdapter::PUSH):
        case (GUIEventAdapter::RELEASE): {
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

        case (GUIEventAdapter::KEYDOWN):
        case (GUIEventAdapter::KEYUP): {
            if (!_drawable->quickWindow->activeFocusItem()) {
                return false;
            }
            
            const bool isKeyRelease = (ea.getEventType() == GUIEventAdapter::KEYUP);
            const auto& key = osgKeyToQt(ea.getKey());
            QString s = key.s;

            QKeyEvent k(isKeyRelease ? QEvent::KeyRelease : QEvent::KeyPress,
                        key.qt, osgModifiersToQt(ea), s);
            QCoreApplication::sendEvent(_drawable->quickWindow, &k);
            return k.isAccepted();
        }
                
        default:
            return false;
        }

        return false;
    }
private:
    Qt::MouseButtons osgButtonMaskToQt(const osgGA::GUIEventAdapter& ea) const
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

    Qt::MouseButton osgButtonToQt(const osgGA::GUIEventAdapter& ea) const
    {
        switch (ea.getButton()) {
        case osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON: return Qt::LeftButton;
        case osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON: return Qt::MiddleButton;
        case osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON: return Qt::RightButton;

        default:
            break;
        }

        return Qt::NoButton;
    }

    Qt::KeyboardModifiers osgModifiersToQt(const osgGA::GUIEventAdapter& ea) const
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
        for (int i = 0; i < 10; ++i) {
            global_keymap.emplace_back(GUIEventAdapter::KEY_0 + i, Qt::Key_0 + i, QString::number(i));
        }

        for (int i = 0; i < 26; ++i) {
            global_keymap.emplace_back(GUIEventAdapter::KEY_A + i, Qt::Key_A + i, QChar::fromLatin1('a' + i));
        }

        for (int i = 0; i < 26; ++i) {
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

QQuickDrawable::QQuickDrawable() : d(new QQuickDrawablePrivate)
{
    setUseDisplayList(false);
    setDataVariance(Object::DYNAMIC);

    osg::StateSet* stateSet = getOrCreateStateSet();
    stateSet->setRenderBinDetails(1001, "RenderBin");

    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);

    static bool doneQmlRegistration = false;
    if (!doneQmlRegistration) {
        doneQmlRegistration = true;

        // singleton system object
        qmlRegisterSingletonType<FGQmlInstance>("FlightGear", 1, 0, "System", fgqmlinstance_provider);
        qmlRegisterSingletonType<FGQmlInstance>("FlightGear", 1, 0, "WindowManager", fgqq_windowManager_provider);

        // QML types
        qmlRegisterType<FGQmlPropertyNode>("FlightGear", 1, 0, "Property");
        qmlRegisterType<DialogStateController>("FlightGear", 1, 0, "DialogStateController");
    }
    
      globals->get_commands()->addCommandObject("reload-quick-gui", new ReloadCommand(this));
}

QQuickDrawable::~QQuickDrawable()
{
    delete d->qmlEngine;
    delete d->renderControl;
}

void QQuickDrawable::setup(osgViewer::GraphicsWindow *gw, osgViewer::Viewer *viewer)
{
    osg::GraphicsContext* gc = gw;

    // none of this stuff needs the context current, so we can do it
    // all safely on the main thread
    
    d->foreignOSGWindow = flightgear::qtWindowFromOSG(gw);
    // d->foreignOSGWindow->setFormat(format);
    d->foreignOSGWindow->setSurfaceType(QSurface::OpenGLSurface);
    
    // QWindow::requestActive would do QPA::makeKey, but on macOS this
    // is a no-op for foreign windows. So we're going to manually set
    // the focus window!
    QGuiApplicationPrivate::focus_window = d->foreignOSGWindow;
    
    d->osgContext = gc;
    d->renderControl = new CustomRenderControl(d->foreignOSGWindow);
    d->quickWindow = new QQuickWindow(d->renderControl);
    d->quickWindow->setClearBeforeRendering(false);

    d->qmlEngine = new QQmlEngine;

    SGPath rootQMLPath = SGPath::fromUtf8(fgGetString("/sim/gui/qml-root-path"));
    SG_LOG(SG_GENERAL, SG_INFO, "Root QML dir:" << rootQMLPath.dir());
    d->qmlEngine->addImportPath(QString::fromStdString(rootQMLPath.dir()));
    // d->qmlEngine->addImportPath(QStringLiteral("qrc:///"));

    if (!d->qmlEngine->incubationController())
        d->qmlEngine->setIncubationController(d->quickWindow->incubationController());

 //   QObject::connect(d->quickWindow, &QQuickWindow::activeFocusItemChanged,
  //                    d.get(), &QQuickDrawablePrivate::onWindowActiveFocusItemChanged);
    
    QObject::connect(d->renderControl, &QQuickRenderControl::sceneChanged,
                     d.get(), &QQuickDrawablePrivate::onSceneChanged);
    QObject::connect(d->renderControl, &QQuickRenderControl::renderRequested,
                     d.get(), &QQuickDrawablePrivate::onRenderRequested);


    viewer->getEventHandlers().push_front(new QuickEventHandler(d.get()));
}

void QQuickDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    if (!d->renderControlInited) {
        d->initRenderControl();
    }
    
    if (QOpenGLContext::currentContext() != d->qtContext) {
        QOpenGLContextPrivate::setCurrentContext(d->qtContext);
    }
    
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
}

void QQuickDrawable::reload(const QUrl& url)
{
    d->qmlEngine->clearComponentCache();
    setSource(url);
}

void QQuickDrawable::setSourcePath(const std::string& path)
{
    setSource(QUrl::fromLocalFile(QString::fromStdString(path)));
}

void QQuickDrawable::setSource(const QUrl& url)
{
    if (d->rootItem)
        delete d->rootItem;
    if (d->qmlComponent)
        delete d->qmlComponent;
    d->rootItem = nullptr;

    d->qmlComponent = new QQmlComponent(d->qmlEngine, url);
    if (d->qmlComponent->isLoading()) {
        QObject::connect(d->qmlComponent, &QQmlComponent::statusChanged,
                         d.get(), &QQuickDrawablePrivate::onComponentLoaded);
    } else {
        d->onComponentLoaded();
    }
}

void QQuickDrawable::resize(int width, int height)
{
    // we need to unscale from physical pixels back to logical, otherwise we end up double-scaled.
    const float currentPixelRatio = static_cast<float>(d->foreignOSGWindow->devicePixelRatio());
    const int logicalWidth = static_cast<int>(width / currentPixelRatio);
    const int logicalHeight = static_cast<int>(height / currentPixelRatio);

    if (d->rootItem) {
        d->rootItem->setWidth(logicalWidth);
        d->rootItem->setHeight(logicalHeight);
    }

//    SG_LOG(SG_GUI, SG_INFO, "Resize:, lw=" << logicalWidth << ", lh=" << logicalHeight);
    d->quickWindow->setGeometry(0, 0, logicalWidth, logicalHeight);
}

#include "QQuickDrawable.moc"
