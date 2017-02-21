#ifndef FG_GUI_LAUNCHCONFIG_HXX
#define FG_GUI_LAUNCHCONFIG_HXX

#include <QObject>
#include <QVariant>

namespace flightgear { class Options; }

class LaunchConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString defaultDownloadDir READ defaultDownloadDir CONSTANT)
public:
    class Arg
    {
    public:
        explicit Arg(QString k, QString v = QString()) : arg(k), value(v) {}

        QString arg;
        QString value;
    };


    LaunchConfig(QObject* parent = nullptr);

    void reset();
    void applyToOptions() const;

    std::vector<Arg> values() const;

    Q_INVOKABLE void setArg(QString name, QString value = QString());

    Q_INVOKABLE void setArg(const std::string& name, const std::string& value = std::string());

    Q_INVOKABLE void setProperty(QString path, QVariant value);

    Q_INVOKABLE void setEnableDisableOption(QString name, bool value);

    // ensure a property is /not/ set?

    // save and restore API?

    QString defaultDownloadDir() const;

signals:
    void collect();
    
private:
    std::vector<Arg> m_values;
    QString m_defaultDownloadDir;
};

#endif
