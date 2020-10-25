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

#include "config.h"

#include "QtLauncher.hxx"

#include <locale.h>

// Qt
#include <QtGlobal>
#include <QString>
#include <QProgressDialog>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QUrl>
#include <QApplication>
#include <QThread>
#include <QProcess>
#include <QTranslator>
#include <QMessageBox>

// Simgear
#include <simgear/timing/timestamp.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/debug/logstream.hxx>

#include <Add-ons/AddonManager.hxx>
#include <Airports/airport.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/sentryIntegration.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/SHPParser.hxx>
#include <Navaids/navrecord.hxx>


#include <Main/fg_init.hxx>
#include <Main/locale.hxx>
#include <Main/options.hxx>
#include <Network/HTTPClient.hxx>
#include <Viewer/WindowBuilder.hxx>

#include "LaunchConfig.hxx"
#include "LauncherMainWindow.hxx"
#include "LocalAircraftCache.hxx"
#include "PathListModel.hxx"
#include "UnitsModel.hxx"

using namespace flightgear;
using namespace simgear::pkg;
using std::string;

namespace { // anonymous namespace

struct ProgressLabel {
    NavDataCache::RebuildPhase phase;
    const char* label;
};

static std::initializer_list<ProgressLabel> progressStrings = {
    {NavDataCache::REBUILD_READING_APT_DAT_FILES, QT_TRANSLATE_NOOP("initNavCache","Reading airport data")},
    {NavDataCache::REBUILD_LOADING_AIRPORTS, QT_TRANSLATE_NOOP("initNavCache","Loading airports")},
    {NavDataCache::REBUILD_FIXES, QT_TRANSLATE_NOOP("initNavCache","Loading waypoint data")},
    {NavDataCache::REBUILD_NAVAIDS, QT_TRANSLATE_NOOP("initNavCache","Loading navigation data")},
    {NavDataCache::REBUILD_POIS, QT_TRANSLATE_NOOP("initNavCache","Loading point-of-interest data")}
};

void initNavCache()
{
    const char* baseLabelKey = QT_TRANSLATE_NOOP("initNavCache", "Initialising navigation data, this may take several minutes");
    QString baseLabel= qApp->translate("initNavCache", baseLabelKey);

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
        rebuildProgress.setMinimumWidth(600);
        rebuildProgress.setAutoReset(false);
        rebuildProgress.setAutoClose(false);
        rebuildProgress.show();
    
        QTimer updateTimer;
        updateTimer.setInterval(100);
        QObject::connect(&updateTimer, &QTimer::timeout, [&cache, &rebuildProgress, &baseLabel]() {
            auto phase = cache->rebuild();
            if (phase == NavDataCache::REBUILD_DONE) {
                rebuildProgress.done(0);
                return;
            }
            
            auto it = std::find_if(progressStrings.begin(), progressStrings.end(), [phase]
                                   (const ProgressLabel& l) { return l.phase == phase; });
            if (it == progressStrings.end()) {
                rebuildProgress.setLabelText(baseLabel);
            } else {
                QString trans = qApp->translate("initNavCache", it->label);
                rebuildProgress.setLabelText(trans);
            }

            if (phase == NavDataCache::REBUILD_UNKNOWN) {
                rebuildProgress.setValue(0);
                rebuildProgress.setMaximum(0);
            } else {
                rebuildProgress.setValue(static_cast<int>(cache->rebuildPhaseCompletionPercentage()));
                rebuildProgress.setMaximum(100);
            }
        });
        
        updateTimer.start(); // timer won't actually run until we process events
        rebuildProgress.exec();
        updateTimer.stop();
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
    void run() override
    {
        loadNaturalEarthFile("ne_10m_coastline.shp", flightgear::PolyLine::COASTLINE, false);
        loadNaturalEarthFile("ne_10m_rivers_lake_centerlines.shp", flightgear::PolyLine::RIVER, false);
        loadNaturalEarthFile("ne_10m_lakes.shp", flightgear::PolyLine::LAKE, true);
        loadNaturalEarthFile("ne_10m_urban_areas.shp", flightgear::PolyLine::URBAN, true);
    }

private:
    Q_SLOT void onFinished()
    {
        flightgear::PolyLineList::const_iterator begin = m_parsedLines.begin() + m_lineInsertCount;
        unsigned int numToAdd = std::min<unsigned int>(1000U, static_cast<unsigned int>(m_parsedLines.size()) - m_lineInsertCount);
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
#if defined(HAVE_QRC_TRANSLATIONS)
    Q_INIT_RESOURCE(translations);
#endif
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
    const auto s = msg.toStdString();
    // important we copy the file name here, since QMessageLogContext doesn't
    sglog().logCopyingFilename(SG_GUI, mappedPriority, file, context.line, s);
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

static std::unique_ptr<QApplication> static_qApp;

// Only requires FGGlobals to be initialized if 'doInitQSettings' is true.
// Safe to call several times.
void initApp(int& argc, char** argv, bool doInitQSettings)
{
    static bool qtInitDone = false;
    static int s_argc;

    if (!qtInitDone) {
        qtInitDone = true;

        // Disable Qt 5.15 warnings about obsolete Connections/onFoo: syntax
        // we cannot use the new syntax
        // as long as we have to support Qt 5.9
        qputenv("QT_LOGGING_RULES", "qt.qml.connections.warning=false");
        
        initQtResources(); // can't be called from a namespace

        s_argc = argc; // QApplication only stores a reference to argc,
        // and may crash if it is freed
        // http://doc.qt.io/qt-5/qguiapplication.html#QGuiApplication

        // log to simgear instead of the console from Qt, so we go to
        // whichever log locations SimGear has configured
        qInstallMessageHandler(simgearMessageOutput);

        // ensure we use desktop OpenGL, don't even fall back to ANGLE, since
        // this gets into a knot on Optimus setups (since we export the magic
        // Optimus / AMD symbols in main.cxx).
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

		// becuase on Windows, Qt only supports integer scaling factors,
		// forceibly enabling HighDpiScaling is controversial.
		// leave things unset here, so users can use env var
		// QT_AUTO_SCREEN_SCALE_FACTOR=1 to enable it at runtime

//#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
      //  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
//#endif
        static_qApp.reset(new QApplication(s_argc, argv));
        static_qApp->setOrganizationName("FlightGear");
        static_qApp->setApplicationName("FlightGear");
        static_qApp->setOrganizationDomain("flightgear.org");

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        static_qApp->setDesktopFileName(
          QStringLiteral("org.flightgear.FlightGear.desktop"));
#endif
        QTranslator* fallbackTranslator = new QTranslator(static_qApp.get());
        if (!fallbackTranslator->load(QLatin1String(":/FlightGear_en_US.qm"))) {
            qWarning() << "Failed to load default (en) translations";
            delete fallbackTranslator;
        } else {
            static_qApp->installTranslator(fallbackTranslator);
        }

        qInfo() << "UI languages:" << QLocale().uiLanguages();
        
        QTranslator* translator = new QTranslator(static_qApp.get());
        // check for --langauge=xx option and prefer that over QLocale
        // detection of the locale if it exists
        auto lang = simgear::strutils::replace(
            Options::getArgValue(argc, argv, "--language"),
            "-",
            "_");
        if (!lang.empty()) {
            QString localeFile = "FlightGear_" + QString::fromStdString(lang);
            if (translator->load(localeFile, QLatin1String(":/"))) {
                qInfo() << "Loaded translations based on --language from:" << localeFile;
                static_qApp->installTranslator(translator);
            } else {
                qInfo() << "--langauge was set, but no translations found at:" << localeFile;
                delete translator;
            }
        } else if (translator->load(QLocale(), QLatin1String("FlightGear"), QLatin1String("_"), QLatin1String(":/"))) {
            // QLocale().name() looks like ' "it_IT" ' (without the outer
            // quotes) when running FG on Linux with LANG=it_IT.UTF-8.
            qInfo() << "Loaded translations for locale" << QLocale().name();
            static_qApp->installTranslator(translator);
        } else {
            delete translator;
        }

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
	// restore default message handler, otherwise Qt logging on
	// shutdown crashes once sglog is killed
	qInstallMessageHandler(nullptr);
    static_qApp.reset();
}

// Requires FGGlobals to be initialized. Safe to call several times.
void initQSettings()
{
    static bool qSettingsInitDone = false;

    if (!qSettingsInitDone) {
        qRegisterMetaType<QuantityValue>();
        qRegisterMetaTypeStreamOperators<QuantityValue>("QuantityValue");

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
#if defined(Q_OS_WIN)
    const auto altState = GetAsyncKeyState(VK_MENU);
    const auto shiftState = GetAsyncKeyState(VK_SHIFT);
    if ((altState < 0) || (shiftState < 0))
#else
    Qt::KeyboardModifiers mods = qApp->queryKeyboardModifiers();
    if (mods & (Qt::AltModifier | Qt::ShiftModifier))
#endif
    {
        qWarning() << "Alt/shift pressed during launch";
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

	// ensure we release whatever mutex/lock file we have in home,
	// so the new instance runs in writeable mode
	fgShutdownHome();

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
    Q_FOREACH(QString path, PathListModel::readEnabledPaths("scenery-paths-v2")) {
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
    const SGPath rootScenery = globals->get_fg_root() / "Scenery";
    if (rootScenery.exists()) {
        globals->append_fg_scenery(rootScenery);
    }
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
        QString downloadDir = settings.value("download-dir").toString();
        if (!downloadDir.isEmpty()) {
            options->setOption("download-dir", downloadDir.toStdString());
        }
    }

    fgInitPackageRoot();

    // setup package language
    auto lang = options->valueForOption("language");
    if (lang.empty()) {
        const auto langName = QLocale::languageToString(QLocale{}.language());
        lang = langName.toStdString();
    }

    // we will re-do this later, but we want to access translated strings
    // from within the launcher
    globals->get_locale()->selectLanguage(lang);
    globals->packageRoot()->setLocale(globals->get_locale()->getPreferredLanguage());

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

    LauncherMainWindow dlg(false);
    if (options->isOptionSet("enable-fullscreen")) {
        dlg.showFullScreen();
    } else {
        dlg.show();
    }

    int appResult = qApp->exec();
    if (appResult <= 0) {
        return false; // quit
    }

    // avoid a race-y crash on the locale, if a scan thread is
    // still running: this reset will cancel any running scan
    LocalAircraftCache::reset();

    // don't set scenery paths twice
    globals->clear_fg_scenery();
    globals->get_locale()->clear();

    return true;
}

bool runInAppLauncherDialog()
{
    LauncherMainWindow dlg(true);
    bool accepted = dlg.execInApp();
    if (!accepted) {
        return false;
    }

    return true;
}

static const char* static_lockFileDialog_Title =
    QT_TRANSLATE_NOOP("LockFileDialog", "Multiple copies of FlightGear running");
static const char* static_lockFileDialog_Text =
    QT_TRANSLATE_NOOP("LockFileDialog",
                      "FlightGear has detected another copy is already running. "
                      "This copy will run in read-only mode, so downloads will not be possible, "
                      "and settings will not be saved.");
static const char* static_lockFileDialog_Info =
    QT_TRANSLATE_NOOP("LockFileDialog",
                      "If you are sure another copy is not running on this computer, "
                      "you can choose to reset the lock file, and run this copy as normal. "
                       "Alternatively, you can close this copy of the software.");

LockFileDialogResult showLockFileDialog()
{
    flightgear::addSentryBreadcrumb("showing lock-file dialog", "info");

    QString title = qApp->translate("LockFileDialog", static_lockFileDialog_Title);
    QString text = qApp->translate("LockFileDialog", static_lockFileDialog_Text);
    QString infoText = qApp->translate("LockFileDialog", static_lockFileDialog_Info);

    QMessageBox mb;
    mb.setIconPixmap(QPixmap(":/app-icon-large"));
    mb.setWindowTitle(title);
    mb.setText(text);
    mb.setInformativeText(infoText);
    mb.addButton(QMessageBox::Ok);
    mb.setDefaultButton(QMessageBox::Ok);
    mb.addButton(QMessageBox::Reset);
    mb.addButton(QMessageBox::Close);

    int r = mb.exec();
    if (r == QMessageBox::Reset)
        return LockFileReset;
    if (r == QMessageBox::Close)
        return LockFileQuit;
    return LockFileContinue;
}

} // of namespace flightgear

#include "QtLauncher.moc"
