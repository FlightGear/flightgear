#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QMenu>

#include <QPushButton>
#include <QStandardItemModel>
#include <QDesktopServices>

#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>

// simgear headers
#include <simgear/package/Install.hxx>
#include <simgear/environment/metar.hxx>
#include <simgear/structure/exception.hxx>

// FlightGear headers
#include <Network/HTTPClient.hxx>
#include <Main/globals.hxx>
#include <Airports/airport.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include "version.h"

// launcher headers
#include "QtLauncher.hxx"
#include "PathsDialog.hxx"

#include "ViewCommandLinePage.hxx"
#include "AircraftModel.hxx"
#include "LocalAircraftCache.hxx"
#include "LauncherController.hxx"
#include "DefaultAircraftLocator.hxx"

#include "ui_Launcher.h"


extern void restartTheApp(QStringList fgArgs);

//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow() :
    QMainWindow(),
    m_subsystemIdleTimer(NULL)
{
    m_ui.reset(new Ui::Launcher);
    m_ui->setupUi(this);

    QMenuBar* mb = menuBar();

#if !defined(Q_OS_MAC)
    QMenu* fileMenu = mb->addMenu(tr("File"));
    QAction* quitAction = fileMenu->addAction(tr("Exit"));
    connect(quitAction, &QAction::triggered,
            this, &LauncherMainWindow::onQuit);

#endif

    m_controller = new LauncherController(this, m_ui->location);
    m_controller->initQML();

    connect(m_controller, &LauncherController::canFlyChanged,
            this, &LauncherMainWindow::onCanFlyChanged);

    m_ui->location->setLaunchConfig(m_controller->config());

    QMenu* toolsMenu = mb->addMenu(tr("Tools"));
    QAction* restoreDefaultsAction = toolsMenu->addAction(tr("Restore defaults..."));
    connect(restoreDefaultsAction, &QAction::triggered,
            this, &LauncherMainWindow::onRestoreDefaults);

    QAction* changeDataAction = toolsMenu->addAction(tr("Select data files location..."));
    connect(changeDataAction, &QAction::triggered,
            this, &LauncherMainWindow::onChangeDataDir);

    QAction* viewCommandLineAction = toolsMenu->addAction(tr("View command-line"));
    connect(viewCommandLineAction, &QAction::triggered,
            this, &LauncherMainWindow::onViewCommandLine);


    m_subsystemIdleTimer = new QTimer(this);
    m_subsystemIdleTimer->setInterval(0);
    connect(m_subsystemIdleTimer, &QTimer::timeout,
            this, &LauncherMainWindow::onSubsytemIdleTimeout);
    m_subsystemIdleTimer->start();

    connect(m_ui->flyButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->summaryButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->aircraftButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->locationButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->environmentButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->settingsButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->addOnsButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &LauncherMainWindow::onQuit);
    addAction(qa);


    AddOnsPage* addOnsPage = new AddOnsPage(NULL, globals->packageRoot());
    connect(addOnsPage, &AddOnsPage::sceneryPathsChanged,
            m_controller, &LauncherController::setSceneryPaths);
    connect(addOnsPage, &AddOnsPage::aircraftPathsChanged,
            m_controller, &LauncherController::onAircraftPathsChanged);
    m_ui->stack->addWidget(addOnsPage);

    connect(m_controller->baseAircraftModel(), &AircraftItemModel::catalogsRefreshed,
             addOnsPage, &AddOnsPage::onCatalogsRefreshed);


    m_viewCommandLinePage = new ViewCommandLinePage;
    m_viewCommandLinePage->setLaunchConfig(m_controller->config());
    m_ui->stack->addWidget(m_viewCommandLinePage);

    QSettings settings;
    restoreGeometry(settings.value("window-geometry").toByteArray());

    m_controller->restoreSettings();
    restoreSettings();

    ////////////
#if defined(Q_OS_WIN)
    const QString osName("win");
#elif defined(Q_OS_MAC)
    const QString osName("mac");
#else
    const QString osName("unix");
#endif

    /////////////
    // aircraft
    m_ui->aircraftList->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_ui->aircraftList->engine()->addImportPath("qrc:///");
    m_ui->aircraftList->engine()->rootContext()->setContextProperty("_launcher", m_controller);

    connect( m_ui->aircraftList, &QQuickWidget::statusChanged,
             this, &LauncherMainWindow::onQuickStatusChanged);
    m_ui->aircraftList->setSource(QUrl("qrc:///qml/AircraftList.qml"));

    // settings
    m_ui->settings->engine()->addImportPath("qrc:///");
    QQmlContext* settingsContext = m_ui->settings->engine()->rootContext();
    settingsContext->setContextProperty("_launcher", m_controller);
    settingsContext->setContextProperty("_osName", osName);
    settingsContext->setContextProperty("_config", m_controller->config());

    m_ui->settings->setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect( m_ui->settings, &QQuickWidget::statusChanged,
             this, &LauncherMainWindow::onQuickStatusChanged);
    m_ui->settings->setSource(QUrl("qrc:///qml/Settings.qml"));

    // environemnt
    m_ui->environmentPage->engine()->addImportPath("qrc:///");
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_launcher", m_controller);
    auto weatherScenariosModel = new flightgear::WeatherScenariosModel(this);
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_weatherScenarios", weatherScenariosModel);
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_config", m_controller->config());

    m_ui->environmentPage->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect( m_ui->environmentPage, &QQuickWidget::statusChanged,
             this, &LauncherMainWindow::onQuickStatusChanged);
    m_ui->environmentPage->setSource(QUrl("qrc:///qml/Environment.qml"));

    // summary
    m_ui->summary->engine()->addImportPath("qrc:///");
    m_ui->summary->engine()->rootContext()->setContextProperty("_launcher", m_controller);
    m_ui->summary->engine()->rootContext()->setContextProperty("_config", m_controller->config());

    m_ui->summary->setResizeMode(QQuickWidget::SizeRootObjectToView);

    connect( m_ui->summary, &QQuickWidget::statusChanged,
             this, &LauncherMainWindow::onQuickStatusChanged);
    m_ui->summary->setSource(QUrl("qrc:///qml/Summary.qml"));
    //////////////////////////

}

void LauncherMainWindow::restoreSettings()
{
    if (!m_inAppMode) {
        m_controller->setSceneryPaths();
    }
}

void LauncherMainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("window-geometry", saveGeometry());
}

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

void LauncherMainWindow::onCanFlyChanged()
{
    m_ui->flyButton->setEnabled(m_controller->canFly());
}

LauncherMainWindow::~LauncherMainWindow()
{
}

bool LauncherMainWindow::execInApp()
{
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
}



void LauncherMainWindow::closeEvent(QCloseEvent *event)
{
    qApp->exit(-1);
}


void LauncherMainWindow::onRun()
{
    m_controller->doRun();
    saveSettings();
    qApp->exit(1);
}

void LauncherMainWindow::onApply()
{
    m_controller->doApply();
    saveSettings();
    m_accepted = true;
    m_runInApp = false;
}

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
    QMessageBox mbox(this);
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

void LauncherMainWindow::onViewCommandLine()
{
    m_ui->stack->setCurrentIndex(6);
    Q_FOREACH (ToolboxButton* tb, findChildren<ToolboxButton*>()) {
        tb->setChecked(false);
    }
    m_viewCommandLinePage->update();
}


void LauncherMainWindow::onClickToolboxButton()
{
    int pageIndex = sender()->property("pageIndex").toInt();
    m_ui->stack->setCurrentIndex(pageIndex);
    Q_FOREACH (ToolboxButton* tb, findChildren<ToolboxButton*>()) {
        tb->setChecked(tb->property("pageIndex").toInt() == pageIndex);
    }
    saveSettings();
}

void LauncherMainWindow::onSubsytemIdleTimeout()
{
    globals->get_subsystem_mgr()->update(0.0);
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

    QMessageBox mbox(this);
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



