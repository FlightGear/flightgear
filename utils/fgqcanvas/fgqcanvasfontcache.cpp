//
// Copyright (C) 2017 James Turner  zakalawe@mac.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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

void FGQCanvasFontCache::setHost(QString hostName, int portNumber)
{
    m_hostName = hostName;
    m_port = portNumber;
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
    if (!path.isEmpty()) {
        qDebug() << "found font" << name << "at path" << path;

        int fontFamilyId = QFontDatabase::addApplicationFont(path);
        if (fontFamilyId >= 0) {
            QStringList families = QFontDatabase::applicationFontFamilies(fontFamilyId);
            qDebug() << "families are:" << families;

            // compute a QFont and cache
            QFont font(families.front());
            m_cache.insert(name, font);
            return;
        } else {
            qWarning() << "Failed to load font into QFontDatabase:" << path;
        }
    }

    if (m_hostName.isEmpty()) {
        qWarning() << "host name not specified";
        return;
    }

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostName);
    url.setPort(m_port);
    url.setPath("/Fonts/" + name);

    Q_FOREACH (QNetworkReply* transfer, m_transfers) {
        if (transfer->url() == url) {
            return; // transfer already active
        }
    }

    qDebug() << "reqeusting font" << url;
    QNetworkReply* reply = m_downloader->get(QNetworkRequest(url));
    reply->setProperty("font", name);

    connect(reply, &QNetworkReply::finished, this, &FGQCanvasFontCache::onFontDownloadFinished);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
            this, SLOT(onFontDownloadError(QNetworkReply::NetworkError)));
    m_transfers.append(reply);
}


