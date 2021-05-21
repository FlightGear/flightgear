#ifndef LAUNCHERARGUMENTTOKENIZER_HXX
#define LAUNCHERARGUMENTTOKENIZER_HXX

#include <set>

#include <QString>
#include <QList>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class ArgumentToken
{
public:
    explicit ArgumentToken(QString k, QString v = QString()) : arg(k), value(v) {}

    QString arg;
    QString value;
};

class LauncherArgumentTokenizer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString argString READ argString WRITE setArgString NOTIFY argStringChanged)
    Q_PROPERTY(QVariantList tokens READ tokens NOTIFY argStringChanged)

    Q_PROPERTY(bool valid READ isValid NOTIFY argStringChanged)
    Q_PROPERTY(bool havePositionalArgs READ havePositionalArgs NOTIFY argStringChanged)
    Q_PROPERTY(bool haveUnsupportedArgs READ haveUnsupportedArgs NOTIFY argStringChanged)
    Q_PROPERTY(bool haveAircraftArgs READ haveAircraftArgs NOTIFY argStringChanged)

public:
    LauncherArgumentTokenizer();


    QString argString() const
    {
        return m_argString;
    }

    QVariantList tokens() const;

    bool isValid() const;

    bool haveUnsupportedArgs() const;
    bool havePositionalArgs() const;
    bool haveAircraftArgs() const;
public slots:
    void setArgString(QString argString);

signals:
    void argStringChanged(QString argString);

private:
    bool haveArgsIn(const std::set<std::string> &args) const;

    void tokenize(QString in);

    enum State {
        Start = 0,
        Key,
        Value,
        Quoted,
        Comment
    };

    std::vector<ArgumentToken> m_tokens;
    QString m_argString;
    bool m_valid = false;
};

#endif // LAUNCHERARGUMENTTOKENIZER_HXX
