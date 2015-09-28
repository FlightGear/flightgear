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
#include "EditRatingsFilterDialog.hxx"
#include "AircraftItemDelegate.hxx"
#include "AircraftModel.hxx"
#include "PathsDialog.hxx"

#include <Main/globals.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx> // for parking
#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Network/HTTPClient.hxx>

using namespace flightgear;
using namespace simgear::pkg;

const int MAX_RECENT_AIRPORTS = 32;
const int MAX_RECENT_AIRCRAFT = 20;

namespace { // anonymous namespace

void initNavCache()
{
    QString baseLabel = QT_TR_NOOP("Initialising navigation data, this may take several minutes");
    NavDataCache* cache = NavDataCache::createInstance();
    if (cache->isRebuildRequired()) {
        QProgressDialog rebuildProgress(baseLabel,
                                       QString() /* cancel text */,
                                       0, 100);
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
        Quoted
    };
};

} // of anonymous namespace

class AirportSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AirportSearchModel() :
        m_searchActive(false)
    {
    }

    void setSearch(QString t)
    {
        beginResetModel();

        m_airports.clear();
        m_ids.clear();

        std::string term(t.toUpper().toStdString());
        // try ICAO lookup first
        FGAirportRef ref = FGAirport::findByIdent(term);
        if (ref) {
            m_ids.push_back(ref->guid());
            m_airports.push_back(ref);
        } else {
            m_search.reset(new NavDataCache::ThreadedAirportSearch(term));
            QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
            m_searchActive = true;
        }

        endResetModel();
    }

    bool isSearchActive() const
    {
        return m_searchActive;
    }

    virtual int rowCount(const QModelIndex&) const
    {
        // if empty, return 1 for special 'no matches'?
        return m_ids.size();
    }

    virtual QVariant data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();
        
        FGAirportRef apt = m_airports[index.row()];
        if (!apt.valid()) {
            apt = FGPositioned::loadById<FGAirport>(m_ids[index.row()]);
            m_airports[index.row()] = apt;
        }

        if (role == Qt::DisplayRole) {
            QString name = QString::fromStdString(apt->name());
            return QString("%1: %2").arg(QString::fromStdString(apt->ident())).arg(name);
        }

        if (role == Qt::EditRole) {
            return QString::fromStdString(apt->ident());
        }

        if (role == Qt::UserRole) {
            return static_cast<qlonglong>(m_ids[index.row()]);
        }

        return QVariant();
    }

    QString firstIdent() const
    {
        if (m_ids.empty())
            return QString();

        if (!m_airports.front().valid()) {
            m_airports[0] = FGPositioned::loadById<FGAirport>(m_ids.front());
        }

        return QString::fromStdString(m_airports.front()->ident());
    }

Q_SIGNALS:
    void searchComplete();

private slots:
    void onSearchResultsPoll()
    {
        PositionedIDVec newIds = m_search->results();
        
        beginInsertRows(QModelIndex(), m_ids.size(), newIds.size() - 1);
        for (unsigned int i=m_ids.size(); i < newIds.size(); ++i) {
            m_ids.push_back(newIds[i]);
            m_airports.push_back(FGAirportRef()); // null ref
        }
        endInsertRows();

        if (m_search->isComplete()) {
            m_searchActive = false;
            m_search.reset();
            emit searchComplete();
        } else {
            QTimer::singleShot(100, this, SLOT(onSearchResultsPoll()));
        }
    }

private:
    PositionedIDVec m_ids;
    mutable std::vector<FGAirportRef> m_airports;
    bool m_searchActive;
    QScopedPointer<NavDataCache::ThreadedAirportSearch> m_search;
};

class AircraftProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    AircraftProxyModel(QObject* pr) :
        QSortFilterProxyModel(pr),
        m_ratingsFilter(true)
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

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
            return false;
        }

        if (m_ratingsFilter) {
            QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
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
    int m_ratings[4];
};

QtLauncher::QtLauncher() :
    QDialog(),
    m_ui(NULL)
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

    m_airportsModel = new AirportSearchModel;
    m_ui->searchList->setModel(m_airportsModel);
    connect(m_ui->searchList, &QListView::clicked,
            this, &QtLauncher::onAirportChoiceSelected);
    connect(m_airportsModel, &AirportSearchModel::searchComplete,
            this, &QtLauncher::onAirportSearchComplete);

    // create and configure the proxy model
    m_aircraftProxy = new AircraftProxyModel(this);
    connect(m_ui->ratingsFilterCheck, &QAbstractButton::toggled,
            m_aircraftProxy, &AircraftProxyModel::setRatingFilterEnabled);
    connect(m_ui->aircraftFilter, &QLineEdit::textChanged,
            m_aircraftProxy, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_ui->runwayCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->parkingCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->runwayRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->parkingRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));
    connect(m_ui->onFinalCheckbox, SIGNAL(toggled(bool)),
            this, SLOT(updateAirportDescription()));

    
    connect(m_ui->airportDiagram, &AirportDiagram::clickedRunway,
            this, &QtLauncher::onAirportDiagramClicked);

    connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));
    connect(m_ui->airportEdit, SIGNAL(returnPressed()),
            this, SLOT(onSearchAirports()));

    connect(m_ui->airportHistory, &QPushButton::clicked,
            this, &QtLauncher::onPopupAirportHistory);
    connect(m_ui->aircraftHistory, &QPushButton::clicked,
          this, &QtLauncher::onPopupAircraftHistory);

    QAction* qa = new QAction(this);
    qa->setShortcut(QKeySequence("Ctrl+Q"));
    connect(qa, &QAction::triggered, this, &QtLauncher::onQuit);
    addAction(qa);

    connect(m_ui->editRatingFilter, &QPushButton::clicked,
            this, &QtLauncher::onEditRatingsFilter);

    QIcon historyIcon(":/history-icon");
    m_ui->aircraftHistory->setIcon(historyIcon);
    m_ui->airportHistory->setIcon(historyIcon);

    m_ui->searchIcon->setPixmap(QPixmap(":/search-icon"));

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

    fgInitPackageRoot();
    RootRef r(globals->packageRoot());

    FGHTTPClient* http = new FGHTTPClient;
    globals->add_subsystem("http", http);

    // we guard against re-init in the global phase; bind and postinit
    // will happen as normal
    http->init();

    m_aircraftModel = new AircraftItemModel(this, r);
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
    
    connect(m_ui->pathsButton, &QPushButton::clicked,
            this, &QtLauncher::onEditPaths);

    restoreSettings();

    QSettings settings;
    m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
    m_aircraftModel->scanDirs();
}

QtLauncher::~QtLauncher()
{
    
}

void QtLauncher::initApp(int& argc, char** argv)
{
    static bool qtInitDone = false;
    if (!qtInitDone) {
        qtInitDone = true;

        QApplication* app = new QApplication(argc, argv);
        app->setOrganizationName("FlightGear");
        app->setApplicationName("FlightGear");
        app->setOrganizationDomain("flightgear.org");

        // avoid double Apple menu and other weirdness if both Qt and OSG
        // try to initialise various Cocoa structures.
        flightgear::WindowBuilder::setPoseAsStandaloneApp(false);

        Qt::KeyboardModifiers mods = app->queryKeyboardModifiers();
        if (mods & Qt::AltModifier) {
            qWarning() << "Alt pressed during launch";

            // wipe out our settings
            QSettings settings;
            settings.clear();


            Options::sharedInstance()->addOption("restore-defaults", "");
        }
    }
}

bool QtLauncher::runLauncherDialog()
{
    sglog().setLogLevels( SG_ALL, SG_INFO );
    Q_INIT_RESOURCE(resources);

    // startup the nav-cache now. This pre-empts normal startup of
    // the cache, but no harm done. (Providing scenery paths are consistent)

    initNavCache();

  // setup scenery paths now, especially TerraSync path for airport
  // parking locations (after they're downloaded)

    QtLauncher dlg;
    dlg.exec();
    if (dlg.result() != QDialog::Accepted) {
        return false;
    }

    return true;
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

    updateSelectedAircraft();

    // ICAO identifiers
    m_recentAirports = settings.value("recent-airports").toStringList();
    if (!m_recentAirports.empty()) {
        setAirport(FGAirport::findByIdent(m_recentAirports.front().toStdString()));
    }
    updateAirportDescription();

    // rating filters
    m_ui->ratingsFilterCheck->setChecked(settings.value("ratings-filter", true).toBool());
    int index = 0;
    Q_FOREACH(QVariant v, settings.value("min-ratings").toList()) {
        m_ratingFilters[index++] = v.toInt();
    }

    m_aircraftProxy->setRatingFilterEnabled(m_ui->ratingsFilterCheck->isChecked());
    m_aircraftProxy->setRatings(m_ratingFilters);

    m_ui->commandLineArgs->setPlainText(settings.value("additional-args").toString());
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
    settings.setValue("recent-aircraft", QUrl::toStringList(m_recentAircraft));
    settings.setValue("recent-airports", m_recentAirports);
    settings.setValue("timeofday", m_ui->timeOfDayCombo->currentIndex());
    settings.setValue("season", m_ui->seasonCombo->currentIndex());
    settings.setValue("additional-args", m_ui->commandLineArgs->toPlainText());
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

void QtLauncher::onRun()
{
    accept();

    flightgear::Options* opt = flightgear::Options::sharedInstance();
    setEnableDisableOptionFromCheckbox(m_ui->terrasyncCheck, "terrasync");
    setEnableDisableOptionFromCheckbox(m_ui->fetchRealWxrCheckbox, "real-weather-fetch");
    setEnableDisableOptionFromCheckbox(m_ui->rembrandtCheckbox, "rembrandt");
    setEnableDisableOptionFromCheckbox(m_ui->fullScreenCheckbox, "fullscreen");
    setEnableDisableOptionFromCheckbox(m_ui->startPausedCheck, "freeze");

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
            PackageRef pkg = packageForAircraftURI(m_selectedAircraft);
            // no need to set aircraft-dir, handled by the corresponding code
            // in fgInitAircraft
            opt->addOption("aircraft", pkg->qualifiedId());
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

    // airport / location
    if (m_selectedAirport) {
        opt->addOption("airport", m_selectedAirport->ident());
    }

    if (m_ui->runwayRadio->isChecked()) {
        int index = m_ui->runwayCombo->itemData(m_ui->runwayCombo->currentIndex()).toInt();
        if ((index >= 0) && m_selectedAirport) {
            // explicit runway choice
            opt->addOption("runway", m_selectedAirport->getRunwayByIndex(index)->ident());
        }

        if (m_ui->onFinalCheckbox->isChecked()) {
            opt->addOption("glideslope", "3.0");
            opt->addOption("offset-distance", "10.0"); // in nautical miles
        }
    } else if (m_ui->parkingRadio->isChecked()) {
        // parking selection
        opt->addOption("parkpos", m_ui->parkingCombo->currentText().toStdString());
    }

    // time of day
    if (m_ui->timeOfDayCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("timeofday", dayval.toStdString());
    }

    if (m_ui->seasonCombo->currentIndex() != 0) {
        QString dayval = m_ui->timeOfDayCombo->currentText().toLower();
        opt->addOption("season", dayval.toStdString());
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

        qDebug() << "Download dir is:" << downloadDir;
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

    saveSettings();
}

void QtLauncher::onQuit()
{
    reject();
}

void QtLauncher::onSearchAirports()
{
    QString search = m_ui->airportEdit->text();
    m_airportsModel->setSearch(search);

    if (m_airportsModel->isSearchActive()) {
        m_ui->searchStatusText->setText(QString("Searching for '%1'").arg(search));
        m_ui->locationStack->setCurrentIndex(2);
    } else if (m_airportsModel->rowCount(QModelIndex()) == 1) {
        QString ident = m_airportsModel->firstIdent();
        setAirport(FGAirport::findByIdent(ident.toStdString()));
        m_ui->locationStack->setCurrentIndex(0);
    }
}

void QtLauncher::onAirportSearchComplete()
{
    int numResults = m_airportsModel->rowCount(QModelIndex());
    if (numResults == 0) {
        m_ui->searchStatusText->setText(QString("No matching airports for '%1'").arg(m_ui->airportEdit->text()));
    } else if (numResults == 1) {
        QString ident = m_airportsModel->firstIdent();
        setAirport(FGAirport::findByIdent(ident.toStdString()));
        m_ui->locationStack->setCurrentIndex(0);
    } else {
        m_ui->locationStack->setCurrentIndex(1);
    }
}

void QtLauncher::onAirportChanged()
{
    m_ui->runwayCombo->setEnabled(m_selectedAirport);
    m_ui->parkingCombo->setEnabled(m_selectedAirport);
    m_ui->airportDiagram->setAirport(m_selectedAirport);

    m_ui->runwayRadio->setChecked(true); // default back to runway mode
    // unelss multiplayer is enabled ?

    if (!m_selectedAirport) {
        m_ui->airportDescription->setText(QString());
        m_ui->airportDiagram->setEnabled(false);
        return;
    }

    m_ui->airportDiagram->setEnabled(true);

    m_ui->runwayCombo->clear();
    m_ui->runwayCombo->addItem("Automatic", -1);
    for (unsigned int r=0; r<m_selectedAirport->numRunways(); ++r) {
        FGRunwayRef rwy = m_selectedAirport->getRunwayByIndex(r);
        // add runway with index as data role
        m_ui->runwayCombo->addItem(QString::fromStdString(rwy->ident()), r);

        m_ui->airportDiagram->addRunway(rwy);
    }

    m_ui->parkingCombo->clear();
    FGAirportDynamics* dynamics = m_selectedAirport->getDynamics();
    PositionedIDVec parkings = NavDataCache::instance()->airportItemsOfType(
                                                                            m_selectedAirport->guid(),
                                                                            FGPositioned::PARKING);
    if (parkings.empty()) {
        m_ui->parkingCombo->setEnabled(false);
        m_ui->parkingRadio->setEnabled(false);
    } else {
        m_ui->parkingCombo->setEnabled(true);
        m_ui->parkingRadio->setEnabled(true);
        Q_FOREACH(PositionedID parking, parkings) {
            FGParking* park = dynamics->getParking(parking);
            m_ui->parkingCombo->addItem(QString::fromStdString(park->getName()),
                                        static_cast<qlonglong>(parking));

            m_ui->airportDiagram->addParking(park);
        }
    }
}

void QtLauncher::onAirportDiagramClicked(FGRunwayRef rwy)
{
    if (rwy) {
        m_ui->runwayRadio->setChecked(true);
        int rwyIndex = m_ui->runwayCombo->findText(QString::fromStdString(rwy->ident()));
        m_ui->runwayCombo->setCurrentIndex(rwyIndex);
    }
    
    updateAirportDescription();
}

void QtLauncher::onToggleTerrasync(bool enabled)
{
    if (enabled) {
        QSettings settings;
        QString downloadDir = settings.value("download-dir").toString();
        if (downloadDir.isEmpty()) {
            downloadDir = QString::fromStdString(flightgear::defaultDownloadDir());
        }

        QFileInfo info(downloadDir);
        if (!info.exists()) {
            QMessageBox msg;
            msg.setWindowTitle(tr("Create download folder?"));
            msg.setText(tr("The download folder '%1' does not exist, create it now? "
                           "Click 'change location' to choose another folder "
                           "to store downloaded files").arg(downloadDir));
            msg.addButton(QMessageBox::Yes);
            msg.addButton(QMessageBox::Cancel);
            msg.addButton(tr("Change location"), QMessageBox::ActionRole);
            int result = msg.exec();
  
            if (result == QMessageBox::Cancel) {
                m_ui->terrasyncCheck->setChecked(false);
                return;
            }

            if (result == QMessageBox::ActionRole) {
                onEditPaths();
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

void QtLauncher::updateAirportDescription()
{
    if (!m_selectedAirport) {
        m_ui->airportDescription->setText(QString("No airport selected"));
        return;
    }

    QString ident = QString::fromStdString(m_selectedAirport->ident()),
        name = QString::fromStdString(m_selectedAirport->name());
    QString locationOnAirport;
    if (m_ui->runwayRadio->isChecked()) {
        bool onFinal = m_ui->onFinalCheckbox->isChecked();
        int comboIndex = m_ui->runwayCombo->currentIndex();
        QString runwayName = (comboIndex == 0) ?
            "active runway" :
            QString("runway %1").arg(m_ui->runwayCombo->currentText());

        if (onFinal) {
            locationOnAirport = QString("on 10-mile final to %1").arg(runwayName);
        } else {
            locationOnAirport = QString("on %1").arg(runwayName);
        }
        
        int runwayIndex = m_ui->runwayCombo->itemData(comboIndex).toInt();
        FGRunwayRef rwy = (runwayIndex >= 0) ?
            m_selectedAirport->getRunwayByIndex(runwayIndex) : FGRunwayRef();
        m_ui->airportDiagram->setSelectedRunway(rwy);
    } else if (m_ui->parkingRadio->isChecked()) {
        locationOnAirport =  QString("at parking position %1").arg(m_ui->parkingCombo->currentText());
    }

    m_ui->airportDescription->setText(QString("%2 (%1): %3").arg(ident).arg(name).arg(locationOnAirport));
}

void QtLauncher::onAirportChoiceSelected(const QModelIndex& index)
{
    m_ui->locationStack->setCurrentIndex(0);
    setAirport(FGPositioned::loadById<FGAirport>(index.data(Qt::UserRole).toULongLong()));
}

void QtLauncher::onAircraftSelected(const QModelIndex& index)
{
    m_selectedAircraft = index.data(AircraftURIRole).toUrl();
    updateSelectedAircraft();
}

void QtLauncher::onRequestPackageInstall(const QModelIndex& index)
{
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
    qDebug() << "cancel download of" << pkg;
    simgear::pkg::PackageRef pref = globals->packageRoot()->getPackageById(pkg.toStdString());
    simgear::pkg::InstallRef i = pref->existingInstall();
    i->cancelDownload();
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
    } else {
        m_ui->thumbnail->setPixmap(QPixmap());
        m_ui->aircraftDescription->setText("");
        m_ui->runButton->setEnabled(false);
    }
}

void QtLauncher::onPopupAirportHistory()
{
    if (m_recentAirports.isEmpty()) {
        return;
    }

    QMenu m;
    Q_FOREACH(QString aptCode, m_recentAirports) {
        FGAirportRef apt = FGAirport::findByIdent(aptCode.toStdString());
        QString name = QString::fromStdString(apt->name());
        QAction* act = m.addAction(QString("%1 - %2").arg(aptCode).arg(name));
        act->setData(aptCode);
    }

    QPoint popupPos = m_ui->airportHistory->mapToGlobal(m_ui->airportHistory->rect().bottomLeft());
    QAction* triggered = m.exec(popupPos);
    if (triggered) {
        FGAirportRef apt = FGAirport::findByIdent(triggered->data().toString().toStdString());
        setAirport(apt);
        m_ui->airportEdit->clear();
        m_ui->locationStack->setCurrentIndex(0);
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

void QtLauncher::setAirport(FGAirportRef ref)
{
    if (m_selectedAirport == ref)
        return;

    m_selectedAirport = ref;
    onAirportChanged();

    if (ref.valid()) {
        // maintain the recent airport list
        QString icao = QString::fromStdString(ref->ident());
        if (m_recentAirports.contains(icao)) {
            // move to front
            m_recentAirports.removeOne(icao);
            m_recentAirports.push_front(icao);
        } else {
            // insert and trim list if necessary
            m_recentAirports.push_front(icao);
            if (m_recentAirports.size() > MAX_RECENT_AIRPORTS) {
                m_recentAirports.pop_back();
            }
        }
    }

    updateAirportDescription();
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

void QtLauncher::onSubsytemIdleTimeout()
{
    globals->get_subsystem_mgr()->update(0.0);
}

void QtLauncher::onEditPaths()
{
    PathsDialog dlg(this, globals->packageRoot());
    dlg.exec();
    if (dlg.result() == QDialog::Accepted) {
        // re-scan the aircraft list
        QSettings settings;
        m_aircraftModel->setPaths(settings.value("aircraft-paths").toStringList());
        m_aircraftModel->scanDirs();
    }
}

simgear::pkg::PackageRef QtLauncher::packageForAircraftURI(QUrl uri) const
{
    if (uri.scheme() != "package") {
        qWarning() << "invalid URL scheme:" << uri;
        return simgear::pkg::PackageRef();
    }
    
    QString ident = uri.path();
    qDebug() << Q_FUNC_INFO << uri << ident;
    return globals->packageRoot()->getPackageById(ident.toStdString());
}

#include "QtLauncher.moc"

