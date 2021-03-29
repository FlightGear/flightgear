#include "config.h" 

#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>

#include <QOpenGLContext>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQmlFileSelector>
#include <QQuickItem>

// launcher headers
#include "AddOnsController.hxx"
#include "AircraftItemModel.hxx"
#include "DefaultAircraftLocator.hxx"
#include "GettingStartedTip.hxx"
#include "LaunchConfig.hxx"
#include "LauncherController.hxx"
#include "LauncherNotificationsController.hxx"
#include "LauncherPackageDelegate.hxx"
#include "LocalAircraftCache.hxx"
#include "LocationController.hxx"
#include "QmlColoredImageProvider.hxx"
#include "QtLauncher.hxx"
#include "UpdateChecker.hxx"

#include <Main/sentryIntegration.hxx>

//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow(bool inSimMode) : QQuickView()
{
    setTitle("FlightGear " FLIGHTGEAR_VERSION);

    m_controller = new LauncherController(this, this);

    int styleTypeId = 0;
    m_controller->initQML(styleTypeId);

    // use a direct connection to be notified synchronously when the render thread
    // starts OpenGL. We use this to log the OpenGL information from the
    // context at that time, for tracing purposes.
    connect(this, &QQuickWindow::sceneGraphInitialized,
            this, &LauncherMainWindow::renderTheadSceneGraphInitialized,
            Qt::DirectConnection);

    m_coloredIconProvider = new QmlColoredImageProvider;
    engine()->addImageProvider("colored-icon", m_coloredIconProvider);

    if (!inSimMode) {
#if defined(Q_OS_MAC)
        QMenuBar* mb = new QMenuBar();

        QMenu* fileMenu = mb->addMenu(tr("File"));
        QAction* openAction = new QAction(tr("Open saved configuration..."));
        openAction->setMenuRole(QAction::NoRole);
        connect(openAction, &QAction::triggered,
                m_controller, &LauncherController::openConfig);

        QAction* saveAction = new QAction(tr("Save configuration as..."));
        saveAction->setMenuRole(QAction::NoRole);
        connect(saveAction, &QAction::triggered,
                m_controller, &LauncherController::saveConfigAs);

        fileMenu->addAction(openAction);
        fileMenu->addAction(saveAction);

        QMenu* toolsMenu = mb->addMenu(tr("Tools"));
        QAction* restoreDefaultsAction = new QAction(tr("Restore defaults..."));
        restoreDefaultsAction->setMenuRole(QAction::NoRole);
        connect(restoreDefaultsAction, &QAction::triggered,
                m_controller, &LauncherController::requestRestoreDefaults);

        QAction* changeDataAction = new QAction(tr("Select data files location..."));
        changeDataAction->setMenuRole(QAction::NoRole);
        connect(changeDataAction, &QAction::triggered,
                m_controller, &LauncherController::requestChangeDataPath);

        QAction* viewCommandLineAction = new QAction(tr("View command-line"));
        connect(viewCommandLineAction, &QAction::triggered,
                m_controller, &LauncherController::viewCommandLine);

        toolsMenu->addAction(restoreDefaultsAction);
        toolsMenu->addAction(changeDataAction);
        toolsMenu->addAction(viewCommandLineAction);
#endif

        QAction* qa = new QAction(this);
        qa->setMenuRole(QAction::QuitRole); // will be addeed accordingly
        qa->setShortcut(QKeySequence("Ctrl+Q"));
        connect(qa, &QAction::triggered, m_controller, &LauncherController::quit);
    }

    if (!checkQQC2Availability()) {
        QMessageBox::critical(nullptr, "Missing required component",
                              tr("Your system is missing a required UI component (QtQuick Controls 2). "
                                 "This normally occurs on Linux platforms where Qt is split into many small packages. "
                                 "On Ubuntu/Debian systems, the package is called 'qml-module-qtquick-controls2'"));
    }

    m_coloredIconProvider->loadStyleColors(engine(), styleTypeId);

    connect(this, &QQuickView::statusChanged, this, &LauncherMainWindow::onQuickStatusChanged);

    m_controller->initialRestoreSettings();

    ////////////
#if defined(Q_OS_WIN)
    const QString osName("win");
#elif defined(Q_OS_MAC)
    const QString osName("mac");
#else
    const QString osName("unix");
#endif

    setResizeMode(QQuickView::SizeRootObjectToView);
    engine()->addImportPath("qrc:///");

    QQmlContext* ctx = rootContext();
    ctx->setContextProperty("_launcher", m_controller);
    ctx->setContextProperty("_config", m_controller->config());
    ctx->setContextProperty("_location", m_controller->location());
    ctx->setContextProperty("_osName", osName);

    auto updater = new UpdateChecker(this);
    ctx->setContextProperty("_updates", updater);

    auto packageDelegate = new LauncherPackageDelegate(this);
    ctx->setContextProperty("_packages", packageDelegate);

    auto notifications = new LauncherNotificationsController{this, engine()};
    ctx->setContextProperty("_notifications", notifications);

    if (!inSimMode) {
        auto addOnsCtl = new AddOnsController(this, m_controller->config());
        ctx->setContextProperty("_addOns", addOnsCtl);
    }

    auto weatherScenariosModel = new flightgear::WeatherScenariosModel(this);
    ctx->setContextProperty("_weatherScenarios", weatherScenariosModel);

    setSource(QUrl("qrc:///qml/Launcher.qml"));
}

void LauncherMainWindow::onQuickStatusChanged(QQuickView::Status status)
{
    if (status == QQuickView::Error) {
        QString errorString;

        Q_FOREACH (auto err, errors()) {
            errorString.append("\n" + err.toString());
        }

        QMessageBox::critical(nullptr, "UI loading failures.",
                              tr("Problems occurred loading the user interface. This is usually due to missing modules on your system. "
                                 "Please report this error to the FlightGear developer list or forum, and take care to mention your system "
                                 "distribution, etc. Please also include the information provided below.\n") +
                                  errorString);
    }
}

bool LauncherMainWindow::checkQQC2Availability()
{
    QQmlComponent comp(engine());
    comp.setData(R"(
               import QtQuick.Controls 2.0
               ScrollBar {
               }
               )",
                 {});
    if (comp.isError()) {
        return false;
    }


    auto item = comp.create();
    const bool haveQQC2 = (item != nullptr);
    if (item)
        item->deleteLater();
    return haveQQC2;
}

LauncherMainWindow::~LauncherMainWindow()
{
}

bool LauncherMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::Close) {
        m_controller->saveSettings();
    }
    return QQuickView::event(event);
}

bool LauncherMainWindow::execInApp()
{
	m_controller->setInAppMode();
    
    show();

    while (m_controller->keepRunningInAppMode()) {
        qApp->processEvents();
    }

    return m_controller->inAppResult();
}

// this slot runs in the context of the render thread. Don't modify
void LauncherMainWindow::renderTheadSceneGraphInitialized()
{
    auto qContext = QOpenGLContext::currentContext();
    if (!qContext) {
        qWarning() << Q_FUNC_INFO << "No current OpenGL context";
        return;
    }

    std::string renderer = (char*)glGetString(GL_RENDERER);
    // capture this to help with debugging this crash:
    // https://sentry.io/share/issue/f98e38dceb4241dbaeed944d6ce4d746/
    // https://bugreports.qt.io/browse/QTBUG-69703
    flightgear::addSentryTag("qt-gl-vendor", (char*)glGetString(GL_VENDOR));
    flightgear::addSentryTag("qt-gl-renderer", renderer.c_str());
    flightgear::addSentryTag("qt-gl-version", (char*)glGetString(GL_VERSION));
    flightgear::addSentryTag("qt-glsl-version", (char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    const char* gltype[] = {"Desktop", "GLES 2", "GLES 1"};
    flightgear::addSentryTag("qt-gl-module-type", gltype[QOpenGLContext::openGLModuleType()]);

    // if necessary, borrow more code from:
    // https://code.qt.io/cgit/qt/qtbase.git/tree/examples/opengl/contextinfo/widget.cpp?h=5.15#n358
    // to expand what this reports
}
