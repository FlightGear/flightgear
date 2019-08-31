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

#ifndef CANVASCONNECTION_H
#define CANVASCONNECTION_H

#include <memory>

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QUrl>
#include <QRectF>
#include <QPointer>
#include <QTimer>

class LocalProp;
class QNetworkAccessManager;
class FGQCanvasImageLoader;
class FGQCanvasFontCache;
class QDataStream;

class CanvasConnection : public QObject
{
    Q_OBJECT

    Q_ENUMS(Status)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

// QML exposed versions of the destination rect
    Q_PROPERTY(QPointF origin READ origin WRITE setOrigin NOTIFY geometryChanged)
    Q_PROPERTY(QSizeF size READ size WRITE setSize NOTIFY geometryChanged)

    Q_PROPERTY(int windowIndex READ windowIndex WRITE setWindowIndex NOTIFY geometryChanged)

    Q_PROPERTY(QUrl webSocketUrl READ webSocketUrl NOTIFY webSocketUrlChanged)
    Q_PROPERTY(QString rootPath READ rootPath NOTIFY rootPathChanged)
public:
    explicit CanvasConnection(QObject *parent = nullptr);
    ~CanvasConnection();

    void setNetworkAccess(QNetworkAccessManager *dl);
    void setRootPropertyPath(QByteArray path);

    void setAutoReconnect();

    enum Status
    {
        NotConnected,
        Connecting,
        Connected,
        Closed,
        Reconnecting,
        Error,
        Snapshot  // offline mode, data from snapshot
    };

    Status status() const
    {
        return m_status;
    }

    QJsonObject saveState() const;
    bool restoreState(QJsonObject state);

    void saveSnapshot(QDataStream& ds) const;
    void restoreSnapshot(QDataStream &ds);

    void connectWebSocket(QByteArray hostName, int port);
    QPointF origin() const;

    QSizeF size() const;

    int windowIndex() const
    {
        return m_windowIndex;
    }

    void setWindowIndex(int index);

    LocalProp* propertyRoot() const;

    QUrl webSocketUrl() const
    {
        return m_webSocketUrl;
    }

    QString rootPath() const
    {
        return QString::fromUtf8(m_rootPropertyPath);
    }

    FGQCanvasImageLoader* imageLoader() const;

    FGQCanvasFontCache* fontCache() const;

public Q_SLOTS:
    void reconnect();

    // not on iOS / Android - requires widgets
    void showDebugTree();

    void setOrigin(QPointF center);

    void setSize(QSizeF size);

signals:
    void statusChanged(Status status);

    void geometryChanged();

    void rootPathChanged();

    void webSocketUrlChanged();

    void updated();
private Q_SLOTS:
    void onWebSocketConnected();
    void onTextMessageReceived(QString message);
    void onWebSocketClosed();

private:
    void setStatus(Status newStatus);
    LocalProp *propertyFromPath(QByteArray path) const;

    QUrl m_webSocketUrl;
    QByteArray m_rootPropertyPath;
    QRectF m_destRect;
    int m_windowIndex = 0;

    QWebSocket m_webSocket;
    QNetworkAccessManager* m_netAccess = nullptr;
    QTimer* m_reconnectTimer = nullptr;
    bool m_autoReconnect = false;

    std::unique_ptr<LocalProp> m_localPropertyRoot;
    QHash<int, QPointer<LocalProp>> idPropertyDict;
    Status m_status = NotConnected;

    mutable FGQCanvasImageLoader* m_imageLoader = nullptr;
    mutable FGQCanvasFontCache* m_fontCache = nullptr;
};

#endif // CANVASCONNECTION_H
