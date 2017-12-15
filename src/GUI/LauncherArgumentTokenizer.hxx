#ifndef LAUNCHERARGUMENTTOKENIZER_HXX
#define LAUNCHERARGUMENTTOKENIZER_HXX

#include <QString>
#include <QList>
#include <QObject>
#include <QJSValue>

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
public:
    LauncherArgumentTokenizer();


    Q_INVOKABLE QList<ArgumentToken> tokenize(QString in) const;

    QString argString() const
    {
        return m_argString;
    }

    QVariantList tokens() const;

public slots:
    void setArgString(QString argString);

signals:
    void argStringChanged(QString argString);

private:
    enum State {
        Start = 0,
        Key,
        Value,
        Quoted,
        Comment
    };

    QString m_argString;
};

#endif // LAUNCHERARGUMENTTOKENIZER_HXX
