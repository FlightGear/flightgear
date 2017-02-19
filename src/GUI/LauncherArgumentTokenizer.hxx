#ifndef LAUNCHERARGUMENTTOKENIZER_HXX
#define LAUNCHERARGUMENTTOKENIZER_HXX

#include <QString>
#include <QList>

class LauncherArgumentTokenizer
{
public:
    LauncherArgumentTokenizer();

    class Arg
    {
    public:
        explicit Arg(QString k, QString v = QString()) : arg(k), value(v) {}

        QString arg;
        QString value;
    };

    QList<Arg> tokenize(QString in) const;

private:
    enum State {
        Start = 0,
        Key,
        Value,
        Quoted,
        Comment
    };
};


#endif // LAUNCHERARGUMENTTOKENIZER_HXX
