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

    // allow selecting different QML files based on the Qt version we are
    // compiled against
    auto selector = new QQmlFileSelector(engine(), this);
#if QT_VERSION >= 0x050600
    selector->setExtraSelectors({"qt56"});
#endif

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

