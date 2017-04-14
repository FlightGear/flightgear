#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QMenu>

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

// simgear headers
#include <simgear/package/Install.hxx>

// FlightGear headers
#include <Network/HTTPClient.hxx>
#include <Main/globals.hxx>
#include <Airports/airport.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>

// launcher headers
#include "QtLauncher.hxx"
#include "EditRatingsFilterDialog.hxx"
#include "AircraftItemDelegate.hxx"
#include "AircraftModel.hxx"
#include "PathsDialog.hxx"
#include "AircraftSearchFilterModel.hxx"
#include "DefaultAircraftLocator.hxx"
#include "SettingsWidgets.hxx"
#include "PreviewWindow.hxx"
#include "LaunchConfig.hxx"
#include "SettingsSectionQML.hxx"
#include "ExtraSettingsSection.hxx"
#include "ViewCommandLinePage.hxx"
#include "MPServersModel.h"

#include "ui_Launcher.h"
#include "ui_NoOfficialHangar.h"

const int MAX_RECENT_AIRCRAFT = 20;

using namespace simgear::pkg;

extern void restartTheApp(QStringList fgArgs);

class NoOfficialHangarMessage : public QWidget
{
    Q_OBJECT
public:
    NoOfficialHangarMessage() :
        m_ui(new Ui::NoOfficialHangarMessage)
    {
        m_ui->setupUi(this);
        // proxy this signal upwards
        connect(m_ui->label, &QLabel::linkActivated,
                this, &NoOfficialHangarMessage::linkActivated);
    }

Q_SIGNALS:
    void linkActivated(QUrl link);

private:
    Ui::NoOfficialHangarMessage* m_ui;
};

#include "LauncherMainWindow.moc"

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
    m_ui(NULL),
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

#if QT_VERSION >= 0x050200
    m_ui->aircraftFilter->setClearButtonEnabled(true);
#endif

    m_serversModel = new MPServersModel(this);
    m_serversModel->refresh();

    initQML();

    m_subsystemIdleTimer = new QTimer(this);
    m_subsystemIdleTimer->setInterval(0);
    connect(m_subsystemIdleTimer, &QTimer::timeout,
            this, &LauncherMainWindow::onSubsytemIdleTimeout);
    m_subsystemIdleTimer->start();

    // create and configure the proxy model
    m_aircraftProxy = new AircraftProxyModel(this);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setRatingFilterEnabled);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            this, &LauncherMainWindow::maybeRestoreAircraftSelection);

    connect(m_ui->onlyShowInstalledCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setInstalledFilterEnabled);
    connect(m_ui->aircraftFilter, &QLineEdit::textChanged,
            m_aircraftProxy, &AircraftProxyModel::setAircraftFilterString);

    connect(m_ui->flyButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->summaryButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->aircraftButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->locationButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->environmentButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->settingsButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);
    connect(m_ui->addOnsButton, &QAbstractButton::clicked, this, &LauncherMainWindow::onClickToolboxButton);

    connect(m_ui->aircraftHistory, &QPushButton::clicked,
          this, &LauncherMainWindow::onPopupAircraftHistory);
    connect(m_ui->locationHistory, &QPushButton::clicked,
          this, &LauncherMainWindow::onPopupLocationHistory);

    connect(m_ui->location, &LocationWidget::descriptionChanged,
            m_ui->locationDescription, &QLabel::setText);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &LauncherMainWindow::onQuit);
    addAction(qa);

    connect(m_ui->editRatingFilter, &QPushButton::clicked,
            this, &LauncherMainWindow::onEditRatingsFilter);
    connect(m_ui->onlyShowInstalledCheck, &QCheckBox::toggled,
            this, &LauncherMainWindow::onShowInstalledAircraftToggled);

    QIcon historyIcon(":/history-icon");
    m_ui->aircraftHistory->setIcon(historyIcon);
    m_ui->locationHistory->setIcon(historyIcon);

    m_aircraftModel = new AircraftItemModel(this);
    connect(m_aircraftModel, &AircraftItemModel::packagesNeedUpdating,
            this, &LauncherMainWindow::onPackagesNeedUpdate);
    m_aircraftProxy->setSourceModel(m_aircraftModel);

    m_aircraftProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_aircraftProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_aircraftProxy->setSortRole(Qt::DisplayRole);
    m_aircraftProxy->setDynamicSortFilter(true);

    m_ui->aircraftList->setModel(m_aircraftProxy);
    m_ui->aircraftList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    AircraftItemDelegate* delegate = new AircraftItemDelegate(m_ui->aircraftList);
    m_ui->aircraftList->setItemDelegate(delegate);
    m_ui->aircraftList->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_ui->aircraftList, &QListView::clicked,
            this, &LauncherMainWindow::onAircraftSelected);
    connect(delegate, &AircraftItemDelegate::variantChanged,
            this, &LauncherMainWindow::onAircraftSelected);
    connect(delegate, &AircraftItemDelegate::requestInstall,
            this, &LauncherMainWindow::onRequestPackageInstall);
    connect(delegate, &AircraftItemDelegate::requestUninstall,
            this, &LauncherMainWindow::onRequestPackageUninstall);
    connect(delegate, &AircraftItemDelegate::cancelDownload,
            this, &LauncherMainWindow::onCancelDownload);
    connect(delegate, &AircraftItemDelegate::showPreviews,
            this, &LauncherMainWindow::onShowPreviews);

    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallCompleted,
            this, &LauncherMainWindow::onAircraftInstalledCompleted);
    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallFailed,
            this, &LauncherMainWindow::onAircraftInstallFailed);
    connect(m_aircraftModel, &AircraftItemModel::scanCompleted,
            this, &LauncherMainWindow::updateSelectedAircraft);

    AddOnsPage* addOnsPage = new AddOnsPage(NULL, globals->packageRoot());
    connect(addOnsPage, &AddOnsPage::sceneryPathsChanged,
            this, &LauncherMainWindow::setSceneryPaths);
    connect(addOnsPage, &AddOnsPage::aircraftPathsChanged,
            this, &LauncherMainWindow::onAircraftPathsChanged);
    m_ui->stack->addWidget(addOnsPage);

    // after any kind of reset, try to restore selection and scroll
    // to match the m_selectedAircraft. This needs to be delayed
    // fractionally otherwise the scrollTo seems to be ignored,
    // unfortunately.
    connect(m_aircraftProxy, &AircraftProxyModel::modelReset,
            this, &LauncherMainWindow::delayedAircraftModelReset);

    connect(m_ui->updateAircraftLabel, &QLabel::linkActivated,
            this, &LauncherMainWindow::onUpdateAircraftLink);

    QSettings settings;
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->setPackageRoot(globals->packageRoot());
    m_aircraftModel->scanDirs();

    buildSettingsSections();
    buildEnvironmentSections();

    m_viewCommandLinePage = new ViewCommandLinePage;
    m_viewCommandLinePage->setLaunchConfig(m_config);
    m_ui->stack->addWidget(m_viewCommandLinePage);

    checkOfficialCatalogMessage();
    checkUpdateAircraft();
    restoreSettings();
    updateSettingsSummary();
}

void LauncherMainWindow::initQML()
{
    QQmlPrivate::RegisterAutoParent autoparent = { 0, &launcher_autoParent };
    QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);

    qmlRegisterType<SettingsSectionQML>("FlightGear.Launcher", 1, 0, "Section");
    qmlRegisterType<SettingsCheckbox>("FlightGear.Launcher", 1, 0, "Checkbox");
    qmlRegisterType<SettingsComboBox>("FlightGear.Launcher", 1, 0, "Combo");
    qmlRegisterType<SettingsIntSpinbox>("FlightGear.Launcher", 1, 0, "Spinbox");
    qmlRegisterType<SettingsText>("FlightGear.Launcher", 1, 0, "LineEdit");
    qmlRegisterType<SettingsDateTime>("FlightGear.Launcher", 1, 0, "DateTime");
    qmlRegisterType<SettingsPath>("FlightGear.Launcher", 1, 0, "PathChooser");
    qmlRegisterUncreatableType<QAbstractItemModel>("FlightGear.Launcher", 1, 0, "QAIM", "no");

    qmlRegisterUncreatableType<SettingsControl>("FlightGear.Launcher", 1, 0, "Control", "Base class");
    qmlRegisterUncreatableType<LaunchConfig>("FlightGear.Launcher", 1, 0, "LaunchConfig", "Singleton API");

    m_config = new LaunchConfig(this);
    connect(m_config, &LaunchConfig::collect, this, &LauncherMainWindow::collectAircraftArgs);
    m_ui->location->setLaunchConfig(m_config);

    m_qmlEngine = new QQmlEngine(this);
    m_qmlEngine->rootContext()->setContextProperty("_config", m_config);
    m_qmlEngine->rootContext()->setContextProperty("_launcher", this);
    m_qmlEngine->rootContext()->setContextProperty("_mpServers", m_serversModel);

#if defined(Q_OS_WIN)
    const QString osName("win");
#elif defined(Q_OS_MAC)
    const QString osName("mac");
#else
    const QString osName("unix");
#endif
    m_qmlEngine->rootContext()->setContextProperty("_osName", osName);

    flightgear::WeatherScenariosModel* weatherScenariosModel = new flightgear::WeatherScenariosModel(this);
    m_qmlEngine->rootContext()->setContextProperty("_weatherScenarios", weatherScenariosModel);
}

void LauncherMainWindow::buildSettingsSections()
{
    QVBoxLayout* settingsVBox = static_cast<QVBoxLayout*>(m_ui->settingsScrollContents->layout());

    QStringList sections = QStringList() << "general" << "mp" << "downloads" << "view" << "render";
    Q_FOREACH (QString section, sections) {
        QQmlComponent* comp = new QQmlComponent(m_qmlEngine, QUrl("qrc:///settings/" + section), this);
        if (comp->isError()) {
            qWarning() << "Errors parsing settings section:" << section << "\n" << comp->errorString();
        } else {
            SettingsSection* ss = qobject_cast<SettingsSection*>(comp->create());
            if (!ss) {
                qWarning() << "failed to create settings section from" << section;
            } else {
                ss->insertSettingsHeader();
                ss->setLaunchConfig(m_config);
                ss->setParent(m_ui->settingsScrollContents);
                settingsVBox->addWidget(ss);
                connect(ss, &SettingsSection::summaryChanged,
                        this, &LauncherMainWindow::updateSettingsSummary);
            }
        }
    }

    m_extraSettings = new ExtraSettingsSection(m_ui->settingsScrollContents);
    m_extraSettings->setLaunchConfig(m_config);
    settingsVBox->addWidget(m_extraSettings);
    settingsVBox->addStretch(1);

    connect(m_ui->settingsSearchEdit, &QLineEdit::textChanged,
            this, &LauncherMainWindow::onSettingsSearchChanged);
}

void LauncherMainWindow::buildEnvironmentSections()
{
    QVBoxLayout* settingsVBox = new QVBoxLayout;
    m_ui->environmentScrollContents->setLayout(settingsVBox);

    QStringList sections = QStringList() << "time" << "weather";
    Q_FOREACH (QString section, sections) {
        QQmlComponent* comp = new QQmlComponent(m_qmlEngine, QUrl("qrc:///environment/" + section), this);
        if (comp->isError()) {
            qWarning() << "Errors parsing environment section:" << section << "\n" << comp->errorString();
        } else {
            SettingsSection* ss = qobject_cast<SettingsSection*>(comp->create());
            if (!ss) {
                qWarning() << "failed to create environment section from" << section;
            } else {
                ss->insertSettingsHeader();
                ss->setLaunchConfig(m_config);
                ss->setParent(m_ui->environmentScrollContents);
                settingsVBox->addWidget(ss);
                connect(ss, &SettingsSection::summaryChanged,
                        this, &LauncherMainWindow::updateSettingsSummary);
            }
        }
    }

    settingsVBox->addStretch(1);
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

void LauncherMainWindow::restoreSettings()
{
    QSettings settings;

    restoreGeometry(settings.value("window-geometry").toByteArray());

    // full paths to -set.xml files
    m_recentAircraft = QUrl::fromStringList(settings.value("recent-aircraft").toStringList());

    if (!m_recentAircraft.empty()) {
        m_selectedAircraft = m_recentAircraft.front();
    } else {
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
    m_recentLocations = settings.value("recent-location-sets").toList();
    QVariantMap currentLocation;
    if (m_recentLocations.isEmpty()) {
        // use the default
        std::string defaultAirport = flightgear::defaultAirportICAO();
        FGAirportRef apt = FGAirport::findByIdent(defaultAirport);
        if (apt) {
            currentLocation["location-id"] = static_cast<qlonglong>(apt->guid());
            currentLocation["location-apt-runway"] = "active";
            qDebug() << "restored default airport:" << QString::fromStdString(defaultAirport);
        } // otherwise we failed to find the default airport in the nav-db :(
    } else {
        // we have a valid current location
        currentLocation = m_recentLocations.front().toMap();
    }

    m_ui->location->restoreLocation(currentLocation);

    // rating filters
    m_ui->onlyShowInstalledCheck->setChecked(settings.value("only-show-installed", false).toBool());
    if (m_ui->onlyShowInstalledCheck->isChecked()) {
        m_ui->ratingsFilterCheck->setEnabled(false);
    }

    m_ui->ratingsFilterCheck->setChecked(settings.value("ratings-filter", true).toBool());
    int index = 0;
    Q_FOREACH(QVariant v, settings.value("min-ratings").toList()) {
        m_ratingFilters[index++] = v.toInt();
    }

    m_aircraftProxy->setRatingFilterEnabled(m_ui->ratingsFilterCheck->isChecked());
    m_aircraftProxy->setRatings(m_ratingFilters);

    updateSelectedAircraft();
    maybeRestoreAircraftSelection();

    Q_FOREACH(SettingsSection* ss, findChildren<SettingsSection*>()) {
        ss->restoreState(settings);
    }

    m_serversModel->requestRestore();
 }

void LauncherMainWindow::delayedAircraftModelReset()
{
    QTimer::singleShot(1, this, SLOT(maybeRestoreAircraftSelection()));
}

void LauncherMainWindow::maybeRestoreAircraftSelection()
{
    QModelIndex aircraftIndex = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    QModelIndex proxyIndex = m_aircraftProxy->mapFromSource(aircraftIndex);
    if (proxyIndex.isValid()) {
        m_ui->aircraftList->selectionModel()->setCurrentIndex(proxyIndex,
                                                              QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftList->selectionModel()->select(proxyIndex,
                                                     QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftList->scrollTo(proxyIndex);

        // and also select the correct variant on the model
        m_aircraftModel->selectVariantForAircraftURI(m_selectedAircraft);
    }
}

void LauncherMainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue("ratings-filter", m_ui->ratingsFilterCheck->isChecked());
    settings.setValue("only-show-installed", m_ui->onlyShowInstalledCheck->isChecked());
    settings.setValue("recent-aircraft", QUrl::toStringList(m_recentAircraft));
    settings.setValue("recent-location-sets", m_recentLocations);

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

    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
      // manage aircraft history
        if (m_recentAircraft.contains(m_selectedAircraft))
          m_recentAircraft.removeOne(m_selectedAircraft);
        m_recentAircraft.prepend(m_selectedAircraft);
        if (m_recentAircraft.size() > MAX_RECENT_AIRCRAFT)
          m_recentAircraft.pop_back();
    }

    // aircraft paths
    QSettings settings;
    updateLocationHistory();

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

        // manage aircraft history
        if (m_recentAircraft.contains(m_selectedAircraft))
            m_recentAircraft.removeOne(m_selectedAircraft);
        m_recentAircraft.prepend(m_selectedAircraft);
        if (m_recentAircraft.size() > MAX_RECENT_AIRCRAFT)
            m_recentAircraft.pop_back();

        globals->get_props()->setStringValue("/sim/aircraft", aircraftPropValue);
        globals->get_props()->setStringValue("/sim/aircraft-dir", aircraftDir);
    }

    // location
    m_ui->location->setLocationProperties();

    saveSettings();
    m_accepted = true;
    m_runInApp = false;
}

void LauncherMainWindow::updateLocationHistory()
{
    QVariant locSet = m_ui->location->saveLocation();

    // check for existing; let's use description to imply uniqueness. This means
    // 'A1111' parkings get merged but I prefer that to keep the menu usable
    QVariant locDesc = locSet.toMap().value("text");
    auto it = std::remove_if(m_recentLocations.begin(), m_recentLocations.end(),
                   [locDesc](QVariant v) { return v.toMap().value("text") == locDesc; });
    m_recentLocations.erase(it, m_recentLocations.end());

    // now we can always prepend
    m_recentLocations.prepend(locSet);
    if (m_recentLocations.size() > MAX_RECENT_AIRCRAFT)
        m_recentLocations.pop_back();
}

void LauncherMainWindow::onQuit()
{
    if (m_inAppMode) {
        m_runInApp = false;
    } else {
        qApp->exit(-1);
    }
}

#if 0
void LauncherMainWindow::onToggleTerrasync(bool enabled)
{
    if (enabled) {
        QSettings settings;
        QString downloadDir = settings.value("download-dir").toString();
        if (downloadDir.isEmpty()) {
            downloadDir = QString::fromStdString(flightgear::defaultDownloadDir().utf8Str());
        }

        QFileInfo info(downloadDir);
        if (!info.exists()) {
            QMessageBox msg;
            msg.setWindowTitle(tr("Create download folder?"));
            msg.setText(tr("The download folder '%1' does not exist, create it now?").arg(downloadDir));
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::Cancel);
            int result = msg.exec();

            if (result == QMessageBox::Cancel) {
                //m_ui->terrasyncCheck->setChecked(false);
                return;
            }

            QDir d(downloadDir);
            d.mkpath(downloadDir);
        }
    } // of is enabled
}
#endif

void LauncherMainWindow::onAircraftInstalledCompleted(QModelIndex index)
{
    maybeUpdateSelectedAircraft(index);
}

void LauncherMainWindow::onRatingsFilterToggled()
{
    QModelIndex aircraftIndex = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    QModelIndex proxyIndex = m_aircraftProxy->mapFromSource(aircraftIndex);
    if (proxyIndex.isValid()) {
        m_ui->aircraftList->scrollTo(proxyIndex);
    }
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

void LauncherMainWindow::onAircraftSelected(const QModelIndex& index)
{
    m_selectedAircraft = index.data(AircraftURIRole).toUrl();
    updateSelectedAircraft();
}

void LauncherMainWindow::onRequestPackageInstall(const QModelIndex& index)
{
    // also select, otherwise UI is confusing
    m_selectedAircraft = index.data(AircraftURIRole).toUrl();
    updateSelectedAircraft();

    QString pkg = index.data(AircraftPackageIdRole).toString();
    simgear::pkg::PackageRef pref = globals->packageRoot()->getPackageById(pkg.toStdString());
    if (pref->isInstalled()) {
        InstallRef install = pref->existingInstall();
        if (install && install->hasUpdate()) {
            globals->packageRoot()->scheduleToUpdate(install);
        }
    } else {
        pref->install();
    }
}

void LauncherMainWindow::onRequestPackageUninstall(const QModelIndex& index)
{

    m_selectedAircraft = index.data(AircraftURIRole).toUrl();
    updateSelectedAircraft();

    QString pkg = index.data(AircraftPackageIdRole).toString();
    if (pkg.isEmpty()) {
        return; // can't be uninstalled, not from a package
    }

    simgear::pkg::PackageRef pref = globals->packageRoot()->getPackageById(pkg.toStdString());
    if (pref && pref->isInstalled()) {
        qDebug() << "uninstalling" << pkg;
        pref->existingInstall()->uninstall();
    }
}

void LauncherMainWindow::onShowPreviews(const QModelIndex &index)
{
    QVariant urls = index.data(AircraftPreviewsRole);

    PreviewWindow* previewWindow = new PreviewWindow;
    previewWindow->setUrls(urls.toList());
}

void LauncherMainWindow::onCancelDownload(const QModelIndex& index)
{
    QString pkg = index.data(AircraftPackageIdRole).toString();
    simgear::pkg::PackageRef pref = globals->packageRoot()->getPackageById(pkg.toStdString());
    simgear::pkg::InstallRef i = pref->existingInstall();
    i->cancelDownload();
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
    QModelIndex index = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    if (index.isValid()) {
        QPixmap pm = index.data(Qt::DecorationRole).value<QPixmap>();
        m_ui->thumbnail->setPixmap(pm);
        m_ui->aircraftName->setText(index.data(Qt::DisplayRole).toString());

        QVariant longDesc = index.data(AircraftLongDescriptionRole);
        m_ui->aircraftDescription->setVisible(!longDesc.isNull());
        m_ui->aircraftDescription->setText(longDesc.toString());

        int status = index.data(AircraftPackageStatusRole).toInt();
        bool canRun = (status == PackageInstalled);
        m_ui->flyButton->setEnabled(canRun);

        LauncherAircraftType aircraftType = Airplane;
        if (index.data(AircraftIsHelicopterRole).toBool()) {
            aircraftType = Helicopter;
        } else if (index.data(AircraftIsSeaplaneRole).toBool()) {
            aircraftType = Seaplane;
        }

        m_ui->location->setAircraftType(aircraftType);
    } else {
        m_ui->thumbnail->setPixmap(QPixmap());
        m_ui->aircraftName->setText("");
        m_ui->aircraftDescription->hide();
        m_ui->flyButton->setEnabled(false);
    }
}

void LauncherMainWindow::onPackagesNeedUpdate(bool yes)
{
    Q_UNUSED(yes);
    checkUpdateAircraft();
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

QModelIndex LauncherMainWindow::proxyIndexForAircraftURI(QUrl uri) const
{
  return m_aircraftProxy->mapFromSource(sourceIndexForAircraftURI(uri));
}

QModelIndex LauncherMainWindow::sourceIndexForAircraftURI(QUrl uri) const
{
    AircraftItemModel* sourceModel = qobject_cast<AircraftItemModel*>(m_aircraftProxy->sourceModel());
    Q_ASSERT(sourceModel);
    return sourceModel->indexOfAircraftURI(uri);
}

void LauncherMainWindow::onPopupAircraftHistory()
{
    if (m_recentAircraft.isEmpty()) {
        return;
    }

    QMenu m;
    Q_FOREACH(QUrl uri, m_recentAircraft) {
        QString nm = m_aircraftModel->nameForAircraftURI(uri);
        if (nm.isEmpty()) {
            continue;
        }

        QAction* act = m.addAction(nm);
        act->setData(uri);
    }

    QPoint popupPos = m_ui->aircraftHistory->mapToGlobal(m_ui->aircraftHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        const QUrl uri = triggered->data().toUrl();
        m_selectedAircraft = uri;
        QModelIndex index = proxyIndexForAircraftURI(m_selectedAircraft);
        m_ui->aircraftList->selectionModel()->setCurrentIndex(index,
                                                              QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftFilter->clear();
        m_aircraftModel->selectVariantForAircraftURI(uri);
        updateSelectedAircraft();
    }
}

void LauncherMainWindow::onPopupLocationHistory()
{
    if (m_recentLocations.isEmpty()) {
        return;
    }

    QMenu m;
    Q_FOREACH(QVariant loc, m_recentLocations) {
        QString summary = loc.toMap().value("text").toString();
        QAction* act = m.addAction(summary);
        act->setData(loc);
    }

    QPoint popupPos = m_ui->locationHistory->mapToGlobal(m_ui->locationHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        m_ui->location->restoreLocation(triggered->data().toMap());
    }
}

void LauncherMainWindow::onEditRatingsFilter()
{
    EditRatingsFilterDialog dialog(this);
    dialog.setRatings(m_ratingFilters);

    dialog.exec();
    if (dialog.result() == QDialog::Accepted) {
        QVariantList vl;
        for (int i=0; i<4; ++i) {
            m_ratingFilters[i] = dialog.getRating(i);
            vl.append(m_ratingFilters[i]);
        }
        m_aircraftProxy->setRatings(m_ratingFilters);

        QSettings settings;
        settings.setValue("min-ratings", vl);
    }
}

void LauncherMainWindow::updateSettingsSummary()
{
    QStringList summary;

    Q_FOREACH(SettingsSection* ss, findChildren<SettingsSection*>()) {
        QString s = ss->summary();
        if (!s.isEmpty()) {
            QStringList pieces = s.split(';', QString::SkipEmptyParts);
            summary.append(pieces);
        }
    }

    QString s = summary.join(", ");
    s[0] = s[0].toUpper();
    m_ui->settingsDescription->setText(s);
}

void LauncherMainWindow::onShowInstalledAircraftToggled(bool b)
{
    m_ui->ratingsFilterCheck->setEnabled(!b);
    maybeRestoreAircraftSelection();
}

void LauncherMainWindow::onSubsytemIdleTimeout()
{
    globals->get_subsystem_mgr()->update(0.0);
}

void LauncherMainWindow::onDownloadDirChanged()
{

    // replace existing package root
    globals->get_subsystem<FGHTTPClient>()->shutdown();
    globals->setPackageRoot(simgear::pkg::RootRef());

    // create new root with updated download-dir value
    fgInitPackageRoot();

    globals->get_subsystem<FGHTTPClient>()->init();

    QSettings settings;
    // re-scan the aircraft list
    m_aircraftModel->setPackageRoot(globals->packageRoot());
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->scanDirs();

    checkOfficialCatalogMessage();

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
void LauncherMainWindow::checkOfficialCatalogMessage()
{
    const bool show = shouldShowOfficialCatalogMessage();
    m_aircraftModel->setMessageWidgetVisible(show);
    if (show) {
        NoOfficialHangarMessage* messageWidget = new NoOfficialHangarMessage;
        connect(messageWidget, &NoOfficialHangarMessage::linkActivated,
                this, &LauncherMainWindow::onOfficialCatalogMessageLink);

        QModelIndex index = m_aircraftProxy->mapFromSource(m_aircraftModel->messageWidgetIndex());
        m_ui->aircraftList->setIndexWidget(index, messageWidget);
    }
}

void LauncherMainWindow::onOfficialCatalogMessageLink(QUrl link)
{
    QString s = link.toString();
    if (s == "action:hide") {
        QSettings settings;
        settings.setValue("hide-official-catalog-message", true);
    } else if (s == "action:add-official") {
        AddOnsPage::addDefaultCatalog(this, false /* not silent */);
    }

    checkOfficialCatalogMessage();
}

void LauncherMainWindow::checkUpdateAircraft()
{
    const size_t numToUpdate = globals->packageRoot()->packagesNeedingUpdate().size();
    const bool showUpdateMessage = (numToUpdate > 0);

    static QString originalText = m_ui->updateAircraftLabel->text();
    const QString t = originalText.arg(numToUpdate);
    m_ui->updateAircraftLabel->setText(t);
    m_ui->updateAircraftLabel->setVisible(showUpdateMessage);
}

void LauncherMainWindow::onUpdateAircraftLink(QUrl link)
{
    QString s = link.toString();
    if (s == "action:hide") {
        m_ui->updateAircraftLabel->hide();
    } else if (s == "action:update") {
        m_ui->updateAircraftLabel->hide();
        const PackageList& toBeUpdated = globals->packageRoot()->packagesNeedingUpdate();
        std::for_each(toBeUpdated.begin(), toBeUpdated.end(), [](PackageRef pkg) {
            globals->packageRoot()->scheduleToUpdate(pkg->install());
        });
    }
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
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->scanDirs();
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
    Q_FOREACH(SettingsSectionQML* ss, findChildren<SettingsSectionQML*>()) {
        ss->setSearchTerm(m_ui->settingsSearchEdit->text());
    }
}
