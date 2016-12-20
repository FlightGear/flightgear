#include "fgqcanvasfontcache.h"

#include <QNetworkAccessManager>
#include <QStandardPaths>
#include <QDebug>
#include <QUrl>
#include <QNetworkReply>
#include <QFontDatabase>
#include <QFile>

FGQCanvasFontCache::FGQCanvasFontCache(QNetworkAccessManager* nam, QObject *parent)
    : QObject(parent)
    , m_downloader(nam)
{
}

QFont FGQCanvasFontCache::fontForName(QByteArray name, bool* ok)
{
    if (m_cache.contains(name)) {
        if (ok) {
            *ok = true;
        }
        return m_cache.value(name); // easy!
    }

    lookupFile(name);
    if (m_cache.contains(name)) {
        if (ok) {
            *ok = true;
        }
        return m_cache.value(name);
    }

    if (ok) {
        *ok = false;
    }

    return QFont(); // default font
}

static FGQCanvasFontCache* s_instance = nullptr;

void FGQCanvasFontCache::initialise(QNetworkAccessManager *nam)
{
    Q_ASSERT(s_instance == nullptr);
    s_instance = new FGQCanvasFontCache(nam);
}

FGQCanvasFontCache *FGQCanvasFontCache::instance()
{

    return s_instance;
}

void FGQCanvasFontCache::onFontDownloadFinished()
{
    QByteArray fontPath = sender()->property("font").toByteArray();
    qDebug() << "finished download of " << fontPath;

    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    QString absPath = cacheDir.absoluteFilePath(fontPath);

    QFileInfo finfo(fontPath);
    cacheDir.mkpath(finfo.dir().path());

    QFile f(absPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "failed to open cache file" << f.fileName();
    }

    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    Q_ASSERT(m_transfers.contains(reply));

    f.write(reply->readAll());
    f.close();

    m_transfers.removeOne(reply);

    // call ourselves again now it's cached;
    lookupFile(fontPath);
}

void FGQCanvasFontCache::onFontDownloadError(QNetworkReply::NetworkError)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    qWarning() << "font download failed:" << reply->errorString();
}

void FGQCanvasFontCache::lookupFile(QByteArray name)
{
    QString path = QStandardPaths::locate(QStandardPaths::CacheLocation, name);
    if (path.isEmpty()) {
        QUrl url = QUrl("http://localhost:8080/Fonts/" + name);

        Q_FOREACH (QNetworkReply* transfer, m_transfers) {
            if (transfer->url() == url) {
                return; // transfer already active
            }
        }

        qDebug() << "reqeusting font" << url;
        QNetworkReply* reply = m_downloader->get(QNetworkRequest(url));
        reply->setProperty("font", name);

        connect(reply, &QNetworkReply::finished, this, &FGQCanvasFontCache::onFontDownloadFinished);
       // connect(reply, &QNetworkReply::error, this, &FGQCanvasFontCache::onFontDownloadError);

        m_transfers.append(reply);
    } else {
        qDebug() << "found font" << name << "at path" << path;

        int fontFamilyId = QFontDatabase::addApplicationFont(path);
        QStringList families = QFontDatabase::applicationFontFamilies(fontFamilyId);
        qDebug() << "families are:" << families;

        // compute a QFont and cache
        QFont font(families.front());
        m_cache.insert(name, font);
    }
}


