#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDesktopServices>

#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

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
#include "AircraftModel.hxx"
#include "PathsDialog.hxx"
#include "AircraftSearchFilterModel.hxx"
#include "DefaultAircraftLocator.hxx"
#include "SettingsWidgets.hxx"
#include "LaunchConfig.hxx"
#include "SettingsSectionQML.hxx"
#include "ExtraSettingsSection.hxx"
#include "ViewCommandLinePage.hxx"
#include "MPServersModel.h"
#include "ThumbnailImageItem.hxx"
#include "PreviewImageItem.hxx"
#include "FlickableExtentQuery.hxx"
#include "LocalAircraftCache.hxx"
#include "QmlAircraftInfo.hxx"
#include "LauncherArgumentTokenizer.hxx"
#include "PathUrlHelper.hxx"
#include "PopupWindowTracker.hxx"
#include "RecentAircraftModel.hxx"
#include "RecentLocationsModel.hxx"

#include "ui_Launcher.h"

using namespace simgear::pkg;

extern void restartTheApp(QStringList fgArgs);

QQmlPrivate::AutoParentResult launcher_autoParent(QObject* thing, QObject* parent)
{
    SettingsSection* ss = qobject_cast<SettingsSection*>(parent);
    SettingsControl* sc = qobject_cast<SettingsControl*>(thing);
    if (ss && sc) {
        sc->setParent(ss);
        return QQmlPrivate::Parented;
    }

    qWarning() << "Unsuitable" << thing  << parent;
    return QQmlPrivate::IncompatibleObject;
}

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

    m_serversModel = new MPServersModel(this);

    m_locationHistory = new RecentLocationsModel(this);

    m_selectedAircraftInfo = new QmlAircraftInfo(this);
    initQML();

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

    connect(m_ui->location, &LocationWidget::descriptionChanged,
            this, &LauncherMainWindow::summaryChanged);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &LauncherMainWindow::onQuit);
    addAction(qa);

    m_aircraftModel = new AircraftItemModel(this);
    m_installedAircraftModel = new AircraftProxyModel(this, m_aircraftModel);
    m_installedAircraftModel->setInstalledFilterEnabled(true);

    m_browseAircraftModel = new AircraftProxyModel(this, m_aircraftModel);
    m_browseAircraftModel->setRatingFilterEnabled(true);

    m_aircraftSearchModel = new AircraftProxyModel(this, m_aircraftModel);

    m_aircraftHistory = new RecentAircraftModel(m_aircraftModel, this);

    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallCompleted,
            this, &LauncherMainWindow::onAircraftInstalledCompleted);
    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallFailed,
            this, &LauncherMainWindow::onAircraftInstallFailed);

    connect(LocalAircraftCache::instance(),
            &LocalAircraftCache::scanCompleted,
            this, &LauncherMainWindow::updateSelectedAircraft);

    AddOnsPage* addOnsPage = new AddOnsPage(NULL, globals->packageRoot());
    connect(addOnsPage, &AddOnsPage::sceneryPathsChanged,
            this, &LauncherMainWindow::setSceneryPaths);
    connect(addOnsPage, &AddOnsPage::aircraftPathsChanged,
            this, &LauncherMainWindow::onAircraftPathsChanged);
    m_ui->stack->addWidget(addOnsPage);

    connect (m_aircraftModel, &AircraftItemModel::catalogsRefreshed,
             addOnsPage, &AddOnsPage::onCatalogsRefreshed);

    QSettings settings;
    LocalAircraftCache::instance()->setPaths(settings.value("aircraft-paths").toStringList());
    LocalAircraftCache::instance()->scanDirs();
    m_aircraftModel->setPackageRoot(globals->packageRoot());

    m_viewCommandLinePage = new ViewCommandLinePage;
    m_viewCommandLinePage->setLaunchConfig(m_config);
    m_ui->stack->addWidget(m_viewCommandLinePage);

    restoreSettings();

    emit summaryChanged();
    emit showNoOfficialHangarChanged();

    /////////////
    // aircraft
    m_ui->aircraftList->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_ui->aircraftList->engine()->addImportPath("qrc:///");
    m_ui->aircraftList->engine()->rootContext()->setContextProperty("_launcher", this);
    m_ui->aircraftList->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);

    m_ui->aircraftList->setSource(QUrl("qrc:///qml/AircraftList.qml"));

    // settings
    m_ui->settings->engine()->addImportPath("qrc:///");
    m_ui->settings->engine()->rootContext()->setContextProperty("_launcher", this);
    m_ui->settings->engine()->rootContext()->setContextProperty("_mpServers", m_serversModel);

    m_ui->settings->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);

    m_ui->settings->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_ui->settings->setSource(QUrl("qrc:///qml/Settings.qml"));

    // environemnt
    m_ui->environmentPage->engine()->addImportPath("qrc:///");
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_launcher", this);
    auto weatherScenariosModel = new flightgear::WeatherScenariosModel(this);
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_weatherScenarios", weatherScenariosModel);

    m_ui->environmentPage->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);
    m_ui->environmentPage->engine()->rootContext()->setContextProperty("_config", m_config);

    m_ui->environmentPage->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_ui->environmentPage->setSource(QUrl("qrc:///qml/Environment.qml"));

    // summary
    m_ui->summary->engine()->addImportPath("qrc:///");
    m_ui->summary->engine()->rootContext()->setContextProperty("_launcher", this);
    m_ui->summary->engine()->setObjectOwnership(this, QQmlEngine::CppOwnership);
    m_ui->summary->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_ui->summary->setSource(QUrl("qrc:///qml/Summary.qml"));
    //////////////////////////

}

void LauncherMainWindow::initQML()
{
    QQmlPrivate::RegisterAutoParent autoparent = { 0, &launcher_autoParent };
    QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);

    qmlRegisterType<LauncherArgumentTokenizer>("FlightGear.Launcher", 1, 0, "ArgumentTokenizer");

    qmlRegisterType<SettingsSectionQML>("FlightGear.Launcher", 1, 0, "Section");
    qmlRegisterType<SettingsCheckbox>("FlightGear.Launcher", 1, 0, "Checkbox");
    qmlRegisterType<SettingsComboBox>("FlightGear.Launcher", 1, 0, "Combo");
    qmlRegisterType<SettingsIntSpinbox>("FlightGear.Launcher", 1, 0, "Spinbox");
    qmlRegisterType<SettingsText>("FlightGear.Launcher", 1, 0, "LineEdit");
    qmlRegisterType<SettingsDateTime>("FlightGear.Launcher", 1, 0, "DateTime");
    qmlRegisterType<SettingsPath>("FlightGear.Launcher", 1, 0, "PathChooser");
    qmlRegisterUncreatableType<QAbstractItemModel>("FlightGear.Launcher", 1, 0, "QAIM", "no");
    qmlRegisterUncreatableType<AircraftProxyModel>("FlightGear.Launcher", 1, 0, "AircraftProxyModel", "no");

    qmlRegisterUncreatableType<RecentAircraftModel>("FlightGear.Launcher", 1, 0, "RecentAircraftModel", "no");
    qmlRegisterUncreatableType<RecentLocationsModel>("FlightGear.Launcher", 1, 0, "RecentLocationsModel", "no");

    qmlRegisterUncreatableType<SettingsControl>("FlightGear.Launcher", 1, 0, "Control", "Base class");
    qmlRegisterUncreatableType<LaunchConfig>("FlightGear.Launcher", 1, 0, "LaunchConfig", "Singleton API");
    qmlRegisterType<FileDialogWrapper>("FlightGear.Launcher", 1, 0, "FileDialog");

    qmlRegisterType<FlickableExtentQuery>("FlightGear.Launcher", 1, 0, "FlickableExtentQuery");
    qmlRegisterType<QmlAircraftInfo>("FlightGear.Launcher", 1, 0, "AircraftInfo");
    qmlRegisterType<PopupWindowTracker>("FlightGear.Launcher", 1, 0, "PopupWindowTracker");

    m_config = new LaunchConfig(this);
    connect(m_config, &LaunchConfig::collect, this, &LauncherMainWindow::collectAircraftArgs);
    m_ui->location->setLaunchConfig(m_config);

#if defined(Q_OS_WIN)
    const QString osName("win");
#elif defined(Q_OS_MAC)
    const QString osName("mac");
#else
    const QString osName("unix");
#endif

    QQmlContext* settingsContext = m_ui->settings->engine()->rootContext();
    settingsContext->setContextProperty("_mpServers", m_serversModel);
    settingsContext->setContextProperty("_config", m_config);
    settingsContext->setContextProperty("_osName", osName);

    QQmlContext* summaryContext = m_ui->summary->engine()->rootContext();
    summaryContext->setContextProperty("_config", m_config);

    qmlRegisterUncreatableType<LocalAircraftCache>("FlightGear.Launcher", 1, 0, "LocalAircraftCache", "Aircraft cache");
    qmlRegisterUncreatableType<AircraftItemModel>("FlightGear.Launcher", 1, 0, "AircraftModel", "Built-in model");
    qmlRegisterType<ThumbnailImageItem>("FlightGear.Launcher", 1, 0, "ThumbnailImage");
    qmlRegisterType<PreviewImageItem>("FlightGear.Launcher", 1, 0, "PreviewImage");

    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    SGPath cachePath = globals->get_fg_home() / "PreviewsCache";
    diskCache->setCacheDirectory(QString::fromStdString(cachePath.utf8Str()));

    QNetworkAccessManager* netAccess = new QNetworkAccessManager(this);
    netAccess->setCache(diskCache);
    PreviewImageItem::setGlobalNetworkAccess(netAccess);

}

LauncherMainWindow::~LauncherMainWindow()
{
    // avoid a double-free when the QQuickWidget's engine seems to try
    // and delete us.
   // delete m_ui->aircraftList;
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

void LauncherMainWindow::restoreSettings()
{
    QSettings settings;

    restoreGeometry(settings.value("window-geometry").toByteArray());

    m_selectedAircraft = m_aircraftHistory->mostRecent();
    if (m_selectedAircraft.isEmpty()) {
        // select the default aircraft specified in defaults.xml
        flightgear::DefaultAircraftLocator da;
        if (da.foundPath().exists()) {
            m_selectedAircraft = QUrl::fromLocalFile(QString::fromStdString(da.foundPath().utf8Str()));
            qDebug() << "Restored default aircraft:" << m_selectedAircraft;
        } else {
            qWarning() << "Completely failed to find the default aircraft";
        }
    }

    if (!m_inAppMode) {
        setSceneryPaths();
    }

    m_ui->location->restoreSettings();
    QVariantMap currentLocation = m_locationHistory->mostRecent();
    if (currentLocation.isEmpty()) {
        // use the default
        std::string defaultAirport = flightgear::defaultAirportICAO();
        FGAirportRef apt = FGAirport::findByIdent(defaultAirport);
        if (apt) {
            currentLocation["location-id"] = static_cast<qlonglong>(apt->guid());
            currentLocation["location-apt-runway"] = "active";
        } // otherwise we failed to find the default airport in the nav-db :(
    }
    m_ui->location->restoreLocation(currentLocation);

    updateSelectedAircraft();

    Q_FOREACH(SettingsSection* ss, findChildren<SettingsSection*>()) {
        ss->restoreState(settings);
    }

    m_serversModel->requestRestore();
 }

void LauncherMainWindow::saveSettings()
{
    emit requestSaveState();

    m_aircraftHistory->saveToSettings();
    m_locationHistory->saveToSettings();

    QSettings settings;
    settings.setValue("window-geometry", saveGeometry());

    Q_FOREACH(SettingsSection* ss, findChildren<SettingsSection*>()) {
        ss->saveState(settings);
    }
}

void LauncherMainWindow::closeEvent(QCloseEvent *event)
{
    qApp->exit(-1);
}

void LauncherMainWindow::collectAircraftArgs()
{
    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
        if (m_selectedAircraft.isLocalFile()) {
            QFileInfo setFileInfo(m_selectedAircraft.toLocalFile());
            m_config->setArg("aircraft-dir", setFileInfo.dir().absolutePath());
            QString setFile = setFileInfo.fileName();
            Q_ASSERT(setFile.endsWith("-set.xml"));
            setFile.truncate(setFile.count() - 8); // drop the '-set.xml' portion
            m_config->setArg("aircraft", setFile);
        } else if (m_selectedAircraft.scheme() == "package") {
            // no need to set aircraft-dir, handled by the corresponding code
            // in fgInitAircraft
            m_config->setArg("aircraft", m_selectedAircraft.path());
        } else {
            qWarning() << "unsupported aircraft launch URL" << m_selectedAircraft;
        }
    }

    // scenery paths
    QSettings settings;
    Q_FOREACH(QString path, settings.value("scenery-paths").toStringList()) {
        m_config->setArg("fg-scenery", path);
    }

    // aircraft paths
    Q_FOREACH(QString path, settings.value("aircraft-paths").toStringList()) {
        m_config->setArg("fg-aircraft", path);
    }
}

void LauncherMainWindow::onRun()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
    m_config->reset();
    m_config->collect();

    m_aircraftHistory->insert(m_selectedAircraft);

    QVariant locSet = m_ui->location->saveLocation();
    m_locationHistory->insert(locSet);

    // aircraft paths
    QSettings settings;
    QString downloadDir = settings.value("downloadSettings/downloadDir").toString();
    if (!downloadDir.isEmpty()) {
        QDir d(downloadDir);
        if (!d.exists()) {
            int result = QMessageBox::question(this, tr("Create download folder?"),
                                  tr("The selected location for downloads does not exist. (%1) Create it?").arg(downloadDir),
                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (result == QMessageBox::Cancel) {
                return;
            }

            if (result == QMessageBox::Yes) {
                d.mkpath(downloadDir);
            }
        }
    }

    if (settings.contains("restore-defaults-on-run")) {
        settings.remove("restore-defaults-on-run");
        opt->addOption("restore-defaults", "");
    }

    m_config->applyToOptions();
    saveSettings();

    // set a positive value here so we can detect this case in runLauncherDialog
    qApp->exit(1);
}

QString LauncherMainWindow::selectAircraftStateAutomatically()
{
    if (m_ui->location->isAirborneLocation()) {
        return "approach";
    }

    if (m_ui->location->isParkedLocation()) {
        return "parked";
    } else {
        return "take-off";
    }

    return {}; // failed to compute, give up
}

void LauncherMainWindow::onApply()
{
    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
        std::string aircraftPropValue,
            aircraftDir;

        if (m_selectedAircraft.isLocalFile()) {
            QFileInfo setFileInfo(m_selectedAircraft.toLocalFile());
            QString setFile = setFileInfo.fileName();
            Q_ASSERT(setFile.endsWith("-set.xml"));
            setFile.truncate(setFile.count() - 8); // drop the '-set.xml' portion
            aircraftDir = setFileInfo.dir().absolutePath().toStdString();
            aircraftPropValue = setFile.toStdString();
        } else if (m_selectedAircraft.scheme() == "package") {
            // no need to set aircraft-dir, handled by the corresponding code
            // in fgInitAircraft
            aircraftPropValue = m_selectedAircraft.path().toStdString();
        } else {
            qWarning() << "unsupported aircraft launch URL" << m_selectedAircraft;
        }

        m_aircraftHistory->insert(m_selectedAircraft);
        globals->get_props()->setStringValue("/sim/aircraft", aircraftPropValue);
        globals->get_props()->setStringValue("/sim/aircraft-dir", aircraftDir);
    }

    // location
    m_ui->location->setLocationProperties();

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

void LauncherMainWindow::onAircraftInstalledCompleted(QModelIndex index)
{
    maybeUpdateSelectedAircraft(index);
}

void LauncherMainWindow::onAircraftInstallFailed(QModelIndex index, QString errorMessage)
{
    qWarning() << Q_FUNC_INFO << index.data(AircraftURIRole) << errorMessage;

    QMessageBox msg;
    msg.setWindowTitle(tr("Aircraft installation failed"));
    msg.setText(tr("An error occurred installing the aircraft %1: %2").
                arg(index.data(Qt::DisplayRole).toString()).arg(errorMessage));
    msg.addButton(QMessageBox::Ok);
    msg.exec();

    maybeUpdateSelectedAircraft(index);
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

void LauncherMainWindow::maybeUpdateSelectedAircraft(QModelIndex index)
{
    QUrl u = index.data(AircraftURIRole).toUrl();
    if (u == m_selectedAircraft) {
        // potentially enable the run button now!
        updateSelectedAircraft();
    }
}

void LauncherMainWindow::updateSelectedAircraft()
{
    m_selectedAircraftInfo->setUri(m_selectedAircraft);
    QModelIndex index = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    if (index.isValid()) {
#if 0
        QPixmap pm = index.data(Qt::DecorationRole).value<QPixmap>();
        m_ui->thumbnail->setPixmap(pm);

        // important to use this version to get the variant-specific name
        // correct; the QModelIndex lookup doesn't do that.
        // actually all the data is for the primary variant, but this will be
        // fixed when enabling the LocalAircraftCache outside the AircaftModel
        m_ui->aircraftName->setText(m_aircraftModel->nameForAircraftURI(m_selectedAircraft));

        QVariant longDesc = index.data(AircraftLongDescriptionRole);
        m_ui->aircraftDescription->setVisible(!longDesc.isNull());
        m_ui->aircraftDescription->setText(longDesc.toString());
#endif
        int status = index.data(AircraftPackageStatusRole).toInt();
        bool canRun = (status == LocalAircraftCache::PackageInstalled);
        m_ui->flyButton->setEnabled(canRun);

        LauncherAircraftType aircraftType = Airplane;
        if (index.data(AircraftIsHelicopterRole).toBool()) {
            aircraftType = Helicopter;
        } else if (index.data(AircraftIsSeaplaneRole).toBool()) {
            aircraftType = Seaplane;
        }

        m_ui->location->setAircraftType(aircraftType);

        const bool hasStates = m_selectedAircraftInfo->hasStates();
#if 0
        m_ui->stateCombo->setVisible(hasStates);
        m_ui->stateLabel->setVisible(hasStates);
        m_ui->stateDescription->setVisible(false);
        if (hasStates) {
            m_ui->stateCombo->setModel(m_selectedAircraftInfo->statesModel());
            m_ui->stateDescription->setText(m_ui->stateCombo->currentData(QmlAircraftInfo::StateDescriptionRole).toString());
            // hiden when no description is present
            m_ui->stateDescription->setVisible(!m_ui->stateDescription->text().isEmpty());
        }
#endif
    } else {
#if 0
        m_ui->thumbnail->setPixmap(QPixmap());
        m_ui->aircraftName->setText("");
        m_ui->aircraftDescription->hide();
        m_ui->stateCombo->hide();
        m_ui->stateLabel->hide();
        m_ui->stateDescription->hide();
#endif
        m_ui->flyButton->setEnabled(false);
    }
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

void LauncherMainWindow::setSceneryPaths()
{
    flightgear::launcherSetSceneryPaths();
}

void LauncherMainWindow::onSubsytemIdleTimeout()
{
    globals->get_subsystem_mgr()->update(0.0);
}

void LauncherMainWindow::downloadDirChanged(QString path)
{
    // this can get run if the UI is disabled, just bail out before doing
    // anything permanent.
    if (!m_config->enableDownloadDirUI()) {
        return;
    }

    // if the default dir is passed in, map that back to the emptru string
    if (path == m_config->defaultDownloadDir()) {
        path.clear();;
    }

    auto options = flightgear::Options::sharedInstance();
    if (options->valueForOption("download-dir") == path.toStdString()) {
        // this works because we propogate the value from QSettings to
        // the flightgear::Options object in runLauncherDialog()
        // so the options object always contains our current idea of this
        // value
        return;
    }

    if (!path.isEmpty()) {
        options->setOption("download-dir", path.toStdString());
    } else {
        options->clearOption("download-dir");
    }

    // replace existing package root
    globals->get_subsystem<FGHTTPClient>()->shutdown();
    globals->setPackageRoot(simgear::pkg::RootRef());

    // create new root with updated download-dir value
    fgInitPackageRoot();

    globals->get_subsystem<FGHTTPClient>()->init();

    QSettings settings;
    // re-scan the aircraft list
    m_aircraftModel->setPackageRoot(globals->packageRoot());

    auto aircraftCache = LocalAircraftCache::instance();
    aircraftCache->setPaths(settings.value("aircraft-paths").toStringList());
    aircraftCache->scanDirs();

    emit showNoOfficialHangarChanged();

    // re-set scenery dirs
    setSceneryPaths();
}

bool LauncherMainWindow::shouldShowOfficialCatalogMessage() const
{
    QSettings settings;
    bool showOfficialCatalogMesssage = !globals->get_subsystem<FGHTTPClient>()->isDefaultCatalogInstalled();
    if (settings.value("hide-official-catalog-message").toBool()) {
        showOfficialCatalogMesssage = false;
    }
    return showOfficialCatalogMesssage;
}

void LauncherMainWindow::officialCatalogAction(QString s)
{
    if (s == "hide") {
        QSettings settings;
        settings.setValue("hide-official-catalog-message", true);
    } else if (s == "add-official") {
        AddOnsPage::addDefaultCatalog(this, false /* not silent */);
    }

    emit showNoOfficialHangarChanged();
}

QUrl LauncherMainWindow::selectedAircraft() const
{
    return m_selectedAircraft;
}

QPointF LauncherMainWindow::mapToGlobal(QQuickItem *item, const QPointF &pos) const
{
    QPointF scenePos = item->mapToScene(pos);
    return m_ui->aircraftList->mapToGlobal(scenePos.toPoint());
}

QmlAircraftInfo *LauncherMainWindow::selectedAircraftInfo() const
{
    return m_selectedAircraftInfo;
}

void LauncherMainWindow::restoreLocation(QVariant var)
{
    m_ui->location->restoreLocation(var.toMap());
}

bool LauncherMainWindow::matchesSearch(QString term, QStringList keywords) const
{
    Q_FOREACH(QString s, keywords) {
        if (s.contains(term, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

bool LauncherMainWindow::isSearchActive() const
{
    return !m_settingsSearchTerm.isEmpty();
}

QStringList LauncherMainWindow::settingsSummary() const
{
    return m_settingsSummary;
}

QStringList LauncherMainWindow::environmentSummary() const
{
    return m_environmentSummary;
}

void LauncherMainWindow::setSelectedAircraft(QUrl selectedAircraft)
{
    if (m_selectedAircraft == selectedAircraft)
        return;

    m_selectedAircraft = selectedAircraft;
    updateSelectedAircraft();
    emit selectedAircraftChanged(m_selectedAircraft);
}

void LauncherMainWindow::setSettingsSearchTerm(QString settingsSearchTerm)
{
    if (m_settingsSearchTerm == settingsSearchTerm)
        return;

    m_settingsSearchTerm = settingsSearchTerm;
    emit searchChanged();
}

void LauncherMainWindow::setSettingsSummary(QStringList settingsSummary)
{
    if (m_settingsSummary == settingsSummary)
        return;

    m_settingsSummary = settingsSummary;
    emit summaryChanged();
}

void LauncherMainWindow::setEnvironmentSummary(QStringList environmentSummary)
{
    if (m_environmentSummary == environmentSummary)
        return;

    m_environmentSummary = environmentSummary;
    emit summaryChanged();
}

QStringList LauncherMainWindow::combinedSummary() const
{
    return m_settingsSummary + m_environmentSummary;
}

QString LauncherMainWindow::locationDescription() const
{
    return m_ui->location->locationDescription();
}

simgear::pkg::PackageRef LauncherMainWindow::packageForAircraftURI(QUrl uri) const
{
    if (uri.scheme() != "package") {
        qWarning() << "invalid URL scheme:" << uri;
        return simgear::pkg::PackageRef();
    }

    QString ident = uri.path();
    return globals->packageRoot()->getPackageById(ident.toStdString());
}

void LauncherMainWindow::onAircraftPathsChanged()
{
    QSettings settings;
    auto aircraftCache = LocalAircraftCache::instance();
    aircraftCache->setPaths(settings.value("aircraft-paths").toStringList());
    aircraftCache->scanDirs();
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

void LauncherMainWindow::onSettingsSearchChanged()
{
#if 0
    Q_FOREACH(SettingsSectionQML* ss, findChildren<SettingsSectionQML*>()) {
        ss->setSearchTerm(m_ui->settingsSearchEdit->text());
    }
#endif
}

bool LauncherMainWindow::validateMetarString(QString metar)
{
    if (metar.isEmpty()) {
        return true;
    }

    try {
        std::string s = metar.toStdString();
        SGMetar theMetar(s);
    } catch (sg_io_exception&) {
        return false;
    }

    return true;
}

void LauncherMainWindow::requestInstallUpdate(QUrl aircraftUri)
{
    // also select, otherwise UI is confusing
    m_selectedAircraft = aircraftUri;
    updateSelectedAircraft();

    simgear::pkg::PackageRef pref = packageForAircraftURI(aircraftUri);
    if (pref) {
        if (pref->isInstalled()) {
            InstallRef install = pref->existingInstall();
            if (install->hasUpdate()) {
                globals->packageRoot()->scheduleToUpdate(install);
            }
        } else {
            pref->install();
        }
    }
}

void LauncherMainWindow::requestUninstall(QUrl aircraftUri)
{
    simgear::pkg::PackageRef pref = packageForAircraftURI(aircraftUri);
    if (pref) {
        simgear::pkg::InstallRef i = pref->existingInstall();
        if (i) {
            i->uninstall();
        }
    }
}

void LauncherMainWindow::requestInstallCancel(QUrl aircraftUri)
{
    simgear::pkg::PackageRef pref = packageForAircraftURI(aircraftUri);
    if (pref) {
        simgear::pkg::InstallRef i = pref->existingInstall();
        if (i) {
            i->cancelDownload();
        }
    }
}

void LauncherMainWindow::requestUpdateAllAircraft()
{
    const PackageList& toBeUpdated = globals->packageRoot()->packagesNeedingUpdate();
    std::for_each(toBeUpdated.begin(), toBeUpdated.end(), [](PackageRef pkg) {
        globals->packageRoot()->scheduleToUpdate(pkg->install());
    });
}

void LauncherMainWindow::queryMPServers()
{
    m_serversModel->refresh();
}

bool LauncherMainWindow::showNoOfficialHanger() const
{
    return shouldShowOfficialCatalogMessage();
}

QString LauncherMainWindow::versionString() const
{
    return FLIGHTGEAR_VERSION;
}

RecentAircraftModel *LauncherMainWindow::aircraftHistory()
{
    return m_aircraftHistory;
}

RecentLocationsModel *LauncherMainWindow::locationHistory()
{
    return m_locationHistory;
}

void LauncherMainWindow::launchUrl(QUrl url)
{
    QDesktopServices::openUrl(url);
}

QVariantList LauncherMainWindow::defaultSplashUrls() const
{
    QVariantList urls;

    for (auto path : flightgear::defaultSplashScreenPaths()) {
        QUrl url = QUrl::fromLocalFile(QString::fromStdString(path));
        urls.append(url);
    }

    return urls;
}
