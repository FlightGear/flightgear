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

#ifndef FGQCANVASIMAGELOADER_H
#define FGQCANVASIMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QMap>
#include <QNetworkReply>

class QNetworkAccessManager;

class FGQCanvasImageLoader : public QObject
{
    Q_OBJECT
public:
    FGQCanvasImageLoader(QNetworkAccessManager* dl, QObject* pr = nullptr);


    void setHost(QString hostName, int portNumber);

    QPixmap getImage(const QByteArray& imagePath);

    /**
     * @brief connectToImageLoaded - allow images to discover when they are loaded
     * @param imagePath - FGFS host relative path as found in the canvas (will be resolved
     * against aircraft-data, and potentially other places)
     * @param receiver - normal connect() receiver object
     * @param slot - slot macro
     */
    void connectToImageLoaded(const QByteArray& imagePath, QObject* receiver, const char* slot);
signals:


private:

    void onDownloadFinished();
    void writeToDiskCache(QByteArray imagePath, QNetworkReply *reply);

private:
    QNetworkAccessManager* m_downloader;
    QString m_hostName;
    int m_port;

    QMap<QByteArray, QPixmap> m_cache;
    QList<QNetworkReply*> m_transfers;

};

#endif // FGQCANVASIMAGELOADER_H
