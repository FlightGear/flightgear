#include "LaunchConfig.hxx"

#include <Main/options.hxx>
#include <simgear/misc/sg_path.hxx>

#include <QSettings>
#include <QDebug>

static bool static_enableDownloadDirUI = true;

LaunchConfig::LaunchConfig(QObject* parent) :
    QObject(parent)
{
}

void LaunchConfig::reset()
{
    m_values.clear();
}

void LaunchConfig::applyToOptions() const
{
    flightgear::Options* options = flightgear::Options::sharedInstance();
    std::for_each(m_values.begin(), m_values.end(), [options](const Arg& arg)
                  {
                      options->addOption(arg.arg.toStdString(), arg.value.toStdString());
                  });
}

void LaunchConfig::setArg(QString name, QString value)
{
    m_values.push_back(Arg(name, value));
}

void LaunchConfig::setArg(const std::string &name, const std::string &value)
{
    setArg(QString::fromStdString(name), QString::fromStdString(value));
}

void LaunchConfig::setProperty(QString path, QVariant value)
{
    m_values.push_back(Arg("prop", path + "=" + value.toString()));
}

void LaunchConfig::setEnableDisableOption(QString name, bool value)
{
    m_values.push_back(Arg((value ? "enable-" : "disable-") + name));
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
#if 0
    if (m_extraSettings) {
        LauncherArgumentTokenizer tk;
        Q_FOREACH(auto arg, tk.tokenize(m_extraSettings->argsText())) {
    //        m_config->setArg(arg.arg, arg.value);
        }
    }
#endif
    reset();
    collect();

    html += tr("<p>Options set in the launcher:</p>\n");
    html += "<ul>\n";
    for (auto arg : values()) {
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

QVariant LaunchConfig::getValueForKey(QString group, QString key, QVariant defaultValue) const
{
    QSettings settings;
    settings.beginGroup(group);
    auto v = settings.value(key, defaultValue);
    bool convertedOk = v.convert(defaultValue.type());
    if (!convertedOk) {
        qWarning() << "type forcing on loaded value failed:" << key << v << v.typeName() << defaultValue;
    }
  //  qInfo() << Q_FUNC_INFO << key << "value" << v << v.typeName() << convertedOk;
    return v;
}

void LaunchConfig::setValueForKey(QString group, QString key, QVariant var)
{
    QSettings settings;
    settings.beginGroup(group);
  //  qInfo() << "saving" << key << "with value" << var << var.typeName();
    settings.setValue(key, var);
    settings.endGroup();
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
