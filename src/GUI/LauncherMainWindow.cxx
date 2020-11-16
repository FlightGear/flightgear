#include "config.h" 

#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>

#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlFileSelector>

// launcher headers
#include "AddOnsController.hxx"
#include "AircraftItemModel.hxx"
#include "DefaultAircraftLocator.hxx"
#include "LaunchConfig.hxx"
#include "LauncherController.hxx"
#include "LauncherNotificationsController.hxx"
#include "LauncherPackageDelegate.hxx"
#include "LocalAircraftCache.hxx"
#include "LocationController.hxx"
#include "QtLauncher.hxx"
#include "UpdateChecker.hxx"

//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow(bool inSimMode) : QQuickView()
{
    setTitle("FlightGear " FLIGHTGEAR_VERSION);

    m_controller = new LauncherController(this, this);
    m_controller->initQML();

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

