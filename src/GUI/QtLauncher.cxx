// QtLauncher.cxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started December 2014.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "QtLauncher.hxx"
#include "QtLauncher_private.hxx"

#include <locale.h>

// Qt
#include <QProgressDialog>
#include <QCoreApplication>
#include <QAbstractListModel>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <QCompleter>
#include <QListView>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QApplication>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProcess>

// Simgear
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>

#include "ui_Launcher.h"
#include "ui_NoOfficialHangar.h"

#include "EditRatingsFilterDialog.hxx"
#include "AircraftItemDelegate.hxx"
#include "AircraftModel.hxx"
#include "PathsDialog.hxx"
#include "EditCustomMPServerDialog.hxx"

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/SHPParser.hxx>

#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Network/HTTPClient.hxx>
#include <Network/RemoteXMLRequest.hxx>

using namespace flightgear;
using namespace simgear::pkg;

const int MAX_RECENT_AIRCRAFT = 20;

namespace { // anonymous namespace

void initNavCache()
{
    QString baseLabel = QT_TR_NOOP("Initialising navigation data, this may take several minutes");
    NavDataCache* cache = NavDataCache::createInstance();
    if (cache->isRebuildRequired()) {
        QProgressDialog rebuildProgress(baseLabel,
                                       QString() /* cancel text */,
                                       0, 100, Q_NULLPTR,
                                       Qt::Dialog
                                           | Qt::CustomizeWindowHint
                                           | Qt::WindowTitleHint
                                           | Qt::WindowSystemMenuHint
                                           | Qt::MSWindowsFixedSizeDialogHint);
        rebuildProgress.setWindowModality(Qt::WindowModal);
        rebuildProgress.show();

        NavDataCache::RebuildPhase phase = cache->rebuild();

        while (phase != NavDataCache::REBUILD_DONE) {
            // sleep to give the rebuild thread more time
            SGTimeStamp::sleepForMSec(50);
            phase = cache->rebuild();

            switch (phase) {
            case NavDataCache::REBUILD_AIRPORTS:
                rebuildProgress.setLabelText(QT_TR_NOOP("Loading airport data"));
                break;

            case NavDataCache::REBUILD_FIXES:
                rebuildProgress.setLabelText(QT_TR_NOOP("Loading waypoint data"));
                break;

            case NavDataCache::REBUILD_NAVAIDS:
                rebuildProgress.setLabelText(QT_TR_NOOP("Loading navigation data"));
                break;


            case NavDataCache::REBUILD_POIS:
                rebuildProgress.setLabelText(QT_TR_NOOP("Loading point-of-interest data"));
                break;

            default:
                rebuildProgress.setLabelText(baseLabel);
            }

            if (phase == NavDataCache::REBUILD_UNKNOWN) {
                rebuildProgress.setValue(0);
                rebuildProgress.setMaximum(0);
            } else {
                rebuildProgress.setValue(cache->rebuildPhaseCompletionPercentage());
                rebuildProgress.setMaximum(100);
            }

            QCoreApplication::processEvents();
        }
    }
}

class ArgumentsTokenizer
{
public:
    class Arg
    {
    public:
        explicit Arg(QString k, QString v = QString()) : arg(k), value(v) {}

        QString arg;
        QString value;
    };

    QList<Arg> tokenize(QString in) const
    {
        int index = 0;
        const int len = in.count();
        QChar c, nc;
        State state = Start;
        QString key, value;
        QList<Arg> result;

        for (; index < len; ++index) {
            c = in.at(index);
            nc = index < (len - 1) ? in.at(index + 1) : QChar();

            switch (state) {
            case Start:
                if (c == QChar('-')) {
                    if (nc == QChar('-')) {
                        state = Key;
                        key.clear();
                        ++index;
                    } else {
                        // should we pemit single hyphen arguments?
                        // choosing to fail for now
                        return QList<Arg>();
                    }
                } else if (c == QChar('#')) {
                    state = Comment;
                    break;
                } else if (c.isSpace()) {
                    break;
                }
                break;

            case Key:
                if (c == QChar('=')) {
                    state = Value;
                    value.clear();
                } else if (c.isSpace()) {
                    state = Start;
                    result.append(Arg(key));
                } else {
                    // could check for illegal charatcers here
                    key.append(c);
                }
                break;

            case Value:
                if (c == QChar('"')) {
                    state = Quoted;
                } else if (c.isSpace()) {
                    state = Start;
                    result.append(Arg(key, value));
                } else {
                    value.append(c);
                }
                break;

            case Quoted:
                if (c == QChar('\\')) {
                    // check for escaped double-quote inside quoted value
                    if (nc == QChar('"')) {
                        ++index;
                    }
                } else if (c == QChar('"')) {
                    state = Value;
                } else {
                    value.append(c);
                }
                break;

            case Comment:
                if ((c == QChar('\n')) || (c == QChar('\r'))) {
                    state = Start;
                    break;
                } else {
                    // nothing to do, eat comment chars
                }
                break;
            } // of state switch
        } // of character loop

        // ensure last argument isn't lost
        if (state == Key) {
            result.append(Arg(key));
        } else if (state == Value) {
            result.append(Arg(key, value));
        }

        return result;
    }

private:
    enum State {
        Start = 0,
        Key,
        Value,
        Quoted,
        Comment
    };
};

} // of anonymous namespace

class AircraftProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AircraftProxyModel(QObject* pr) :
        QSortFilterProxyModel(pr),
        m_ratingsFilter(true),
        m_onlyShowInstalled(false)
    {
        for (int i=0; i<4; ++i) {
            m_ratings[i] = 3;
        }
    }

    void setRatings(int* ratings)
    {
        ::memcpy(m_ratings, ratings, sizeof(int) * 4);
        invalidate();
    }

public slots:
    void setRatingFilterEnabled(bool e)
    {
        if (e == m_ratingsFilter) {
            return;
        }

        m_ratingsFilter = e;
        invalidate();
    }

    void setInstalledFilterEnabled(bool e)
    {
        if (e == m_onlyShowInstalled) {
            return;
        }

        m_onlyShowInstalled = e;
        invalidate();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant v = index.data(AircraftPackageStatusRole);
        AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
        if (status == NoOfficialCatalogMessage) {
            return true;
        }

        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
            return false;
        }

        if (m_onlyShowInstalled) {
            QVariant v = index.data(AircraftPackageStatusRole);
            AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
            if (status == PackageNotInstalled) {
                return false;
            }
        }

        if (!m_onlyShowInstalled && m_ratingsFilter) {
            for (int i=0; i<4; ++i) {
                if (m_ratings[i] > index.data(AircraftRatingRole + i).toInt()) {
                    return false;
                }
            }
        }

        return true;
    }

private:
    bool m_ratingsFilter;
    bool m_onlyShowInstalled;
    int m_ratings[4];
};

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

static void initQtResources()
{
    Q_INIT_RESOURCE(resources);
}

namespace flightgear
{

void initApp(int& argc, char** argv)
{
    static bool qtInitDone = false;
    static int s_argc;

    if (!qtInitDone) {
        qtInitDone = true;

        sglog().setLogLevels( SG_ALL, SG_INFO );
        initQtResources(); // can't be called from a namespace

        s_argc = argc; // QApplication only stores a reference to argc,
        // and may crash if it is freed
        // http://doc.qt.io/qt-5/qguiapplication.html#QGuiApplication

        QApplication* app = new QApplication(s_argc, argv);
        app->setOrganizationName("FlightGear");
        app->setApplicationName("FlightGear");
        app->setOrganizationDomain("flightgear.org");

        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           QString::fromStdString(globals->get_fg_home().utf8Str()));

        // reset numeric / collation locales as described at:
        // http://doc.qt.io/qt-5/qcoreapplication.html#details
        ::setlocale(LC_NUMERIC, "C");
        ::setlocale(LC_COLLATE, "C");

        Qt::KeyboardModifiers mods = app->queryKeyboardModifiers();
        if (mods & (Qt::AltModifier | Qt::ShiftModifier)) {
            qWarning() << "Alt/shift pressed during launch";
            QSettings settings;
            settings.setValue("fg-root", "!ask");
        }
    }
}

void loadNaturalEarthFile(const std::string& aFileName,
                          flightgear::PolyLine::Type aType,
                          bool areClosed)
{
    SGPath path(globals->get_fg_root());
    path.append( "Geodata" );
    path.append(aFileName);
    if (!path.exists())
        return; // silently fail for now

    flightgear::PolyLineList lines;
    flightgear::SHPParser::parsePolyLines(path, aType, lines, areClosed);
    flightgear::PolyLine::bulkAddToSpatialIndex(lines);
}

void loadNaturalEarthData()
{
    SGTimeStamp st;
    st.stamp();

    loadNaturalEarthFile("ne_10m_coastline.shp", flightgear::PolyLine::COASTLINE, false);
    loadNaturalEarthFile("ne_10m_rivers_lake_centerlines.shp", flightgear::PolyLine::RIVER, false);
    loadNaturalEarthFile("ne_10m_lakes.shp", flightgear::PolyLine::LAKE, true);

    qDebug() << "load basic data took" << st.elapsedMSec();


    st.stamp();
    loadNaturalEarthFile("ne_10m_urban_areas.shp", flightgear::PolyLine::URBAN, true);

    qDebug() << "loading urban areas took:" << st.elapsedMSec();
}

bool runLauncherDialog()
{
    // startup the nav-cache now. This pre-empts normal startup of
    // the cache, but no harm done. (Providing scenery paths are consistent)

    initNavCache();

    QSettings settings;
    QString downloadDir = settings.value("download-dir").toString();
    if (!downloadDir.isEmpty()) {
        flightgear::Options::sharedInstance()->setOption("download-dir", downloadDir.toStdString());
    }

    fgInitPackageRoot();

    // startup the HTTP system now since packages needs it
    FGHTTPClient* http = globals->add_new_subsystem<FGHTTPClient>();

    // we guard against re-init in the global phase; bind and postinit
    // will happen as normal
    http->init();

    loadNaturalEarthData();

    // avoid double Apple menu and other weirdness if both Qt and OSG
    // try to initialise various Cocoa structures.
    flightgear::WindowBuilder::setPoseAsStandaloneApp(false);

    QtLauncher dlg;
    dlg.show();

    int appResult = qApp->exec();
    if (appResult < 0) {
        return false; // quit
    }

    // don't set scenery paths twice
    globals->clear_fg_scenery();

    return true;
}

bool runInAppLauncherDialog()
{
    QtLauncher dlg;
    dlg.setInAppMode();
    dlg.exec();
    if (dlg.result() != QDialog::Accepted) {
        return false;
    }

    return true;
}

} // of namespace flightgear

QtLauncher::QtLauncher() :
    QDialog(),
    m_ui(NULL),
    m_subsystemIdleTimer(NULL),
    m_inAppMode(false),
    m_doRestoreMPServer(false)
{
    m_ui.reset(new Ui::Launcher);
    m_ui->setupUi(this);

#if QT_VERSION >= 0x050300
    // don't require Qt 5.3
    m_ui->commandLineArgs->setPlaceholderText("--option=value --prop:/sim/name=value");
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
            this, &QtLauncher::onSubsytemIdleTimeout);
    m_subsystemIdleTimer->start();

    // create and configure the proxy model
    m_aircraftProxy = new AircraftProxyModel(this);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setRatingFilterEnabled);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            this, &QtLauncher::maybeRestoreAircraftSelection);

    connect(m_ui->onlyShowInstalledCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setInstalledFilterEnabled);
    connect(m_ui->aircraftFilter, &QLineEdit::textChanged,
            m_aircraftProxy, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));

    connect(m_ui->aircraftHistory, &QPushButton::clicked,
          this, &QtLauncher::onPopupAircraftHistory);

    connect(m_ui->location, &LocationWidget::descriptionChanged,
            m_ui->locationDescription, &QLabel::setText);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &QtLauncher::onQuit);
    addAction(qa);

    connect(m_ui->editRatingFilter, &QPushButton::clicked,
            this, &QtLauncher::onEditRatingsFilter);
    connect(m_ui->onlyShowInstalledCheck, &QCheckBox::toggled,
            this, &QtLauncher::onShowInstalledAircraftToggled);

    QIcon historyIcon(":/history-icon");
    m_ui->aircraftHistory->setIcon(historyIcon);

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

    connect(m_ui->rembrandtCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(onRembrandtToggled(bool)));
    connect(m_ui->terrasyncCheck, &QCheckBox::toggled,
            this, &QtLauncher::onToggleTerrasync);
    updateSettingsSummary();

    connect(m_ui->mpServerCombo, SIGNAL(activated(int)),
            this, SLOT(onMPServerActivated(int)));

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
            this, &QtLauncher::onAircraftSelected);
    connect(delegate, &AircraftItemDelegate::variantChanged,
            this, &QtLauncher::onAircraftSelected);
    connect(delegate, &AircraftItemDelegate::requestInstall,
            this, &QtLauncher::onRequestPackageInstall);
    connect(delegate, &AircraftItemDelegate::cancelDownload,
            this, &QtLauncher::onCancelDownload);

    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallCompleted,
            this, &QtLauncher::onAircraftInstalledCompleted);
    connect(m_aircraftModel, &AircraftItemModel::aircraftInstallFailed,
            this, &QtLauncher::onAircraftInstallFailed);
    connect(m_aircraftModel, &AircraftItemModel::scanCompleted,
            this, &QtLauncher::updateSelectedAircraft);
    connect(m_ui->restoreDefaultsButton, &QPushButton::clicked,
            this, &QtLauncher::onRestoreDefaults);


    AddOnsPage* addOnsPage = new AddOnsPage(NULL, globals->packageRoot());
    connect(addOnsPage, &AddOnsPage::downloadDirChanged,
            this, &QtLauncher::onDownloadDirChanged);
    connect(addOnsPage, &AddOnsPage::sceneryPathsChanged,
            this, &QtLauncher::setSceneryPaths);

    m_ui->tabWidget->addTab(addOnsPage, tr("Add-ons"));
    // after any kind of reset, try to restore selection and scroll
    // to match the m_selectedAircraft. This needs to be delayed
    // fractionally otherwise the scrollTo seems to be ignored,
    // unfortunately.
    connect(m_aircraftProxy, &AircraftProxyModel::modelReset,
            this, &QtLauncher::delayedAircraftModelReset);

    QSettings settings;
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->setPackageRoot(globals->packageRoot());
    m_aircraftModel->scanDirs();

    checkOfficialCatalogMessage();
    restoreSettings();

    onRefreshMPServers();
}

QtLauncher::~QtLauncher()
{

}

void QtLauncher::setSceneryPaths()
{
    globals->clear_fg_scenery();

// mimic what optionss.cxx does, so we can find airport data for parking
// positions
    QSettings settings;
    // append explicit scenery paths
    Q_FOREACH(QString path, settings.value("scenery-paths").toStringList()) {
        globals->append_fg_scenery(path.toStdString());
    }

    // append the TerraSync path
    QString downloadDir = settings.value("download-dir").toString();
    if (downloadDir.isEmpty()) {
        downloadDir = QString::fromStdString(flightgear::defaultDownloadDir().utf8Str());
    }

    SGPath terraSyncDir(downloadDir.toStdString());
    terraSyncDir.append("TerraSync");
    if (terraSyncDir.exists()) {
        globals->append_fg_scenery(terraSyncDir);
    }

}

void QtLauncher::setInAppMode()
{
  m_inAppMode = true;
  m_ui->tabWidget->removeTab(2);
  m_ui->tabWidget->removeTab(2);

  m_ui->runButton->setText(tr("Apply"));
  m_ui->quitButton->setText(tr("Cancel"));

  disconnect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
  connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onApply()));
}

void QtLauncher::restoreSettings()
{
    QSettings settings;
    m_ui->rembrandtCheckbox->setChecked(settings.value("enable-rembrandt", false).toBool());
    m_ui->terrasyncCheck->setChecked(settings.value("enable-terrasync", true).toBool());
    m_ui->fullScreenCheckbox->setChecked(settings.value("start-fullscreen", false).toBool());
    m_ui->msaaCheckbox->setChecked(settings.value("enable-msaa", false).toBool());
    m_ui->fetchRealWxrCheckbox->setChecked(settings.value("enable-realwx", true).toBool());
    m_ui->startPausedCheck->setChecked(settings.value("start-paused", false).toBool());
    m_ui->timeOfDayCombo->setCurrentIndex(settings.value("timeofday", 0).toInt());
    m_ui->seasonCombo->setCurrentIndex(settings.value("season", 0).toInt());

    // full paths to -set.xml files
    m_recentAircraft = QUrl::fromStringList(settings.value("recent-aircraft").toStringList());

    if (!m_recentAircraft.empty()) {
        m_selectedAircraft = m_recentAircraft.front();
    } else {
        // select the default C172p
    }

    if (!m_inAppMode) {
        setSceneryPaths();
    }

    m_ui->location->restoreSettings();

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

    m_ui->commandLineArgs->setPlainText(settings.value("additional-args").toString());

    m_ui->mpBox->setChecked(settings.value("mp-enabled").toBool());
    m_ui->mpCallsign->setText(settings.value("mp-callsign").toString());
    // don't restore MP server here, we do it after a refresh
    m_doRestoreMPServer = true;
}

void QtLauncher::delayedAircraftModelReset()
{
    QTimer::singleShot(1, this, SLOT(maybeRestoreAircraftSelection()));
}

void QtLauncher::maybeRestoreAircraftSelection()
{
    QModelIndex aircraftIndex = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    QModelIndex proxyIndex = m_aircraftProxy->mapFromSource(aircraftIndex);
    if (proxyIndex.isValid()) {
        m_ui->aircraftList->selectionModel()->setCurrentIndex(proxyIndex,
                                                              QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftList->selectionModel()->select(proxyIndex,
                                                     QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftList->scrollTo(proxyIndex);
    }
}

void QtLauncher::saveSettings()
{
    QSettings settings;
    settings.setValue("enable-rembrandt", m_ui->rembrandtCheckbox->isChecked());
    settings.setValue("enable-terrasync", m_ui->terrasyncCheck->isChecked());
    settings.setValue("enable-msaa", m_ui->msaaCheckbox->isChecked());
    settings.setValue("start-fullscreen", m_ui->fullScreenCheckbox->isChecked());
    settings.setValue("enable-realwx", m_ui->fetchRealWxrCheckbox->isChecked());
    settings.setValue("start-paused", m_ui->startPausedCheck->isChecked());
    settings.setValue("ratings-filter", m_ui->ratingsFilterCheck->isChecked());
    settings.setValue("only-show-installed", m_ui->onlyShowInstalledCheck->isChecked());
    settings.setValue("recent-aircraft", QUrl::toStringList(m_recentAircraft));

    settings.setValue("timeofday", m_ui->timeOfDayCombo->currentIndex());
    settings.setValue("season", m_ui->seasonCombo->currentIndex());
    settings.setValue("additional-args", m_ui->commandLineArgs->toPlainText());

    m_ui->location->saveSettings();

    settings.setValue("mp-callsign", m_ui->mpCallsign->text());
    settings.setValue("mp-server", m_ui->mpServerCombo->currentData());
    settings.setValue("mp-enabled", m_ui->mpBox->isChecked());
}

void QtLauncher::setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
    std::string stdName(name.toStdString());
    if (cbox->isChecked()) {
        opt->addOption("enable-" + stdName, "");
    } else {
        opt->addOption("disable-" + stdName, "");
    }
}

void QtLauncher::closeEvent(QCloseEvent *event)
{
    qApp->exit(-1);
}

void QtLauncher::reject()
{
    qApp->exit(-1);
}

void QtLauncher::onRun()
{
    flightgear::Options* opt = flightgear::Options::sharedInstance();
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

    // MSAA is more complex
    if (!m_ui->rembrandtCheckbox->isChecked()) {
        if (m_ui->msaaCheckbox->isChecked()) {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 1);
            globals->get_props()->setIntValue("/sim/rendering/multi-samples", 4);
        } else {
            globals->get_props()->setIntValue("/sim/rendering/multi-sample-buffers", 0);
        }
    }

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

    if (m_ui->mpBox->isChecked()) {
        opt->addOption("callsign", m_ui->mpCallsign->text().toStdString());
        QString host = m_ui->mpServerCombo->currentData().toString();
        int port = 5000;
        if (host == "custom") {
            QSettings settings;
            host = settings.value("mp-custom-host").toString();
            port = settings.value("mp-custom-port").toInt();
        } else {
            port = findMPServerPort(host.toStdString());
        }
        globals->get_props()->setStringValue("/sim/multiplay/txhost", host.toStdString());
        globals->get_props()->setIntValue("/sim/multiplay/txport", port);
    }

    m_ui->location->setLocationOptions();

    // time of day
    if (m_ui->timeOfDayCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("timeofday", dayval.toStdString());
    }

    if (m_ui->seasonCombo->currentIndex() != 0) {
        QString seasonName = m_ui->seasonCombo->currentText().toLower();
        opt->addOption("season", seasonName.toStdString());
    }

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
    ArgumentsTokenizer tk;
    Q_FOREACH(ArgumentsTokenizer::Arg a, tk.tokenize(m_ui->commandLineArgs->toPlainText())) {
        if (a.arg.startsWith("prop:")) {
            QString v = a.arg.mid(5) + "=" + a.value;
            opt->addOption("prop", v.toStdString());
        } else {
            opt->addOption(a.arg.toStdString(), a.value.toStdString());
        }
    }

    if (settings.contains("restore-defaults-on-run")) {
        settings.remove("restore-defaults-on-run");
        opt->addOption("restore-defaults", "");
    }

    saveSettings();

    qApp->exit(0);
}


void QtLauncher::onApply()
{
    accept();

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


    saveSettings();
}

void QtLauncher::onQuit()
{
    qApp->exit(-1);
}

void QtLauncher::onToggleTerrasync(bool enabled)
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
                m_ui->terrasyncCheck->setChecked(false);
                return;
            }

            QDir d(downloadDir);
            d.mkpath(downloadDir);
        }
    } // of is enabled
}

void QtLauncher::onAircraftInstalledCompleted(QModelIndex index)
{
    maybeUpdateSelectedAircraft(index);
}

void QtLauncher::onRatingsFilterToggled()
{
    QModelIndex aircraftIndex = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    QModelIndex proxyIndex = m_aircraftProxy->mapFromSource(aircraftIndex);
    if (proxyIndex.isValid()) {
        m_ui->aircraftList->scrollTo(proxyIndex);
    }
}

void QtLauncher::onAircraftInstallFailed(QModelIndex index, QString errorMessage)
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

void QtLauncher::onAircraftSelected(const QModelIndex& index)
{
    m_selectedAircraft = index.data(AircraftURIRole).toUrl();
    updateSelectedAircraft();
}

void QtLauncher::onRequestPackageInstall(const QModelIndex& index)
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

void QtLauncher::onCancelDownload(const QModelIndex& index)
{
    QString pkg = index.data(AircraftPackageIdRole).toString();
    simgear::pkg::PackageRef pref = globals->packageRoot()->getPackageById(pkg.toStdString());
    simgear::pkg::InstallRef i = pref->existingInstall();
    i->cancelDownload();
}

void QtLauncher::onRestoreDefaults()
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

    restartTheApp(QStringList());
}

void QtLauncher::maybeUpdateSelectedAircraft(QModelIndex index)
{
    QUrl u = index.data(AircraftURIRole).toUrl();
    if (u == m_selectedAircraft) {
        // potentially enable the run button now!
        updateSelectedAircraft();
    }
}

void QtLauncher::updateSelectedAircraft()
{
    QModelIndex index = m_aircraftModel->indexOfAircraftURI(m_selectedAircraft);
    if (index.isValid()) {
        QPixmap pm = index.data(Qt::DecorationRole).value<QPixmap>();
        m_ui->thumbnail->setPixmap(pm);
        m_ui->aircraftDescription->setText(index.data(Qt::DisplayRole).toString());

        int status = index.data(AircraftPackageStatusRole).toInt();
        bool canRun = (status == PackageInstalled);
        m_ui->runButton->setEnabled(canRun);

        LauncherAircraftType aircraftType = Airplane;
        if (index.data(AircraftIsHelicopterRole).toBool()) {
            aircraftType = Helicopter;
        } else if (index.data(AircraftIsSeaplaneRole).toBool()) {
            aircraftType = Seaplane;
        }

        m_ui->location->setAircraftType(aircraftType);
    } else {
        m_ui->thumbnail->setPixmap(QPixmap());
        m_ui->aircraftDescription->setText("");
        m_ui->runButton->setEnabled(false);
    }
}

QModelIndex QtLauncher::proxyIndexForAircraftURI(QUrl uri) const
{
  return m_aircraftProxy->mapFromSource(sourceIndexForAircraftURI(uri));
}

QModelIndex QtLauncher::sourceIndexForAircraftURI(QUrl uri) const
{
    AircraftItemModel* sourceModel = qobject_cast<AircraftItemModel*>(m_aircraftProxy->sourceModel());
    Q_ASSERT(sourceModel);
    return sourceModel->indexOfAircraftURI(uri);
}

void QtLauncher::onPopupAircraftHistory()
{
    if (m_recentAircraft.isEmpty()) {
        return;
    }

    QMenu m;
    Q_FOREACH(QUrl uri, m_recentAircraft) {
        QModelIndex index = sourceIndexForAircraftURI(uri);
        if (!index.isValid()) {
            // not scanned yet
            continue;
        }
        QAction* act = m.addAction(index.data(Qt::DisplayRole).toString());
        act->setData(uri);
    }

    QPoint popupPos = m_ui->aircraftHistory->mapToGlobal(m_ui->aircraftHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        m_selectedAircraft = triggered->data().toUrl();
        QModelIndex index = proxyIndexForAircraftURI(m_selectedAircraft);
        m_ui->aircraftList->selectionModel()->setCurrentIndex(index,
                                                              QItemSelectionModel::ClearAndSelect);
        m_ui->aircraftFilter->clear();
        updateSelectedAircraft();
    }
}

void QtLauncher::onEditRatingsFilter()
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

void QtLauncher::updateSettingsSummary()
{
    QStringList summary;
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

    QString s = summary.join(", ");
    s[0] = s[0].toUpper();
    m_ui->settingsDescription->setText(s);
}

void QtLauncher::onRembrandtToggled(bool b)
{
    // Rembrandt and multi-sample are exclusive
    m_ui->msaaCheckbox->setEnabled(!b);
}

void QtLauncher::onShowInstalledAircraftToggled(bool b)
{
    m_ui->ratingsFilterCheck->setEnabled(!b);
    maybeRestoreAircraftSelection();
}

void QtLauncher::onSubsytemIdleTimeout()
{
    globals->get_subsystem_mgr()->update(0.0);
}

void QtLauncher::onDownloadDirChanged()
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

void QtLauncher::checkOfficialCatalogMessage()
{
    QSettings settings;
    bool showOfficialCatalogMesssage = !globals->get_subsystem<FGHTTPClient>()->isDefaultCatalogInstalled();
    if (settings.value("hide-official-catalog-message").toBool()) {
        showOfficialCatalogMesssage = false;
    }

    m_aircraftModel->setOfficialHangarMessageVisible(showOfficialCatalogMesssage);
    if (showOfficialCatalogMesssage) {
        NoOfficialHangarMessage* messageWidget = new NoOfficialHangarMessage;
        connect(messageWidget, &NoOfficialHangarMessage::linkActivated,
                this, &QtLauncher::onOfficialCatalogMessageLink);

        QModelIndex index = m_aircraftProxy->mapFromSource(m_aircraftModel->officialHangarMessageIndex());
        m_ui->aircraftList->setIndexWidget(index, messageWidget);
    }
}

void QtLauncher::onOfficialCatalogMessageLink(QUrl link)
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

void QtLauncher::onRefreshMPServers()
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
    m_mpServerRequest->done(this, &QtLauncher::onRefreshMPServersDone);
    m_mpServerRequest->fail(this, &QtLauncher::onRefreshMPServersFailed);
    globals->get_subsystem<FGHTTPClient>()->makeRequest(m_mpServerRequest);
}

void QtLauncher::onRefreshMPServersDone(simgear::HTTP::Request*)
{
    // parse the properties
    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    m_ui->mpServerCombo->clear();

    for (int i=0; i<targetnode->nChildren(); ++i) {
        SGPropertyNode* c = targetnode->getChild(i);
        if (c->getName() != std::string("server")) {
            continue;
        }

        QString name = QString::fromStdString(c->getStringValue("name"));
        QString loc = QString::fromStdString(c->getStringValue("location"));
        QString host = QString::fromStdString(c->getStringValue("hostname"));
        m_ui->mpServerCombo->addItem(tr("%1 - %2").arg(name,loc), host);
    }

    EditCustomMPServerDialog::addCustomItem(m_ui->mpServerCombo);
    restoreMPServerSelection();

    m_mpServerRequest.clear();
}

void QtLauncher::onRefreshMPServersFailed(simgear::HTTP::Request*)
{
    qWarning() << "refreshing MP servers failed:" << QString::fromStdString(m_mpServerRequest->responseReason());
    m_mpServerRequest.clear();
    EditCustomMPServerDialog::addCustomItem(m_ui->mpServerCombo);
    restoreMPServerSelection();
}

void QtLauncher::restoreMPServerSelection()
{
    if (m_doRestoreMPServer) {
        QSettings settings;
        int index = m_ui->mpServerCombo->findData(settings.value("mp-server"));
        if (index >= 0) {
            m_ui->mpServerCombo->setCurrentIndex(index);
        }
        m_doRestoreMPServer = false;
    }
}

void QtLauncher::onMPServerActivated(int index)
{
    if (m_ui->mpServerCombo->itemData(index) == "custom") {
        EditCustomMPServerDialog dlg(this);
        dlg.exec();
        if (dlg.result() == QDialog::Accepted) {
            m_ui->mpServerCombo->setItemText(index, tr("Custom - %1").arg(dlg.hostname()));
        }
    }
}

int QtLauncher::findMPServerPort(const std::string& host)
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

simgear::pkg::PackageRef QtLauncher::packageForAircraftURI(QUrl uri) const
{
    if (uri.scheme() != "package") {
        qWarning() << "invalid URL scheme:" << uri;
        return simgear::pkg::PackageRef();
    }

    QString ident = uri.path();
    return globals->packageRoot()->getPackageById(ident.toStdString());
}

void QtLauncher::restartTheApp(QStringList fgArgs)
{
    // Spawn a new instance of myApplication:
    QProcess proc;
    QStringList args;

#if defined(Q_OS_MAC)
    QDir dir(qApp->applicationDirPath()); // returns the 'MacOS' dir
    dir.cdUp(); // up to 'contents' dir
    dir.cdUp(); // up to .app dir
    // see 'man open' for details, but '-n' ensures we launch a new instance,
    // and we want to pass remaining arguments to us, not open.
    args << "-n" << dir.absolutePath() << "--args" << "--launcher" << fgArgs;
    qDebug() << "args" << args;
    proc.startDetached("open", args);
#else
    args << "--launcher" << fgArgs;
    proc.startDetached(qApp->applicationFilePath(), args);
#endif
    qApp->exit(-1);
}

#include "QtLauncher.moc"
