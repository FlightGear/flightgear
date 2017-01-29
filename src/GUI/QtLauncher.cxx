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
#include <QThread>

// Simgear
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/debug/logstream.hxx>

#include "ui_Launcher.h"
#include "ui_NoOfficialHangar.h"
#include "ui_UpdateAllAircraft.h"

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
#include <Airports/airport.hxx>

#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Main/AircraftDirVisitorBase.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Network/HTTPClient.hxx>
#include <Network/RemoteXMLRequest.hxx>

using namespace flightgear;
using namespace simgear::pkg;
using std::string;

const int MAX_RECENT_AIRCRAFT = 20;
const int DEFAULT_MP_PORT = 5000;

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
            case NavDataCache::REBUILD_READING_APT_DAT_FILES:
                rebuildProgress.setLabelText(QT_TR_NOOP("Reading airport data"));
                break;

            case NavDataCache::REBUILD_LOADING_AIRPORTS:
                rebuildProgress.setLabelText(QT_TR_NOOP("Loading airports"));
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

class NaturalEarthDataLoaderThread : public QThread
{
    Q_OBJECT
public:

    NaturalEarthDataLoaderThread() :
        m_lineInsertCount(0)
    {
        connect(this, &QThread::finished, this, &NaturalEarthDataLoaderThread::onFinished);
    }

protected:
    virtual void run() Q_DECL_OVERRIDE
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

private:
    Q_SLOT void onFinished()
    {
        flightgear::PolyLineList::const_iterator begin = m_parsedLines.begin() + m_lineInsertCount;
        unsigned int numToAdd = std::min<unsigned long>(1000UL, m_parsedLines.size() - m_lineInsertCount);
        flightgear::PolyLineList::const_iterator end = begin + numToAdd;
        flightgear::PolyLine::bulkAddToSpatialIndex(begin, end);

        if (end == m_parsedLines.end()) {
            deleteLater(); // commit suicide
        } else {
            m_lineInsertCount += 1000;
            QTimer::singleShot(50, this, SLOT(onFinished()));
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
        flightgear::SHPParser::parsePolyLines(path, aType, m_parsedLines, areClosed);
    }
    
    flightgear::PolyLineList m_parsedLines;
    unsigned int m_lineInsertCount;
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

    void setAircraftFilterString(QString s)
    {
        m_filterString = s;

        m_filterProps = new SGPropertyNode;
        int index = 0;
        Q_FOREACH(QString term, s.split(' ')) {
            m_filterProps->getNode("all-of/text", index++, true)->setStringValue(term.toStdString());
        }

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
        if (status == MessageWidget) {
            return true;
        }

        if (!filterAircraft(index)) {
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
    bool filterAircraft(const QModelIndex& sourceIndex) const
    {
        if (m_filterString.isEmpty()) {
            return true;
        }

        simgear::pkg::PackageRef pkg = sourceIndex.data(AircraftPackageRefRole).value<simgear::pkg::PackageRef>();
        if (pkg) {
            return pkg->matches(m_filterProps.ptr());
        }

        QString baseName = sourceIndex.data(Qt::DisplayRole).toString();
        if (baseName.contains(m_filterString, Qt::CaseInsensitive)) {
            return true;
        }

        QString longDesc = sourceIndex.data(AircraftLongDescriptionRole).toString();
        if (longDesc.contains(m_filterString, Qt::CaseInsensitive)) {
            return true;
        }

        const int variantCount = sourceIndex.data(AircraftVariantCountRole).toInt();
        for (int variant = 0; variant < variantCount; ++variant) {
            QString desc = sourceIndex.data(AircraftVariantDescriptionRole + variant).toString();
            if (desc.contains(m_filterString, Qt::CaseInsensitive)) {
                return true;
            }
        }

        return false;
    }

    bool m_ratingsFilter;
    bool m_onlyShowInstalled;
    int m_ratings[4];
    QString m_filterString;
    SGPropertyNode_ptr m_filterProps;
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


static void initQtResources()
{
    Q_INIT_RESOURCE(resources);
}

static SGPropertyNode_ptr loadXMLDefaults()
{
    SGPropertyNode_ptr root(new SGPropertyNode);
    const SGPath defaultsXML = globals->get_fg_root() / "defaults.xml";
    readProperties(defaultsXML, root);

    if (!root->hasChild("sim")) {
        qWarning() << "Failed to find /sim node in defaults.xml, broken";
        return SGPropertyNode_ptr();
    }

    return root;
}

static std::string defaultAirportICAO()
{
    SGPropertyNode_ptr root = loadXMLDefaults();
    if (!root) {
        return std::string();
    }

    std::string airportCode = root->getStringValue("/sim/presets/airport-id");
    return airportCode;
}

/**
 * we don't want to rely on the main AircraftModel threaded scan, to find the
 * default aircraft, so we do a synchronous scan here, on the assumption that
 * FG_DATA/Aircraft only contains a handful of entries.
 */
class DefaultAircraftLocator : public AircraftDirVistorBase
{
public:
    DefaultAircraftLocator()
    {
        SGPropertyNode_ptr root = loadXMLDefaults();
        if (root) {
            _aircraftId = root->getStringValue("/sim/aircraft");
        } else {
            qWarning() << "failed to load default aircraft identifier";
            _aircraftId = "ufo"; // last ditch fallback
        }

        _aircraftId += "-set.xml";
        const SGPath rootAircraft = globals->get_fg_root() / "Aircraft";
        visitDir(rootAircraft, 0);
    }

    SGPath foundPath() const
    {
        return _foundPath;
    }

private:
    virtual VisitResult visit(const SGPath& p) override
    {
        if (p.file() == _aircraftId) {
            _foundPath = p;
            return VISIT_DONE;
        }

        return VISIT_CONTINUE;
    }
    
    std::string _aircraftId;
    SGPath _foundPath;
};

static void simgearMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    sgDebugPriority mappedPriority = SG_WARN;
    switch (type) {
    case QtDebugMsg:    mappedPriority = SG_DEBUG; break;
    case QtInfoMsg:     mappedPriority = SG_INFO; break;
    case QtWarningMsg:  mappedPriority = SG_WARN; break;
    case QtCriticalMsg: mappedPriority = SG_ALERT; break;
    case QtFatalMsg:    mappedPriority = SG_POPUP; break;
    }

    static const char* nullFile = "";
    const char* file = context.file ? context.file : nullFile;
    sglog().log(SG_GUI, mappedPriority, file, context.line, msg.toStdString());
    if (type == QtFatalMsg) {
        abort();
    }
}

namespace flightgear
{

// Only requires FGGlobals to be initialized if 'doInitQSettings' is true.
// Safe to call several times.
void initApp(int& argc, char** argv, bool doInitQSettings)
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

        // log to simgear instead of the console from Qt, so we go to
        // whichever log locations SimGear has configured
        qInstallMessageHandler(simgearMessageOutput);

        QApplication* app = new QApplication(s_argc, argv);
        app->setOrganizationName("FlightGear");
        app->setApplicationName("FlightGear");
        app->setOrganizationDomain("flightgear.org");

        // reset numeric / collation locales as described at:
        // http://doc.qt.io/qt-5/qcoreapplication.html#details
        ::setlocale(LC_NUMERIC, "C");
        ::setlocale(LC_COLLATE, "C");
    }

    if (doInitQSettings) {
        initQSettings();
    }
}

// Requires FGGlobals to be initialized. Safe to call several times.
void initQSettings()
{
    static bool qSettingsInitDone = false;

    if (!qSettingsInitDone) {
        qSettingsInitDone = true;
        string fgHome = globals->get_fg_home().utf8Str();

        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           QString::fromStdString(fgHome));
    }
}

bool checkKeyboardModifiersForSettingFGRoot()
{
    initQSettings();

    Qt::KeyboardModifiers mods = qApp->queryKeyboardModifiers();
    if (mods & (Qt::AltModifier | Qt::ShiftModifier)) {
        qWarning() << "Alt/shift pressed during launch";
        QSettings settings;
        settings.setValue("fg-root", "!ask");
        return true;
    }

    return false;
}

bool runLauncherDialog()
{
    // Used for NavDataCache initialization: needed to find the apt.dat files
    QtLauncher::setSceneryPaths();
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

    NaturalEarthDataLoaderThread* naturalEarthLoader = new NaturalEarthDataLoaderThread;
    naturalEarthLoader->start();

    // avoid double Apple menu and other weirdness if both Qt and OSG
    // try to initialise various Cocoa structures.
    flightgear::WindowBuilder::setPoseAsStandaloneApp(false);

    QtLauncher dlg;
    dlg.show();

    int appResult = qApp->exec();
    if (appResult <= 0) {
        return false; // quit
    }

    // don't set scenery paths twice
    globals->clear_fg_scenery();

    return true;
}

bool runInAppLauncherDialog()
{
    QtLauncher dlg;
    bool accepted = dlg.execInApp();
    if (!accepted) {
        return false;
    }

    return true;
}

} // of namespace flightgear

QtLauncher::QtLauncher() :
    QMainWindow(),
    m_ui(NULL),
    m_subsystemIdleTimer(NULL),
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
            m_aircraftProxy, &AircraftProxyModel::setAircraftFilterString);

    connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
    connect(m_ui->quitButton, SIGNAL(clicked()), this, SLOT(onQuit()));

    connect(m_ui->aircraftHistory, &QPushButton::clicked,
          this, &QtLauncher::onPopupAircraftHistory);
    connect(m_ui->locationHistory, &QPushButton::clicked,
          this, &QtLauncher::onPopupLocationHistory);

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
    m_ui->locationHistory->setIcon(historyIcon);

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
    connect(delegate, &AircraftItemDelegate::requestUninstall,
            this, &QtLauncher::onRequestPackageUninstall);
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
    // if we don't cancel this now, it may complete after we are gone,
    // causing a crash when the SGCallback fires (SGCallbacks don't clean up
    // when their subject is deleted)
    globals->get_subsystem<FGHTTPClient>()->client()->cancelRequest(m_mpServerRequest);
}

void QtLauncher::setSceneryPaths() // static method
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

    // add the installation path since it contains default airport data,
    // if terrasync is disabled or on first-launch
    globals->append_fg_scenery(globals->get_fg_root() / "Scenery");

}

bool QtLauncher::execInApp()
{
  m_inAppMode = true;
  m_ui->tabWidget->removeTab(3);
  m_ui->tabWidget->removeTab(3);

  m_ui->runButton->setText(tr("Apply"));
  m_ui->quitButton->setText(tr("Cancel"));

  disconnect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onRun()));
  connect(m_ui->runButton, SIGNAL(clicked()), this, SLOT(onApply()));
    m_runInApp = true;

    show();

    while (m_runInApp) {
        qApp->processEvents();
    }

    return m_accepted;
}

void QtLauncher::restoreSettings()
{
    QSettings settings;

    restoreGeometry(settings.value("window-geometry").toByteArray());

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
        // select the default aircraft specified in defaults.xml
        DefaultAircraftLocator da;
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
        std::string defaultAirport = defaultAirportICAO();
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

        // and also select the correct variant on the model
        m_aircraftModel->selectVariantForAircraftURI(m_selectedAircraft);
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
    settings.setValue("recent-location-sets", m_recentLocations);

    settings.setValue("timeofday", m_ui->timeOfDayCombo->currentIndex());
    settings.setValue("season", m_ui->seasonCombo->currentIndex());
    settings.setValue("additional-args", m_ui->commandLineArgs->toPlainText());

    settings.setValue("mp-callsign", m_ui->mpCallsign->text());
    settings.setValue("mp-server", m_ui->mpServerCombo->currentData());
    settings.setValue("mp-enabled", m_ui->mpBox->isChecked());

    settings.setValue("window-geometry", saveGeometry());
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

    m_ui->location->setLocationProperties();
    updateLocationHistory();

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

    // set a positive value here so we can detect this case in runLauncherDialog
    qApp->exit(1);
}


void QtLauncher::onApply()
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

void QtLauncher::updateLocationHistory()
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

void QtLauncher::onQuit()
{
    if (m_inAppMode) {
        m_runInApp = false;
    } else {
        qApp->exit(-1);
    }
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

void QtLauncher::onRequestPackageUninstall(const QModelIndex& index)
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
        m_ui->aircraftName->setText(index.data(Qt::DisplayRole).toString());

        QVariant longDesc = index.data(AircraftLongDescriptionRole);
        m_ui->aircraftDescription->setVisible(!longDesc.isNull());
        m_ui->aircraftDescription->setText(longDesc.toString());

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
        m_ui->aircraftName->setText("");
        m_ui->aircraftDescription->hide();
        m_ui->runButton->setEnabled(false);
    }
}

void QtLauncher::onUpdateAllAircraft()
{
    const PackageList& toBeUpdated = globals->packageRoot()->packagesNeedingUpdate();
    std::for_each(toBeUpdated.begin(), toBeUpdated.end(), [](PackageRef pkg) {
        globals->packageRoot()->scheduleToUpdate(pkg->install());
    });
}

void QtLauncher::onPackagesNeedUpdate(bool yes)
{
    Q_UNUSED(yes);
    checkUpdateAircraft();
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

void QtLauncher::onPopupLocationHistory()
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

    if (m_ui->mpBox->isChecked()) {
        summary.append(tr("multiplayer: %1").arg(m_ui->mpCallsign->text()));
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

bool QtLauncher::shouldShowOfficialCatalogMessage() const
{
    QSettings settings;
    bool showOfficialCatalogMesssage = !globals->get_subsystem<FGHTTPClient>()->isDefaultCatalogInstalled();
    if (settings.value("hide-official-catalog-message").toBool()) {
        showOfficialCatalogMesssage = false;
    }
    return showOfficialCatalogMesssage;
}
void QtLauncher::checkOfficialCatalogMessage()
{
    const bool show = shouldShowOfficialCatalogMessage();
    m_aircraftModel->setMessageWidgetVisible(show);
    if (show) {
        NoOfficialHangarMessage* messageWidget = new NoOfficialHangarMessage;
        connect(messageWidget, &NoOfficialHangarMessage::linkActivated,
                this, &QtLauncher::onOfficialCatalogMessageLink);

        QModelIndex index = m_aircraftProxy->mapFromSource(m_aircraftModel->messageWidgetIndex());
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

void QtLauncher::checkUpdateAircraft()
{
    if (shouldShowOfficialCatalogMessage()) {
        return; // don't interfere
    }

    const bool showUpdateMessage = !globals->packageRoot()->packagesNeedingUpdate().empty();
    m_aircraftModel->setMessageWidgetVisible(showUpdateMessage);
    if (showUpdateMessage) {
        UpdateAllAircraftMessage* messageWidget = new UpdateAllAircraftMessage;
       // connect(messageWidget, &UpdateAllAircraftMessage::linkActivated,
      //        this, &QtLauncher::onMessageLink);
        connect(messageWidget, &UpdateAllAircraftMessage::updateAll, this, &QtLauncher::onUpdateAllAircraft);
        QModelIndex index = m_aircraftProxy->mapFromSource(m_aircraftModel->messageWidgetIndex());
        m_ui->aircraftList->setIndexWidget(index, messageWidget);
    }
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
