#include "LaunchConfig.hxx"

#include <Main/options.hxx>
#include <simgear/misc/sg_path.hxx>

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
