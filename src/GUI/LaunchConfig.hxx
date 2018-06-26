#ifndef FG_GUI_LAUNCHCONFIG_HXX
#define FG_GUI_LAUNCHCONFIG_HXX

#include <set>
#include <QObject>
#include <QVariant>

namespace flightgear { class Options; }

class LaunchConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString defaultDownloadDir READ defaultDownloadDir CONSTANT)
    Q_PROPERTY(bool enableDownloadDirUI READ enableDownloadDirUI CONSTANT)
public:
    enum Origin
    {
        Launcher = 0,
        ExtraArgs
    };

    Q_ENUMS(Origin);

    class Arg
    {
    public:
        explicit Arg(QString k, QString v, Origin o) : arg(k), value(v), origin(o) {}

        QString arg;
        QString value;
        Origin origin;
    };


    LaunchConfig(QObject* parent = nullptr);

    void reset();
    void applyToOptions() const;

    std::vector<Arg> values() const;



    Q_INVOKABLE void setArg(QString name, QString value = QString(), Origin origin = Launcher);

    Q_INVOKABLE void setArg(const std::string& name, const std::string& value = std::string());

    Q_INVOKABLE void setProperty(QString path, QVariant value, Origin origin = Launcher);

    Q_INVOKABLE void setEnableDisableOption(QString name, bool value);

    Q_INVOKABLE QString htmlForCommandLine();


    // ensure a property is /not/ set?

    // save and restore API?

    Q_INVOKABLE QVariant getValueForKey(QString group, QString key, QVariant defaultValue = QVariant()) const;
    Q_INVOKABLE void setValueForKey(QString group, QString key, QVariant var);

    QString defaultDownloadDir() const;

    bool enableDownloadDirUI() const;

    static void setEnableDownloadDirUI(bool enableDownloadDirUI);

    std::vector<Arg> valuesFromLauncher() const;
    std::vector<Arg> valuesFromExtraArgs() const;

signals:
    void collect();
    
private:
	std::set<std::string> extraArgNames() const;

    std::vector<Arg> m_values;
    QString m_defaultDownloadDir;
};

#endif
