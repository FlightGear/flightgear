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

#include "version.h"

// launcher headers
#include "QtLauncher.hxx"
#include "DefaultAircraftLocator.hxx"
#include "LaunchConfig.hxx"
#include "AircraftModel.hxx"
#include "LocalAircraftCache.hxx"
#include "LauncherController.hxx"
#include "AddOnsController.hxx"
#include "LocationController.hxx"


//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow() :
    QQuickView()
{
    setTitle("FlightGear " FLIGHTGEAR_VERSION);

    m_controller = new LauncherController(this, this);
    m_controller->initQML();

#if defined(Q_OS_MAC)
   QMenuBar* mb = new QMenuBar();

   QMenu* fileMenu = mb->addMenu(tr("File"));
   QAction* openAction = fileMenu->addAction(tr("Open saved configuration..."));
   connect(openAction, &QAction::triggered,
       m_controller, &LauncherController::openConfig);

   QAction* saveAction = fileMenu->addAction(tr("Save configuration as..."));
   connect(saveAction, &QAction::triggered,
       m_controller, &LauncherController::saveConfigAs);

   QMenu* toolsMenu = mb->addMenu(tr("Tools"));
   QAction* restoreDefaultsAction = toolsMenu->addAction(tr("Restore defaults..."));
   connect(restoreDefaultsAction, &QAction::triggered,
	   m_controller, &LauncherController::requestRestoreDefaults);

   QAction* changeDataAction = toolsMenu->addAction(tr("Select data files location..."));
   connect(changeDataAction, &QAction::triggered,
	   m_controller, &LauncherController::requestChangeDataPath);

   QAction* viewCommandLineAction = toolsMenu->addAction(tr("View command-line"));
   connect(viewCommandLineAction, &QAction::triggered,
           m_controller, &LauncherController::viewCommandLine);
#endif

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, m_controller, &LauncherController::quit);

    m_controller->initialRestoreSettings();
    flightgear::launcherSetSceneryPaths();

    auto addOnsCtl = new AddOnsController(this);

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
    ctx->setContextProperty("_addOns", addOnsCtl);
    ctx->setContextProperty("_config", m_controller->config());
    ctx->setContextProperty("_location", m_controller->location());
    ctx->setContextProperty("_osName", osName);

    auto weatherScenariosModel = new flightgear::WeatherScenariosModel(this);
    ctx->setContextProperty("_weatherScenarios", weatherScenariosModel);

    setSource(QUrl("qrc:///qml/Launcher.qml"));
}

#if 0
void LauncherMainWindow::onQuickStatusChanged(QQuickWidget::Status status)
{
    if (status == QQuickWidget::Error) {
        QQuickWidget* qw = qobject_cast<QQuickWidget*>(sender());
        QString errorString;

        Q_FOREACH(auto err, qw->errors()) {
            errorString.append("\n" + err.toString());
        }

        QMessageBox::critical(this, "UI loading failures.",
                              tr("Problems occurred loading the user interface. This is often due to missing modules on your system. "
                                 "Please report this error to the FlightGear developer list or forum, and take care to mention your system "
                                 "distribution, etc. Please also include the information provided below.\n")
                              + errorString);
    }
}
#endif

LauncherMainWindow::~LauncherMainWindow()
{
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

