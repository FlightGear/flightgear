#include "LauncherController.hxx"

// Qt headers
#include <QDebug>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QDesktopServices>
#include <QMessageBox>
#include <QSettings>
#include <QQuickWindow>
#include <QQmlComponent>

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

#include "QtLauncher.hxx"
#include "QmlAircraftInfo.hxx"
#include "LauncherArgumentTokenizer.hxx"
#include "PathUrlHelper.hxx"
#include "PopupWindowTracker.hxx"
#include "RecentAircraftModel.hxx"
#include "RecentLocationsModel.hxx"
#include "ThumbnailImageItem.hxx"
#include "PreviewImageItem.hxx"
#include "MPServersModel.h"
#include "AircraftSearchFilterModel.hxx"
#include "DefaultAircraftLocator.hxx"
#include "LaunchConfig.hxx"
#include "AircraftModel.hxx"
#include "LocationController.hxx"
#include "QmlPositioned.hxx"
#include "PixmapImageItem.hxx"
#include "AirportDiagram.hxx"
#include "NavaidDiagram.hxx"
#include "QmlRadioButtonHelper.hxx"

using namespace simgear::pkg;


LauncherController::LauncherController(QObject *parent, QWindow* window) :
    QObject(parent),
    m_window(window)
{
    m_serversModel = new MPServersModel(this);
    m_location = new LocationController(this);
    m_locationHistory = new RecentLocationsModel(this);
    m_selectedAircraftInfo = new QmlAircraftInfo(this);

    m_config = new LaunchConfig(this);
    connect(m_config, &LaunchConfig::collect, this, &LauncherController::collectAircraftArgs);

    m_location->setLaunchConfig(m_config);
    connect(m_location, &LocationController::descriptionChanged,
            this, &LauncherController::summaryChanged);

    initQML();

    m_aircraftModel = new AircraftItemModel(this);
    m_installedAircraftModel = new AircraftProxyModel(this, m_aircraftModel);
    m_installedAircraftModel->setInstalledFilterEnabled(true);

    m_browseAircraftModel = new AircraftProxyModel(this, m_aircraftModel);
    m_browseAircraftModel->setRatingFilterEnabled(true);

    m_aircraftSearchModel = new AircraftProxyModel(this, m_aircraftModel);

    m_aircraftHistory = new RecentAircraftModel(m_aircraftModel, this);

    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallCompleted,
            this, &LauncherController::onAircraftInstalledCompleted);
    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallFailed,
            this, &LauncherController::onAircraftInstallFailed);

    connect(LocalAircraftCache::instance(),
            &LocalAircraftCache::scanCompleted,
            this, &LauncherController::updateSelectedAircraft);

    QSettings settings;
    LocalAircraftCache::instance()->setPaths(settings.value("aircraft-paths").toStringList());
    LocalAircraftCache::instance()->scanDirs();
    m_aircraftModel->setPackageRoot(globals->packageRoot());

    m_subsystemIdleTimer = new QTimer(this);
    m_subsystemIdleTimer->setInterval(0);
    connect(m_subsystemIdleTimer, &QTimer::timeout, []()
       {globals->get_subsystem_mgr()->update(0.0);});
    m_subsystemIdleTimer->start();

    QRect winRect= settings.value("window-geometry").toRect();
    if (winRect.isValid()) {
        m_window->setGeometry(winRect);
    } else {
        m_window->setWidth(600);
        m_window->setHeight(800);
    }
}

void LauncherController::initQML()
{
    qmlRegisterUncreatableType<LauncherController>("FlightGear.Launcher", 1, 0, "LauncherController", "no");
    qmlRegisterUncreatableType<LocationController>("FlightGear.Launcher", 1, 0, "LocationController", "no");

    qmlRegisterType<LauncherArgumentTokenizer>("FlightGear.Launcher", 1, 0, "ArgumentTokenizer");
    qmlRegisterUncreatableType<QAbstractItemModel>("FlightGear.Launcher", 1, 0, "QAIM", "no");
    qmlRegisterUncreatableType<AircraftProxyModel>("FlightGear.Launcher", 1, 0, "AircraftProxyModel", "no");
    qmlRegisterUncreatableType<RecentAircraftModel>("FlightGear.Launcher", 1, 0, "RecentAircraftModel", "no");
    qmlRegisterUncreatableType<RecentLocationsModel>("FlightGear.Launcher", 1, 0, "RecentLocationsModel", "no");
    qmlRegisterUncreatableType<LaunchConfig>("FlightGear.Launcher", 1, 0, "LaunchConfig", "Singleton API");
    qmlRegisterUncreatableType<MPServersModel>("FlightGear.Launcher", 1, 0, "MPServers", "Singleton API");

    qmlRegisterType<FileDialogWrapper>("FlightGear.Launcher", 1, 0, "FileDialog");
    qmlRegisterType<QmlAircraftInfo>("FlightGear.Launcher", 1, 0, "AircraftInfo");
    qmlRegisterType<PopupWindowTracker>("FlightGear.Launcher", 1, 0, "PopupWindowTracker");

    qmlRegisterUncreatableType<LocalAircraftCache>("FlightGear.Launcher", 1, 0, "LocalAircraftCache", "Aircraft cache");
    qmlRegisterUncreatableType<AircraftItemModel>("FlightGear.Launcher", 1, 0, "AircraftModel", "Built-in model");
    qmlRegisterType<ThumbnailImageItem>("FlightGear.Launcher", 1, 0, "ThumbnailImage");
    qmlRegisterType<PreviewImageItem>("FlightGear.Launcher", 1, 0, "PreviewImage");

    qmlRegisterType<QmlPositioned>("FlightGear", 1, 0, "Positioned");
    // this is a Q_GADGET, but we need to register it for use in return types, etc
    qRegisterMetaType<QmlGeod>();

    qmlRegisterType<PixmapImageItem>("FlightGear", 1, 0, "PixmapImage");
    qmlRegisterType<AirportDiagram>("FlightGear", 1, 0, "AirportDiagram");
    qmlRegisterType<NavaidDiagram>("FlightGear", 1, 0, "NavaidDiagram");
    qmlRegisterType<QmlRadioButtonGroup>("FlightGear", 1, 0, "RadioButtonGroup");

    QNetworkDiskCache* diskCache = new QNetworkDiskCache(this);
    SGPath cachePath = globals->get_fg_home() / "PreviewsCache";
    diskCache->setCacheDirectory(QString::fromStdString(cachePath.utf8Str()));

    QNetworkAccessManager* netAccess = new QNetworkAccessManager(this);
    netAccess->setCache(diskCache);
    PreviewImageItem::setGlobalNetworkAccess(netAccess);
}

void LauncherController::restoreSettings()
{
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


    m_location->restoreSettings();
    QVariantMap currentLocation = m_locationHistory->mostRecent();
    if (currentLocation.isEmpty()) {
        // use the default
        std::string defaultAirport = flightgear::defaultAirportICAO();
        FGAirportRef apt = FGAirport::findByIdent(defaultAirport);
        if (apt) {
            currentLocation["location-id"] = static_cast<qlonglong>(apt->guid());
            currentLocation["location-apt-runway"] = "ACTIVE";
        } // otherwise we failed to find the default airport in the nav-db :(
    }
    m_location->restoreLocation(currentLocation);

    updateSelectedAircraft();
    m_serversModel->requestRestore();

    emit summaryChanged();
 }

void LauncherController::saveSettings()
{
    emit requestSaveState();

    QSettings settings;
    settings.setValue("window-geometry", m_window->geometry());

    m_aircraftHistory->saveToSettings();
    m_locationHistory->saveToSettings();
}

void LauncherController::collectAircraftArgs()
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

void LauncherController::doRun()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
    m_config->reset();
    m_config->collect();

    m_aircraftHistory->insert(m_selectedAircraft);

    QVariant locSet = m_location->saveLocation();
    m_locationHistory->insert(locSet);

    // aircraft paths
    QSettings settings;
    QString downloadDir = settings.value("downloadSettings/downloadDir").toString();
    if (!downloadDir.isEmpty()) {
        QDir d(downloadDir);
        if (!d.exists()) {
            int result = QMessageBox::question(nullptr, tr("Create download folder?"),
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
}

void LauncherController::doApply()
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

    m_location->setLocationProperties();
    saveSettings();
}


QString LauncherController::selectAircraftStateAutomatically()
{
    if (!m_selectedAircraftInfo)
        return {};

    if (m_location->isAirborneLocation() && m_selectedAircraftInfo->hasState("approach")) 
    {
        return "approach";
    }

    if (m_location->isParkedLocation() && m_selectedAircraftInfo->hasState("parked")) {
        return "parked";
    } else {
        // also try 'engines-running'?
        if (m_selectedAircraftInfo->hasState("take-off"))
            return "take-off";
    }

    return {}; // failed to compute, give up
}

void LauncherController::maybeUpdateSelectedAircraft(QModelIndex index)
{
    QUrl u = index.data(AircraftURIRole).toUrl();
    if (u == m_selectedAircraft) {
        // potentially enable the run button now!
        updateSelectedAircraft();
    }
}

void LauncherController::updateSelectedAircraft()
{
    m_selectedAircraftInfo->setUri(m_selectedAircraft);
    QModelIndex index = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    if (index.isValid()) {
        m_aircraftType = Airplane;
        if (index.data(AircraftIsHelicopterRole).toBool()) {
            m_aircraftType = Helicopter;
        } else if (index.data(AircraftIsSeaplaneRole).toBool()) {
            m_aircraftType = Seaplane;
        }
    }

    emit canFlyChanged();
}

bool LauncherController::canFly() const
{
    QModelIndex index = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    if (!index.isValid()) {
        return false;
    }

    int status = index.data(AircraftPackageStatusRole).toInt();
    bool canRun = (status == LocalAircraftCache::PackageInstalled);
    return canRun;
}

void LauncherController::downloadDirChanged(QString path)
{
    // this can get run if the UI is disabled, just bail out before doing
    // anything permanent.
    if (!m_config->enableDownloadDirUI()) {
        return;
    }

    // if the default dir is passed in, map that back to the empty string
    if (path == m_config->defaultDownloadDir()) {
        path.clear();
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

    m_config->setValueForKey("", "download-dir", path);
    saveSettings();
    flightgear::restartTheApp();
}

QmlAircraftInfo *LauncherController::selectedAircraftInfo() const
{
    return m_selectedAircraftInfo;
}

void LauncherController::restoreLocation(QVariant var)
{
    m_location->restoreLocation(var.toMap());
}

QUrl LauncherController::selectedAircraft() const
{
    return m_selectedAircraft;
}

bool LauncherController::matchesSearch(QString term, QStringList keywords) const
{
    Q_FOREACH(QString s, keywords) {
        if (s.contains(term, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

bool LauncherController::isSearchActive() const
{
    return !m_settingsSearchTerm.isEmpty();
}

QStringList LauncherController::settingsSummary() const
{
    return m_settingsSummary;
}

QStringList LauncherController::environmentSummary() const
{
    return m_environmentSummary;
}


void LauncherController::setSelectedAircraft(QUrl selectedAircraft)
{
    if (m_selectedAircraft == selectedAircraft)
        return;

    m_selectedAircraft = selectedAircraft;
    updateSelectedAircraft();
    emit selectedAircraftChanged(m_selectedAircraft);
}

void LauncherController::setSettingsSearchTerm(QString settingsSearchTerm)
{
    if (m_settingsSearchTerm == settingsSearchTerm)
        return;

    m_settingsSearchTerm = settingsSearchTerm;
    emit searchChanged();
}

void LauncherController::setSettingsSummary(QStringList settingsSummary)
{
    if (m_settingsSummary == settingsSummary)
        return;

    m_settingsSummary = settingsSummary;
    emit summaryChanged();
}

void LauncherController::setEnvironmentSummary(QStringList environmentSummary)
{
    if (m_environmentSummary == environmentSummary)
        return;

    m_environmentSummary = environmentSummary;
    emit summaryChanged();
}

void LauncherController::fly()
{
    doRun();
    qApp->exit(1);
}

QStringList LauncherController::combinedSummary() const
{
    return m_settingsSummary + m_environmentSummary;
}

simgear::pkg::PackageRef LauncherController::packageForAircraftURI(QUrl uri) const
{
    if (uri.scheme() != "package") {
        qWarning() << "invalid URL scheme:" << uri;
        return simgear::pkg::PackageRef();
    }

    QString ident = uri.path();
    return globals->packageRoot()->getPackageById(ident.toStdString());
}

bool LauncherController::validateMetarString(QString metar)
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

void LauncherController::requestInstallUpdate(QUrl aircraftUri)
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

void LauncherController::requestUninstall(QUrl aircraftUri)
{
    simgear::pkg::PackageRef pref = packageForAircraftURI(aircraftUri);
    if (pref) {
        simgear::pkg::InstallRef i = pref->existingInstall();
        if (i) {
            i->uninstall();
        }
    }
}

void LauncherController::requestInstallCancel(QUrl aircraftUri)
{
    simgear::pkg::PackageRef pref = packageForAircraftURI(aircraftUri);
    if (pref) {
        simgear::pkg::InstallRef i = pref->existingInstall();
        if (i) {
            i->cancelDownload();
        }
    }
}

void LauncherController::requestUpdateAllAircraft()
{
    const PackageList& toBeUpdated = globals->packageRoot()->packagesNeedingUpdate();
    std::for_each(toBeUpdated.begin(), toBeUpdated.end(), [](PackageRef pkg) {
        globals->packageRoot()->scheduleToUpdate(pkg->install());
    });
}

void LauncherController::queryMPServers()
{
    m_serversModel->refresh();
}

QString LauncherController::versionString() const
{
    return FLIGHTGEAR_VERSION;
}

RecentAircraftModel *LauncherController::aircraftHistory()
{
    return m_aircraftHistory;
}

RecentLocationsModel *LauncherController::locationHistory()
{
    return m_locationHistory;
}

void LauncherController::launchUrl(QUrl url)
{
    QDesktopServices::openUrl(url);
}

QVariantList LauncherController::defaultSplashUrls() const
{
    QVariantList urls;

    for (auto path : flightgear::defaultSplashScreenPaths()) {
        QUrl url = QUrl::fromLocalFile(QString::fromStdString(path));
        urls.append(url);
    }

    return urls;
}

void LauncherController::onAircraftInstalledCompleted(QModelIndex index)
{
    maybeUpdateSelectedAircraft(index);
}

void LauncherController::onAircraftInstallFailed(QModelIndex index, QString errorMessage)
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


QPointF LauncherController::mapToGlobal(QQuickItem *item, const QPointF &pos) const
{
    QPointF scenePos = item->mapToScene(pos);
    QQuickWindow* win = item->window();
    return win->mapToGlobal(scenePos.toPoint());
}
