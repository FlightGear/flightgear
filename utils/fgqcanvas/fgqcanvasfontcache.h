#ifndef FGQCANVASFONTCACHE_H
#define FGQCANVASFONTCACHE_H

#include <QObject>
#include <QFont>
#include <QDir>
#include <QHash>
#include <QNetworkReply>

class QNetworkAccessManager;

class FGQCanvasFontCache : public QObject
{
    Q_OBJECT
public:
    explicit FGQCanvasFontCache(QNetworkAccessManager* nam, QObject *parent = 0);

    QFont fontForName(QByteArray name, bool* ok = nullptr);

    static void initialise(QNetworkAccessManager* nam);
    static FGQCanvasFontCache* instance();

    void setHost(QString hostName, int portNumber);
signals:
    void fontLoaded(QByteArray name);

private slots:
    void onFontDownloadFinished();
    void onFontDownloadError(QNetworkReply::NetworkError);

private:
    QNetworkAccessManager* m_downloader;
    QHash<QByteArray, QFont> m_cache;
    QString m_hostName;
    int m_port;

    void lookupFile(QByteArray name);

    QList<QNetworkReply*> m_transfers;
};

#endif // FGQCANVASFONTCACHE_H
