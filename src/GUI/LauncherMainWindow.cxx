#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>

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


extern void restartTheApp(QStringList fgArgs);

//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow() :
    QQuickView()
{
    setTitle("FlightGear " FLIGHTGEAR_VERSION);

    m_controller = new LauncherController(this, this);
    m_controller->initQML();

#if defined(Q_OS_MAC)
   QMenuBar* mb = new QMenuBar();

   QMenu* toolsMenu = mb->addMenu(tr("Tools"));
   QAction* restoreDefaultsAction = toolsMenu->addAction(tr("Restore defaults..."));
   connect(restoreDefaultsAction, &QAction::triggered,
           this, &LauncherMainWindow::onRestoreDefaults);

   QAction* changeDataAction = toolsMenu->addAction(tr("Select data files location..."));
   connect(changeDataAction, &QAction::triggered,
           this, &LauncherMainWindow::onChangeDataDir);

   QAction* viewCommandLineAction = toolsMenu->addAction(tr("View command-line"));
   connect(viewCommandLineAction, &QAction::triggered,
           m_controller, &LauncherController::viewCommandLine);
#endif

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &LauncherMainWindow::onQuit);

    m_controller->restoreSettings();
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
#if 0
    m_inAppMode = true;
    m_ui->addOnsButton->hide();
    m_ui->settingsButton->hide();
    disconnect(m_ui->flyButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->flyButton, SIGNAL(clicked()), this, SLOT(onApply()));
    m_runInApp = true;

    show();

    while (m_runInApp) {
        qApp->processEvents();
    }

    return m_accepted;
#endif
}

#if 0

void LauncherMainWindow::onApply()
{
    m_controller->doApply();
    saveSettings();
    m_accepted = true;
    m_runInApp = false;
}
#endif

void LauncherMainWindow::onQuit()
{
    if (m_inAppMode) {
        m_runInApp = false;
    } else {
        qApp->exit(-1);
    }
}

void LauncherMainWindow::onRestoreDefaults()
{
    QMessageBox mbox;
    mbox.setText(tr("Restore all settings to defaults?"));
    mbox.setInformativeText(tr("Restoring settings to their defaults may affect available add-ons such as scenery or aircraft."));
    QPushButton* quitButton = mbox.addButton(tr("Restore and restart now"), QMessageBox::YesRole);
    mbox.addButton(QMessageBox::Cancel);
    mbox.setDefaultButton(QMessageBox::Cancel);
    mbox.setIconPixmap(QPixmap(":/app-icon-large"));

    mbox.exec();
    if (mbox.clickedButton() != quitButton) {
        return;
    }

    {
        QSettings settings;
        settings.clear();
        settings.setValue("restore-defaults-on-run", true);
    }

    flightgear::restartTheApp();
}


void LauncherMainWindow::onChangeDataDir()
{
    QString currentLocText;
    QSettings settings;
    QString root = settings.value("fg-root").toString();
    if (root.isNull()) {
        currentLocText = tr("Currently the built-in data files are being used");
    } else {
        currentLocText = tr("Currently using location: %1").arg(root);
    }

    QMessageBox mbox;
    mbox.setText(tr("Change the data files used by FlightGear?"));
    mbox.setInformativeText(tr("FlightGear requires additional files to operate. "
                               "(Also called the base package, or fg-data) "
                               "You can restart FlightGear and choose a "
                               "different data files location, or restore the default setting. %1").arg(currentLocText));
    QPushButton* quitButton = mbox.addButton(tr("Restart FlightGear now"), QMessageBox::YesRole);
    mbox.addButton(QMessageBox::Cancel);
    mbox.setDefaultButton(QMessageBox::Cancel);
    mbox.setIconPixmap(QPixmap(":/app-icon-large"));

    mbox.exec();
    if (mbox.clickedButton() != quitButton) {
        return;
    }

    {
        QSettings settings;
        // set the option to the magic marker value
        settings.setValue("fg-root", "!ask");
    } // scope the ensure settings are written nicely

    flightgear::restartTheApp();
}

