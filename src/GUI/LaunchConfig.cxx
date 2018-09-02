#include "LaunchConfig.hxx"

#include <set>

#include <Main/options.hxx>
#include <simgear/misc/sg_path.hxx>

#include <QSettings>
#include <QDebug>
#include <QIODevice>
#include <QDataStream>
#include <QClipboard>
#include <QGuiApplication>

static bool static_enableDownloadDirUI = true;
static QSettings::Format static_binaryFormat = QSettings::InvalidFormat;

static bool binaryReadFunc(QIODevice &device, QSettings::SettingsMap &map)
{
    QDataStream ds(&device);
    int count;
    ds >> count;
    for (int i=0; i < count; ++i) {
        QString k;
        QVariant v;
        ds >> k >> v;
        map.insert(k, v);
    }

    return true;
}

static bool binaryWriteFunc(QIODevice &device, const QSettings::SettingsMap &map)
{
    QDataStream ds(&device);

    ds << map.size();
    Q_FOREACH(QString key, map.keys()) {
        ds << key << map.value(key);
    }

    return true;
}

LaunchConfig::LaunchConfig(QObject* parent) :
    QObject(parent)
{
    if (static_binaryFormat == QSettings::InvalidFormat) {
        static_binaryFormat = QSettings::registerFormat("fglaunch",
                                                        &binaryReadFunc,
                                                        &binaryWriteFunc);
    }
}

LaunchConfig::~LaunchConfig()
{

}

void LaunchConfig::reset()
{
    m_values.clear();
}

void LaunchConfig::applyToOptions() const
{
	const auto extraArgs = extraArgNames();
    flightgear::Options* options = flightgear::Options::sharedInstance();
    std::for_each(m_values.begin(), m_values.end(),
                  [options, &extraArgs](const Arg& arg)
    {
        const auto name = arg.arg.toStdString();
        if (arg.origin == Launcher) {
            auto it = extraArgs.find(name);
            if (it != extraArgs.end()) {
                return;
            }
        }
        options->addOption(name, arg.value.toStdString());
    });
}

std::set<std::string> LaunchConfig::extraArgNames() const
{
	// build a set of all the extra args we have defined
	std::set<std::string> r;
	for (auto arg : m_values) {
		// don't override prop: arguments
		if (arg.arg == "prop") continue;

        // allow some multi-valued arguments
        if (arg.arg == "fg-scenery")
        {
            continue;
        }

		if (arg.origin == ExtraArgs)
			r.insert(arg.arg.toStdString());
	}
	return r;
}

void LaunchConfig::setArg(QString name, QString value, Origin origin)
{
    m_values.push_back(Arg(name, value, origin));
}

void LaunchConfig::setArg(const std::string &name, const std::string &value)
{
    setArg(QString::fromStdString(name), QString::fromStdString(value), Launcher);
}

void LaunchConfig::setProperty(QString path, QVariant value, Origin origin)
{
    m_values.push_back(Arg("prop", path + "=" + value.toString(), origin));
}

void LaunchConfig::setEnableDisableOption(QString name, bool value)
{
    m_values.push_back(Arg((value ? "enable-" : "disable-") + name, "", Launcher));
}

QString LaunchConfig::htmlForCommandLine()
{
    QString html;
    string_list commandLineOpts = flightgear::Options::sharedInstance()->extractOptions();
    if (!commandLineOpts.empty()) {
        html += tr("<p>Options passed on the command line:</p>\n");
        html += "<ul>\n";
        for (auto opt : commandLineOpts) {
            html += QString("<li>--") + QString::fromStdString(opt) + "</li>\n";
        }
        html += "</ul>\n";
    }

    reset();
    collect();

	const auto extraArgs = extraArgNames();

    html += tr("<p>Options set in the launcher:</p>\n");
    html += "<ul>\n";
    for (auto arg : valuesFromLauncher()) {
		auto it = extraArgs.find(arg.arg.toStdString());
		html += "<li>";
		bool strikeThrough = (it != extraArgs.end());
		if (strikeThrough) {
			html += "<i>";
		}
        if (arg.value.isEmpty()) {
			html += QString("--") + arg.arg;
        } else if (arg.arg == "prop") {
            html += QString("--") + arg.arg + ":" + arg.value;
        } else {
            html += QString("--") + arg.arg + "=" + arg.value;
        }
		if (strikeThrough) {
			html += tr(" (will be skipped due to being specified as an additional argument)") + "</i> ";
		}
		html += "</li>\n";
    }
    html += "</ul>\n";

    html += tr("<p>Options set as additional arguments:</p>\n");
    html += "<ul>\n";
    for (auto arg : valuesFromExtraArgs()) {
        if (arg.value.isEmpty()) {
            html += QString("<li>--") + arg.arg + "</li>\n";
        } else if (arg.arg == "prop") {
            html += QString("<li>--") + arg.arg + ":" + arg.value + "</li>\n";
        } else {
            html += QString("<li>--") + arg.arg + "=" + arg.value + "</li>\n";
        }
    }
    html += "</ul>\n";

    return html;
}

static QString formatArgForClipboard(const LaunchConfig::Arg& a)
{
    if (a.value.isEmpty()) {
        return "--" + a.arg + " ";
    } else if (a.arg == "prop") {
        return "--" + a.arg + ":" + a.value + " ";
    } else {
        return "--" + a.arg + "=" + a.value + " ";
    }
}

void LaunchConfig::copyCommandLine()
{
    QString r;

    for (auto opt : flightgear::Options::sharedInstance()->extractOptions()) {
        r += "--" + QString::fromStdString(opt) + " ";
    }

    reset();
    collect();
    const auto extraArgs = extraArgNames();

    for (auto arg : valuesFromLauncher()) {
        auto it = extraArgs.find(arg.arg.toStdString());
        if (it != extraArgs.end()) {
            continue; // skipped due to extra arg overriding
        }

        r += formatArgForClipboard(arg);
    }

    for (auto arg : valuesFromExtraArgs()) {
        r += formatArgForClipboard(arg);
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(r);
}

bool LaunchConfig::saveConfigToINI()
{
    // create settings using default type (INI) and path (inside FG_HOME),
    // as setup in initQSettings()
    m_loadSaveSettings.reset(new QSettings);
    emit save();
    m_loadSaveSettings->sync();
    m_loadSaveSettings.reset();

    return true;
}

bool LaunchConfig::loadConfigFromINI()
{
    // create settings using default type (INI) and path (inside FG_HOME),
    // as setup in initQSettings()
    m_loadSaveSettings.reset(new QSettings);
    emit restore();
    emit postRestore();
    m_loadSaveSettings.reset();
    return true;
}

bool LaunchConfig::saveConfigToFile(QString path)
{
    m_loadSaveSettings.reset(new QSettings(path, static_binaryFormat));
    emit save();
    m_loadSaveSettings.reset();
    return true;
}

bool LaunchConfig::loadConfigFromFile(QString path)
{
    m_loadSaveSettings.reset(new QSettings(path, static_binaryFormat));
    emit restore();
    // some things have an ordering dependency, give them a chance to run
    // after other settings have been restored (eg, location or aircraft)
    emit postRestore();
    m_loadSaveSettings.reset();
    return true;
}

QVariant LaunchConfig::getValueForKey(QString group, QString key, QVariant defaultValue) const
{
    if (!m_loadSaveSettings) {
        // becuase we load settings on component completion, we need
        // to create the default implementation (using the INI file)
        // on demand
        m_loadSaveSettings.reset(new QSettings);
    }

    m_loadSaveSettings->beginGroup(group);
    auto v = m_loadSaveSettings->value(key, defaultValue);
    bool convertedOk = v.convert(static_cast<int>(defaultValue.type()));
    if (!convertedOk) {
        qWarning() << "type forcing on loaded value failed:" << key << v << v.typeName() << defaultValue;
    }
  //  qInfo() << Q_FUNC_INFO << key << "value" << v << v.typeName() << convertedOk;
    return v;
}

void LaunchConfig::setValueForKey(QString group, QString key, QVariant var)
{
    Q_ASSERT(m_loadSaveSettings);
    m_loadSaveSettings->beginGroup(group);
  //  qInfo() << "saving" << key << "with value" << var << var.typeName();
    m_loadSaveSettings->setValue(key, var);
    m_loadSaveSettings->endGroup();
}

QString LaunchConfig::defaultDownloadDir() const
{
    return QString::fromStdString(flightgear::defaultDownloadDir().utf8Str());
}

bool LaunchConfig::enableDownloadDirUI() const
{
    return static_enableDownloadDirUI;
}

void LaunchConfig::setEnableDownloadDirUI(bool enableDownloadDirUI)
{
    static_enableDownloadDirUI = enableDownloadDirUI;
}

auto LaunchConfig::values() const -> std::vector<Arg>
{
    return m_values;
}

auto LaunchConfig::valuesFromLauncher() const -> std::vector<Arg>
{
    std::vector<Arg> result;
    std::copy_if(m_values.begin(), m_values.end(), std::back_inserter(result), [](const Arg& a)
    { return a.origin == Launcher; });
    return result;
}

auto LaunchConfig::valuesFromExtraArgs() const -> std::vector<Arg>
{
    std::vector<Arg> result;
    std::copy_if(m_values.begin(), m_values.end(), std::back_inserter(result), [](const Arg& a)
    { return a.origin == ExtraArgs; });
    return result;
}
