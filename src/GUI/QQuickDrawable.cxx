

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
#include <osgViewer/GraphicsWindow>

#include <GUI/FGQmlInstance.hxx>
#include <GUI/FGQmlPropertyNode.hxx>

#include <Viewer/GraphicsWindowQt5.hxx>

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
        }
        default:
            break;
        }
        
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
        qmlRegisterSingletonType<FGQmlInstance>("flightgear.org", 1, 0, "System", fgqmlinstance_provider);

        // QML types
        qmlRegisterType<FGQmlPropertyNode>("flightgear.org", 1, 0, "Property");
    }
}

QQuickDrawable::~QQuickDrawable()
{
    delete d->qmlEngine;
    delete d->renderControl;
    delete d;
}

void QQuickDrawable::setup(osgViewer::GraphicsWindow* gw)
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
    
    // insert ourselves as a filter on the GraphicsWindow, so we can intercept
    // events directly and pass on to our window
    d->graphicsWindow->getGLWindow()->installEventFilter(d);
}

void QQuickDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    if (d->syncRequired) {
        // should happen on the main thread
        d->renderControl->polishItems();
        
        d->syncRequired = false;
        // must ensure the GUI thread is blocked for this
        d->renderControl->sync();
        // can unblock now
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
    
    glFuncs->glFlush();
    
}

void QQuickDrawable::setSource(QUrl url)
{
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
    const float currentPixelRatio = d->graphicsWindow->getGLWindow()->devicePixelRatio();
    const int logicalWidth = static_cast<int>(width / currentPixelRatio);
    const int logicalHeight = static_cast<int>(height / currentPixelRatio);
    
    if (d->rootItem) {
        d->rootItem->setWidth(logicalWidth);
        d->rootItem->setHeight(logicalHeight);
    }
    
    d->quickWindow->setGeometry(0, 0, logicalWidth, logicalHeight);
}

#include "QQuickDrawable.moc"
