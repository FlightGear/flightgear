#include "LauncherMainWindow.hxx"

// Qt headers
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QMenu>

// simgear headers
#include <simgear/package/Install.hxx>

// FlightGear headers
#include <Network/RemoteXMLRequest.hxx>
#include <Network/HTTPClient.hxx>
#include <Main/globals.hxx>
#include <Airports/airport.hxx>
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>

// launcher headers
#include "QtLauncher.hxx"
#include "renderingsettings.h"
#include "ViewSettings.h"
#include "MPSettings.h"
#include "DownloadSettings.h"
#include "AdditionalSettings.h"
#include "EditRatingsFilterDialog.hxx"
#include "AircraftItemDelegate.hxx"
#include "AircraftModel.hxx"
#include "PathsDialog.hxx"
#include "EditCustomMPServerDialog.hxx"
#include "LauncherArgumentTokenizer.hxx"
#include "AircraftSearchFilterModel.hxx"
#include "DefaultAircraftLocator.hxx"

#include "ui_Launcher.h"
#include "ui_NoOfficialHangar.h"
#include "ui_UpdateAllAircraft.h"

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

class UpdateAllAircraftMessage : public QWidget
{
    Q_OBJECT
public:
    UpdateAllAircraftMessage() :
        m_ui(new Ui::UpdateAllAircraftMessage)
    {
        m_ui->setupUi(this);
        // proxy this signal upwards
        connect(m_ui->label, &QLabel::linkActivated, this, &UpdateAllAircraftMessage::linkActivated);
        connect(m_ui->updateAllButton, &QPushButton::clicked, this, &UpdateAllAircraftMessage::updateAll);
    }

Q_SIGNALS:
    void linkActivated(QUrl link);
    void updateAll();
private:
    Ui::UpdateAllAircraftMessage* m_ui;
};

#include "LauncherMainWindow.moc"

//////////////////////////////////////////////////////////////////////////////

LauncherMainWindow::LauncherMainWindow() :
    QMainWindow(),
    m_ui(NULL),
    m_subsystemIdleTimer(NULL),
    m_doRestoreMPServer(false)
{
    m_ui.reset(new Ui::Launcher);
    m_ui->setupUi(this);

#if QT_VERSION >= 0x050300
    // don't require Qt 5.3
    //m_ui->commandLineArgs->setPlaceholderText("--option=value --prop:/sim/name=value");
#endif

#if QT_VERSION >= 0x050200
    m_ui->aircraftFilter->setClearButtonEnabled(true);
#endif

    for (int i=0; i<4; ++i) {
        m_ratingFilters[i] = 3;
    }

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


   // connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));

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

#if 0
    connect(m_ui->timeOfDayCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->seasonCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->fetchRealWxrCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->rembrandtCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->terrasyncCheck, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->startPausedCheck, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->msaaCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));

    connect(m_ui->mpBox, SIGNAL(toggled(bool)),
            this, SLOT(updateSettingsSummary()));
    connect(m_ui->mpCallsign, SIGNAL(textChanged(QString)),
            this, SLOT(updateSettingsSummary()));

    connect(m_ui->rembrandtCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(onRembrandtToggled(bool)));
    connect(m_ui->terrasyncCheck, &QCheckBox::toggled,
            this, &LauncherMainWindow::onToggleTerrasync);
#endif

    updateSettingsSummary();

#if 0
    connect(m_ui->mpServerCombo, SIGNAL(activated(int)),
            this, SLOT(onMPServerActivated(int)));
#endif
    m_aircraftModel = new AircraftItemModel(this);
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

    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallCompleted,
            this, &LauncherMainWindow::onAircraftInstalledCompleted);
    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallFailed,
            this, &LauncherMainWindow::onAircraftInstallFailed);
    connect(m_aircraftModel, &AircraftItemModel::scanCompleted,
            this, &LauncherMainWindow::updateSelectedAircraft);
#if 0
    connect(m_ui->restoreDefaultsButton, &QPushButton::clicked,
            this, &LauncherMainWindow::onRestoreDefaults);
#endif

    AddOnsPage* addOnsPage = new AddOnsPage(NULL, globals->packageRoot());
    connect(addOnsPage, &AddOnsPage::downloadDirChanged,
            this, &LauncherMainWindow::onDownloadDirChanged);
    connect(addOnsPage, &AddOnsPage::sceneryPathsChanged,
            this, &LauncherMainWindow::setSceneryPaths);

  //  m_ui->tabWidget->addTab(addOnsPage, tr("Add-ons"));
    // after any kind of reset, try to restore selection and scroll
    // to match the m_selectedAircraft. This needs to be delayed
    // fractionally otherwise the scrollTo seems to be ignored,
    // unfortunately.
    connect(m_aircraftProxy, &AircraftProxyModel::modelReset,
            this, &LauncherMainWindow::delayedAircraftModelReset);

    QSettings settings;
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->setPackageRoot(globals->packageRoot());
    m_aircraftModel->scanDirs();

    checkOfficialCatalogMessage();
    restoreSettings();

    onRefreshMPServers();

    RenderingSettings* renderSettings = new RenderingSettings(m_ui->settingsScrollContents);
    QVBoxLayout* settingsVBox = static_cast<QVBoxLayout*>(m_ui->settingsScrollContents->layout());
    settingsVBox->addWidget(renderSettings);

    ViewSettings* viewSettings = new ViewSettings(m_ui->settingsScrollContents);
    settingsVBox->addWidget(viewSettings);

    MPSettings* mpSettings = new MPSettings(m_ui->settingsScrollContents);
    settingsVBox->addWidget(mpSettings);

    DownloadSettings* downloadSettings = new DownloadSettings(m_ui->settingsScrollContents);
    settingsVBox->addWidget(downloadSettings);

    AdditionalSettings* addSettings = new AdditionalSettings(m_ui->settingsScrollContents);
    settingsVBox->addWidget(addSettings);

    settingsVBox->addStretch(1);
}

LauncherMainWindow::~LauncherMainWindow()
{
    // if we don't cancel this now, it may complete after we are gone,
    // causing a crash when the SGCallback fires (SGCallbacks don't clean up
    // when their subject is deleted)
    globals->get_subsystem<FGHTTPClient>()->client()->cancelRequest(m_mpServerRequest);
}

bool LauncherMainWindow::execInApp()
{
  m_inAppMode = true;
 // m_ui->tabWidget->removeTab(3);
 // m_ui->tabWidget->removeTab(3);

 // m_ui->runButton->setText(tr("Apply"));
 // m_ui->quitButton->setText(tr("Cancel"));

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

#if 0
    m_ui->rembrandtCheckbox->setChecked(settings.value("enable-rembrandt", false).toBool());
    m_ui->terrasyncCheck->setChecked(settings.value("enable-terrasync", true).toBool());
    m_ui->fullScreenCheckbox->setChecked(settings.value("start-fullscreen", false).toBool());
    m_ui->msaaCheckbox->setChecked(settings.value("enable-msaa", false).toBool());
    m_ui->fetchRealWxrCheckbox->setChecked(settings.value("enable-realwx", true).toBool());
    m_ui->startPausedCheck->setChecked(settings.value("start-paused", false).toBool());
    m_ui->timeOfDayCombo->setCurrentIndex(settings.value("timeofday", 0).toInt());
    m_ui->seasonCombo->setCurrentIndex(settings.value("season", 0).toInt());
#endif
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
#if 0
    m_ui->commandLineArgs->setPlainText(settings.value("additional-args").toString());

    m_ui->mpBox->setChecked(settings.value("mp-enabled").toBool());
    m_ui->mpCallsign->setText(settings.value("mp-callsign").toString());
#endif
    // don't restore MP server here, we do it after a refresh
    m_doRestoreMPServer = true;
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
#if 0
    settings.setValue("enable-rembrandt", m_ui->rembrandtCheckbox->isChecked());
    settings.setValue("enable-terrasync", m_ui->terrasyncCheck->isChecked());
    settings.setValue("enable-msaa", m_ui->msaaCheckbox->isChecked());
    settings.setValue("start-fullscreen", m_ui->fullScreenCheckbox->isChecked());
    settings.setValue("enable-realwx", m_ui->fetchRealWxrCheckbox->isChecked());
    settings.setValue("start-paused", m_ui->startPausedCheck->isChecked());
#endif

    settings.setValue("ratings-filter", m_ui->ratingsFilterCheck->isChecked());
    settings.setValue("only-show-installed", m_ui->onlyShowInstalledCheck->isChecked());
    settings.setValue("recent-aircraft", QUrl::toStringList(m_recentAircraft));
    settings.setValue("recent-location-sets", m_recentLocations);

#if 0
    settings.setValue("timeofday", m_ui->timeOfDayCombo->currentIndex());
    settings.setValue("season", m_ui->seasonCombo->currentIndex());
    settings.setValue("additional-args", m_ui->commandLineArgs->toPlainText());

    settings.setValue("mp-callsign", m_ui->mpCallsign->text());
    settings.setValue("mp-server", m_ui->mpServerCombo->currentData());
    settings.setValue("mp-enabled", m_ui->mpBox->isChecked());
#endif
    settings.setValue("window-geometry", saveGeometry());
}

void LauncherMainWindow::setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
    std::string stdName(name.toStdString());
    if (cbox->isChecked()) {
        opt->addOption("enable-" + stdName, "");
    } else {
        opt->addOption("disable-" + stdName, "");
    }
}

void LauncherMainWindow::closeEvent(QCloseEvent *event)
{
    qApp->exit(-1);
}

void LauncherMainWindow::onRun()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
#if 0
    setEnableDisableOptionFromCheckbox(m_ui->terrasyncCheck, "terrasync");
    setEnableDisableOptionFromCheckbox(m_ui->fetchRealWxrCheckbox, "real-weather-fetch");
    setEnableDisableOptionFromCheckbox(m_ui->rembrandtCheckbox, "rembrandt");
    setEnableDisableOptionFromCheckbox(m_ui->fullScreenCheckbox, "fullscreen");
//    setEnableDisableOptionFromCheckbox(m_ui->startPausedCheck, "freeze");

    bool startPaused = m_ui->startPausedCheck->isChecked() ||
            m_ui->location->shouldStartPaused();
    if (startPaused) {
        opt->addOption("enable-freeze", "");
    }
#endif

#if 0
    // MSAA is more complex
    if (!m_ui->rembrandtCheckbox->isChecked()) {
        if (m_ui->msaaCheckbox->isChecked()) {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 1);
            globals->get_props()->setIntValue("/sim/rendering/multi-samples", 4);
        } else {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 0);
        }
    }
#endif

    // aircraft
    if (!m_selectedAircraft.isEmpty()) {
        if (m_selectedAircraft.isLocalFile()) {
            QFileInfo setFileInfo(m_selectedAircraft.toLocalFile());
            opt->addOption("aircraft-dir", setFileInfo.dir().absolutePath().toStdString());
            QString setFile = setFileInfo.fileName();
            Q_ASSERT(setFile.endsWith("-set.xml"));
            setFile.truncate(setFile.count() - 8); // drop the '-set.xml' portion
            opt->addOption("aircraft", setFile.toStdString());
        } else if (m_selectedAircraft.scheme() == "package") {
            QString qualifiedId = m_selectedAircraft.path();
            // no need to set aircraft-dir, handled by the corresponding code
            // in fgInitAircraft
            opt->addOption("aircraft", qualifiedId.toStdString());
        } else {
            qWarning() << "unsupported aircraft launch URL" << m_selectedAircraft;
        }

      // manage aircraft history
        if (m_recentAircraft.contains(m_selectedAircraft))
          m_recentAircraft.removeOne(m_selectedAircraft);
        m_recentAircraft.prepend(m_selectedAircraft);
        if (m_recentAircraft.size() > MAX_RECENT_AIRCRAFT)
          m_recentAircraft.pop_back();
    }

#if 0
    if (m_ui->mpBox->isChecked()) {
        std::string callSign = m_ui->mpCallsign->text().toStdString();
        if (!callSign.empty()) {
            opt->addOption("callsign", callSign);
        }

        QString host = m_ui->mpServerCombo->currentData().toString();
        int port = DEFAULT_MP_PORT;
        if (host == "custom") {
            QSettings settings;
            host = settings.value("mp-custom-host").toString();
        } else {
            port = findMPServerPort(host.toStdString());
        }

        if (port == 0) {
            port = DEFAULT_MP_PORT;
        }
        globals->get_props()->setStringValue("/sim/multiplay/txhost", host.toStdString());
        globals->get_props()->setIntValue("/sim/multiplay/txport", port);
    }
#endif

    m_ui->location->setLocationProperties();
    updateLocationHistory();

#if 0
    // time of day
    if (m_ui->timeOfDayCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("timeofday", dayval.toStdString());
    }

    if (m_ui->seasonCombo->currentIndex() != 0) {
        QString seasonName = m_ui->seasonCombo->currentText().toLower();
        opt->addOption("season", seasonName.toStdString());
    }
#endif

    QSettings settings;
    QString downloadDir = settings.value("download-dir").toString();
    if (!downloadDir.isEmpty()) {
        QDir d(downloadDir);
        if (!d.exists()) {
            int result = QMessageBox::question(this, tr("Create download folder?"),
                                  tr("The selected location for downloads does not exist. Create it?"),
                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (result == QMessageBox::Cancel) {
                return;
            }

            if (result == QMessageBox::Yes) {
                d.mkpath(downloadDir);
            }
        }

        opt->addOption("download-dir", downloadDir.toStdString());
    }

    // scenery paths
    Q_FOREACH(QString path, settings.value("scenery-paths").toStringList()) {
        opt->addOption("fg-scenery", path.toStdString());
    }

    // aircraft paths
    Q_FOREACH(QString path, settings.value("aircraft-paths").toStringList()) {
        // can't use fg-aircraft for this, as it is processed before the launcher is run
        globals->append_aircraft_path(path.toStdString());
    }

    // additional arguments
#if 0
    ArgumentsTokenizer tk;
    Q_FOREACH(ArgumentsTokenizer::Arg a, tk.tokenize(m_ui->commandLineArgs->toPlainText())) {
        if (a.arg.startsWith("prop:")) {
            QString v = a.arg.mid(5) + "=" + a.value;
            opt->addOption("prop", v.toStdString());
        } else {
            opt->addOption(a.arg.toStdString(), a.value.toStdString());
        }
    }
#endif

    if (settings.contains("restore-defaults-on-run")) {
        settings.remove("restore-defaults-on-run");
        opt->addOption("restore-defaults", "");
    }

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

void LauncherMainWindow::onUpdateAllAircraft()
{
    const PackageList& toBeUpdated = globals->packageRoot()->packagesNeedingUpdate();
    std::for_each(toBeUpdated.begin(), toBeUpdated.end(), [](PackageRef pkg) {
        globals->packageRoot()->scheduleToUpdate(pkg->install());
    });
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
#if 0
    if (m_ui->timeOfDayCombo->currentIndex() > 0) {
        summary.append(QString(m_ui->timeOfDayCombo->currentText().toLower()));
    }

    if (m_ui->seasonCombo->currentIndex() > 0) {
        summary.append(QString(m_ui->seasonCombo->currentText().toLower()));
    }

    if (m_ui->rembrandtCheckbox->isChecked()) {
        summary.append("Rembrandt enabled");
    } else if (m_ui->msaaCheckbox->isChecked()) {
        summary.append("anti-aliasing");
    }

    if (m_ui->fetchRealWxrCheckbox->isChecked()) {
        summary.append("live weather");
    }

    if (m_ui->terrasyncCheck->isChecked()) {
        summary.append("automatic scenery downloads");
    }

    if (m_ui->startPausedCheck->isChecked()) {
        summary.append("paused");
    }

    if (m_ui->mpBox->isChecked()) {
        summary.append(tr("multiplayer: %1").arg(m_ui->mpCallsign->text()));
    }

    QString s = summary.join(", ");
    s[0] = s[0].toUpper();
    m_ui->settingsDescription->setText(s);
#endif
}

void LauncherMainWindow::onRembrandtToggled(bool b)
{
    // Rembrandt and multi-sample are exclusive
  //  m_ui->msaaCheckbox->setEnabled(!b);
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
        AddOnsPage::addDefaultCatalog(this);
    }

    checkOfficialCatalogMessage();
}

void LauncherMainWindow::checkUpdateAircraft()
{
    if (shouldShowOfficialCatalogMessage()) {
        return; // don't interfere
    }

    const bool showUpdateMessage = !globals->packageRoot()->packagesNeedingUpdate().empty();
    m_aircraftModel->setMessageWidgetVisible(showUpdateMessage);
    if (showUpdateMessage) {
        UpdateAllAircraftMessage* messageWidget = new UpdateAllAircraftMessage;
       // connect(messageWidget, &UpdateAllAircraftMessage::linkActivated,
      //        this, &LauncherMainWindow::onMessageLink);
        connect(messageWidget, &UpdateAllAircraftMessage::updateAll, this, &LauncherMainWindow::onUpdateAllAircraft);
        QModelIndex index = m_aircraftProxy->mapFromSource(m_aircraftModel->messageWidgetIndex());
        m_ui->aircraftList->setIndexWidget(index, messageWidget);
    }
}

void LauncherMainWindow::onRefreshMPServers()
{
    if (m_mpServerRequest.get()) {
        return; // in-progress
    }

    string url(fgGetString("/sim/multiplay/serverlist-url",
                           "http://liveries.flightgear.org/mpstatus/mpservers.xml"));

    if (url.empty()) {
        SG_LOG(SG_IO, SG_ALERT, "do_multiplayer.refreshserverlist: no URL given");
        return;
    }

    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    m_mpServerRequest.reset(new RemoteXMLRequest(url, targetnode));
    m_mpServerRequest->done(this, &LauncherMainWindow::onRefreshMPServersDone);
    m_mpServerRequest->fail(this, &LauncherMainWindow::onRefreshMPServersFailed);
    globals->get_subsystem<FGHTTPClient>()->makeRequest(m_mpServerRequest);
}

void LauncherMainWindow::onRefreshMPServersDone(simgear::HTTP::Request*)
{
#if 0
    // parse the properties
    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    m_ui->mpServerCombo->clear();

    for (int i=0; i<targetnode->nChildren(); ++i) {
        SGPropertyNode* c = targetnode->getChild(i);
        if (c->getName() != std::string("server")) {
            continue;
        }

        if (c->getBoolValue("online") != true) {
            // only list online servers
            continue;
        }

        QString name = QString::fromStdString(c->getStringValue("name"));
        QString loc = QString::fromStdString(c->getStringValue("location"));
        QString host = QString::fromStdString(c->getStringValue("hostname"));
        m_ui->mpServerCombo->addItem(tr("%1 - %2").arg(name,loc), host);
    }

    EditCustomMPServerDialog::addCustomItem(m_ui->mpServerCombo);
    restoreMPServerSelection();
#endif
    m_mpServerRequest.clear();
}

void LauncherMainWindow::onRefreshMPServersFailed(simgear::HTTP::Request*)
{
    qWarning() << "refreshing MP servers failed:" << QString::fromStdString(m_mpServerRequest->responseReason());
    m_mpServerRequest.clear();
#if 0
    EditCustomMPServerDialog::addCustomItem(m_ui->mpServerCombo);
    restoreMPServerSelection();
#endif
}

void LauncherMainWindow::restoreMPServerSelection()
{
#if 0
    if (m_doRestoreMPServer) {
        QSettings settings;
        int index = m_ui->mpServerCombo->findData(settings.value("mp-server"));
        if (index >= 0) {
            m_ui->mpServerCombo->setCurrentIndex(index);
        }
        m_doRestoreMPServer = false;
    }
#endif
}

void LauncherMainWindow::onMPServerActivated(int index)
{
#if 0
    if (m_ui->mpServerCombo->itemData(index) == "custom") {
        EditCustomMPServerDialog dlg(this);
        dlg.exec();
        if (dlg.result() == QDialog::Accepted) {
            m_ui->mpServerCombo->setItemText(index, tr("Custom - %1").arg(dlg.hostname()));
        }
    }
#endif
}

int LauncherMainWindow::findMPServerPort(const std::string& host)
{
    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    for (int i=0; i<targetnode->nChildren(); ++i) {
        SGPropertyNode* c = targetnode->getChild(i);
        if (c->getName() != std::string("server")) {
            continue;
        }

        if (c->getStringValue("hostname") == host) {
            return c->getIntValue("port");
        }
    }

    return 0;
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
