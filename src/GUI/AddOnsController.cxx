#include "AddOnsController.hxx"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QQmlComponent>
#include <QDesktopServices>
#include <QValidator>

#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>

#include <Main/globals.hxx>
#include <Network/HTTPClient.hxx>

#include "Add-ons/Addon.hxx"
#include "Add-ons/addon_fwd.hxx"
#include "LocalAircraftCache.hxx"
#include "LauncherMainWindow.hxx"
#include "CatalogListModel.hxx"
#include "AddonsModel.hxx"
#include "InstallSceneryDialog.hxx"
#include "QtLauncher.hxx"
#include "PathListModel.hxx"
#include "LaunchConfig.hxx"

//////////////////////////////////////////////////////////////////////////

AddOnsController::AddOnsController(LauncherMainWindow *parent, LaunchConfig* config) :
    QObject(parent),
    m_launcher(parent),
    m_config(config)
{
    m_catalogs = new CatalogListModel(this,
                                      simgear::pkg::RootRef(globals->packageRoot()));

    connect(m_catalogs, &CatalogListModel::catalogsChanged, this, &AddOnsController::onCatalogsChanged);

    m_addonsModuleModel = new AddonsModel(this);
    connect(m_addonsModuleModel, &AddonsModel::modulesChanged, this, &AddOnsController::onAddonsChanged);

    using namespace flightgear::addons;

    m_sceneryPaths = new PathListModel(this);
    connect(m_sceneryPaths, &PathListModel::enabledPathsChanged, [this] () {
        m_sceneryPaths->saveToSettings("scenery-paths-v2");
        flightgear::launcherSetSceneryPaths();
    });
    m_sceneryPaths->loadFromSettings("scenery-paths-v2");

    m_aircraftPaths = new PathListModel(this);
    m_aircraftPaths->loadFromSettings("aircraft-paths-v2");

    // sync up the aircraft cache now
    auto aircraftCache = LocalAircraftCache::instance();
    aircraftCache->setPaths(m_aircraftPaths->enabledPaths());
    aircraftCache->scanDirs();

    // watch for future changes
    connect(m_aircraftPaths, &PathListModel::enabledPathsChanged, [this] () {
        m_aircraftPaths->saveToSettings("aircraft-paths-v2");

        auto aircraftCache = LocalAircraftCache::instance();
        aircraftCache->setPaths(m_aircraftPaths->enabledPaths());
        aircraftCache->scanDirs();
    });

    QSettings settings;
    int size = settings.beginReadArray("addon-modules");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);

        QString path = settings.value("path").toString();
        const SGPath addonPath(path.toStdString());
        if (!addonPath.exists()) {
            // drop non-existing paths on load
            continue;
        }

        m_addonModulePaths.push_back( path );
        bool enable = settings.value("enable").toBool();

        try {
            auto addon = Addon::fromAddonDir<AddonRef>(addonPath);
            m_addonsModuleModel->append(path, addon, enable);
        }
        catch (const sg_exception &e) {
            string msg = "Error getting add-on metadata: " + e.getFormattedMessage();
            SG_LOG(SG_GENERAL, SG_ALERT, msg);
        }
    }
    settings.endArray();

    qmlRegisterUncreatableType<AddOnsController>("FlightGear.Launcher", 1, 0, "AddOnsControllers", "no");
    qmlRegisterUncreatableType<CatalogListModel>("FlightGear.Launcher", 1, 0, "CatalogListModel", "no");
    qmlRegisterUncreatableType<AddonsModel>("FlightGear.Launcher", 1, 0, "AddonsModel", "no");
    qmlRegisterUncreatableType<PathListModel>("FlightGear.Launcher", 1, 0, "PathListMode", "no");

    connect(m_config, &LaunchConfig::collect,
            this, &AddOnsController::collectArgs);
}

PathListModel* AddOnsController::aircraftPaths() const
{
    return m_aircraftPaths;
}

PathListModel* AddOnsController::sceneryPaths() const
{
    return m_sceneryPaths;
}

QStringList AddOnsController::modulePaths() const
{
    return m_addonModulePaths;
}

QString AddOnsController::addAircraftPath() const
{
    QString path = QFileDialog::getExistingDirectory(nullptr, tr("Choose aircraft folder"));
    if (path.isEmpty()) {
        return {};
    }

    // the user might add a directory containing an 'Aircraft' subdir. Let's attempt
    // to check for that case and handle it gracefully.
    bool pathOk = false;
    if (LocalAircraftCache::isCandidateAircraftPath(path)) {
        pathOk = true;
    } else {
        // no aircraft in specified path, look for Aircraft/ subdir
        QDir d(path);
        if (d.exists("Aircraft")) {
            QString p2 = d.filePath("Aircraft");
            if (LocalAircraftCache::isCandidateAircraftPath(p2)) {
                pathOk = true;
                path = p2;
            }
        }
    }

    if (!pathOk) {
        QMessageBox mb;
        mb.setText(tr("No aircraft found in the folder '%1' - add anyway?").arg(path));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        mb.setDefaultButton(QMessageBox::No);
        mb.exec();

        if (mb.result() == QMessageBox::No) {
            return {};
        }
    }

    m_aircraftPaths->appendPath(path);
    return path;
}


QString AddOnsController::addAddOnModulePath() const
{
    QString path = QFileDialog::getExistingDirectory(nullptr, tr("Choose addon module folder"));
    if (path.isEmpty()) {
        return {};

    }

    // validation
    SGPath p(path.toStdString());
    bool isValid = false;

    for (const auto& file: {"addon-metadata.xml", "addon-main.nas"}) {
        if ((p / file).exists()) {
            isValid = true;
            break;
        }
    }

    if (!isValid) {
        QMessageBox mb;
        mb.setText(tr("The folder '%1' doesn't appear to contain an addon module - add anyway?").arg(path));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        mb.setDefaultButton(QMessageBox::No);
        mb.setInformativeText(tr("Added modules should contain at least both of the following "
                                 "files: addon-metadata.xml, addon-main.nas."));
        mb.exec();

        if (mb.result() == QMessageBox::No) {
            return {};
        }
    }

    using namespace flightgear::addons;
    const SGPath addonPath(path.toStdString());

    try {
        // when newly added, enable by default
        auto addon = Addon::fromAddonDir<AddonRef>(addonPath);
        if (!m_addonsModuleModel->append(path, addon, true)) {
            path = QString("");
        }
    } catch (const sg_exception &e) {
        string msg = "Error getting add-on metadata: " + e.getFormattedMessage();
        SG_LOG(SG_GENERAL, SG_ALERT, msg);
    }

    return path;
}

QString AddOnsController::addSceneryPath() const
{
    QString path = QFileDialog::getExistingDirectory(nullptr, tr("Choose scenery folder"));
    if (path.isEmpty()) {
        return {};

    }

    // validation
    SGPath p(path.toStdString());
    bool isValid = false;

    for (const auto& dir: {"Objects", "Terrain", "Buildings", "Roads", "Pylons", "NavData", "Airports", "Orthophotos"}) {
        if ((p / dir).exists()) {
            isValid = true;
            break;
        }
    }

    if (!isValid) {
        QMessageBox mb;
        mb.setText(tr("The folder '%1' doesn't appear to contain scenery - add anyway?").arg(path));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        mb.setDefaultButton(QMessageBox::No);
        mb.setInformativeText(tr("Added scenery should contain at least one of the following "
                                 "folders: Objects, Terrain, Buildings, Roads, Pylons, NavData, Airports, Orthophotos."));
        mb.exec();

        if (mb.result() == QMessageBox::No) {
            return {};
        }
    }

    m_sceneryPaths->appendPath(path);
    return path;
}

QString AddOnsController::installCustomScenery()
{
    QSettings settings;
    QString downloadDir = settings.value("download-dir").toString();
    InstallSceneryDialog dlg(nullptr, downloadDir);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.sceneryPath();
    }

    return {};
}

void AddOnsController::openDirectory(QString path)
{
    QUrl u = QUrl::fromLocalFile(path);
    QDesktopServices::openUrl(u);
}

void AddOnsController::setAddons(AddonsModel* addons)
{
    
}


void AddOnsController::setModulePaths(QStringList modulePaths)
{
    if (m_addonModulePaths == modulePaths)
        return;

    m_addonModulePaths = modulePaths;

    m_addonsModuleModel->resetData(modulePaths);

    QSettings settings;
    int i = 0;
    settings.beginWriteArray("addon-modules");
    for (const QString& path : modulePaths) {
        if (m_addonsModuleModel->containsPath(path)) {
            settings.setArrayIndex(i++);
            settings.setValue("path", path);
            settings.setValue("enable", m_addonsModuleModel->getPathEnable(path));
        }
    }
    settings.endArray();

    emit modulePathsChanged(m_addonModulePaths);
    emit modulesChanged();
}

void AddOnsController::officialCatalogAction(QString s)
{
    if (s == "hide") {
        QSettings settings;
        settings.setValue("hide-official-catalog-message", true);
    } else if (s == "add-official") {
        m_catalogs->installDefaultCatalog(false);
    }

    emit showNoOfficialHangarChanged();
}

bool AddOnsController::shouldShowOfficialCatalogMessage() const
{
    QSettings settings;
    bool showOfficialCatalogMesssage = !globals->get_subsystem<FGHTTPClient>()->isDefaultCatalogInstalled();
    if (settings.value("hide-official-catalog-message").toBool()) {
        showOfficialCatalogMesssage = false;
    }
    return showOfficialCatalogMesssage;
}


bool AddOnsController::isOfficialHangarRegistered()
{
    return globals->get_subsystem<FGHTTPClient>()->isDefaultCatalogInstalled();
}

bool AddOnsController::showNoOfficialHangar() const
{
    return shouldShowOfficialCatalogMessage();
}

void AddOnsController::onCatalogsChanged()
{
    emit showNoOfficialHangarChanged();
    emit isOfficialHangarRegisteredChanged();
}


void AddOnsController::onAddonsChanged()
{
    QSettings settings;

    int i = 0;
    settings.beginWriteArray("addon-modules");
    for (const auto& path : m_addonModulePaths) {
        settings.setArrayIndex(i++);
        settings.setValue("path", path);
        settings.setValue("enable", m_addonsModuleModel->getPathEnable(path));
    }
    settings.endArray();
}

void AddOnsController::collectArgs()
{
    // scenery paths
    Q_FOREACH(QString path, m_sceneryPaths->enabledPaths()) {
        m_config->setArg("fg-scenery", path);
    }

    // aircraft paths
    Q_FOREACH(QString path, m_aircraftPaths->enabledPaths()) {
        m_config->setArg("fg-aircraft", path);
    }

    // add-on module paths
    // we could query this directly from AddonsModel, but this is simpler right now
    QSettings settings;
    int size = settings.beginReadArray("addon-modules");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);

        QString path = settings.value("path").toString();
        const SGPath addonPath(path.toStdString());
        if (!addonPath.exists()) {
            // don't pass missing paths
            continue;
        }

        bool enable = settings.value("enable").toBool();
        if (enable) {
            m_config->setArg("addon", path);
        }
    }
    settings.endArray();
}
