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
