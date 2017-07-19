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

#include <locale.h>

// Qt
#include <QtGlobal>
#include <QString>
#include <QProgressDialog>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <QCompleter>
#include <QListView>
#include <QSettings>
#include <QUrl>
#include <QAction>
#include <QDateTime>
#include <QApplication>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QThread>
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
#include <simgear/debug/logstream.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/SHPParser.hxx>
#include <Airports/airport.hxx>

#include <Main/options.hxx>
#include <Main/fg_init.hxx>
#include <Viewer/WindowBuilder.hxx>
#include <Network/HTTPClient.hxx>

#include "LauncherMainWindow.hxx"
#include "LaunchConfig.hxx"

using namespace flightgear;
using namespace simgear::pkg;
using std::string;

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

static void initQtResources()
{
    Q_INIT_RESOURCE(resources);
}



static void simgearMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    sgDebugPriority mappedPriority = SG_WARN;
    switch (type) {
    case QtDebugMsg:    mappedPriority = SG_DEBUG; break;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:     mappedPriority = SG_INFO; break;
#endif
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

// making this a unique ptr ensures the QApplication will be deleted
// event if we forget to call shutdownQtApp. Cleanly destroying this is
// important so QPA resources, in particular the XCB thread, are exited
// cleanly on quit. However, at present, the official policy is that static
// destruction is too late to call this, hence why we have shutdownQtApp()

std::unique_ptr<QApplication> static_qApp;

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

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
        static_qApp.reset(new QApplication(s_argc, argv));
        static_qApp->setOrganizationName("FlightGear");
        static_qApp->setApplicationName("FlightGear");
        static_qApp->setOrganizationDomain("flightgear.org");

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        static_qApp->setDesktopFileName(
          QStringLiteral("org.flightgear.FlightGear.desktop"));
#endif

        // reset numeric / collation locales as described at:
        // http://doc.qt.io/qt-5/qcoreapplication.html#details
        ::setlocale(LC_NUMERIC, "C");
        ::setlocale(LC_COLLATE, "C");
    }

    if (doInitQSettings) {
        initQSettings();
    }
}

void shutdownQtApp()
{
    static_qApp.reset();
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


void restartTheApp()
{
    QStringList fgArgs;

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


void launcherSetSceneryPaths()
{
    globals->clear_fg_scenery();

// mimic what options.cxx does, so we can find airport data for parking
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

bool runLauncherDialog()
{
    // Used for NavDataCache initialization: needed to find the apt.dat files
    launcherSetSceneryPaths();
    // startup the nav-cache now. This pre-empts normal startup of
    // the cache, but no harm done. (Providing scenery paths are consistent)

    initNavCache();

    auto options = flightgear::Options::sharedInstance();
    if (options->isOptionSet("download-dir")) {
        // user set download-dir on command line, don't mess with it in the
        // launcher GUI. We'll disable the UI.
        LaunchConfig::setEnableDownloadDirUI(false);
    } else {
        QSettings settings;
        QString downloadDir = settings.value("downloadSettings/downloadDir").toString();
        if (!downloadDir.isEmpty()) {
            options->setOption("download-dir", downloadDir.toStdString());
        }
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

    LauncherMainWindow dlg;
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
    LauncherMainWindow dlg;
    bool accepted = dlg.execInApp();
    if (!accepted) {
        return false;
    }

    return true;
}

} // of namespace flightgear

#include "QtLauncher.moc"
