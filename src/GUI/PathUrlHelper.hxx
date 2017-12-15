#ifndef PATHURLHELPER_H
#define PATHURLHELPER_H

#include <QObject>
#include <QUrl>

class PathUrlHelper : public QObject
{
    Q_OBJECT
public:
    explicit PathUrlHelper(QObject *parent = nullptr);

    Q_INVOKABLE QString urlToLocalFilePath(QUrl url) const;

    Q_INVOKABLE QUrl urlFromLocalFilePath(QString path) const;
signals:

public slots:
};

#endif // PATHURLHELPER_H
