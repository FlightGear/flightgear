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

#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include <QObject>
#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QQmlListProperty>
#include <QVariantList>

class CanvasConnection;
class QWindow;
class QTimer;
class WindowData;

class ApplicationController : public QObject
{
    Q_OBJECT


    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(unsigned int port READ port WRITE setPort NOTIFY portChanged)

    Q_PROPERTY(QVariantList canvases READ canvases NOTIFY canvasListChanged)
    Q_PROPERTY(QVariantList configs READ configs NOTIFY configListChanged)
    Q_PROPERTY(QVariantList snapshots READ snapshots NOTIFY snapshotListChanged)


    Q_PROPERTY(QQmlListProperty<CanvasConnection> activeCanvases READ activeCanvases NOTIFY activeCanvasesChanged)
    Q_PROPERTY(QQmlListProperty<WindowData> windowList READ windowList NOTIFY windowListChanged)

    Q_ENUMS(Status)

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

    Q_PROPERTY(bool showUI READ showUI  NOTIFY showUIChanged)
    Q_PROPERTY(bool blockUIIdle READ blockUIIdle WRITE setBlockUIIdle NOTIFY blockUIIdleChanged)

    Q_PROPERTY(QString gettingStartedText READ gettingStartedText CONSTANT)
    Q_PROPERTY(bool showGettingStarted READ showGettingStarted WRITE setShowGettingStarted NOTIFY showGettingStartedChanged)
public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    void loadFromFile(QString path);

    void setDaemonMode();
    void createWindows();

    Q_INVOKABLE void query();
    Q_INVOKABLE void cancelQuery();
    Q_INVOKABLE void clearQuery();

    Q_INVOKABLE void save(QString configName);
    Q_INVOKABLE void restoreConfig(int index);
    Q_INVOKABLE void deleteConfig(int index);
    Q_INVOKABLE void saveConfigChanges(int index);

    Q_INVOKABLE void openCanvas(QString path);
    Q_INVOKABLE void closeCanvas(CanvasConnection* canvas);

    Q_INVOKABLE void saveSnapshot(QString snapshotName);
    Q_INVOKABLE void restoreSnapshot(int index);

    QString host() const;

    unsigned int port() const;

    QVariantList canvases() const;

    QQmlListProperty<CanvasConnection> activeCanvases();
    QQmlListProperty<WindowData> windowList();

    QNetworkAccessManager* netAccess() const;

    enum Status {
        Idle,
        Querying,
        SuccessfulQuery,
        QueryFailed
    };

    Status status() const
    {
        return m_status;
    }

    QVariantList configs() const
    {
        return m_configs;
    }

    QVariantList snapshots() const
    {
        return m_snapshots;
    }

    bool showUI() const;

    bool blockUIIdle() const
    {
        return m_blockUIIdle;
    }

    QString gettingStartedText() const;

    bool showGettingStarted() const;

signals:

    void hostChanged(QString host);

    void portChanged(unsigned int port);

    void activeCanvasesChanged();
    void windowListChanged();

    void canvasListChanged();
    void statusChanged(Status status);

    void configListChanged(QVariantList configs);

    void snapshotListChanged();

    void showUIChanged();
    void blockUIIdleChanged(bool blockUIIdle);

    void showGettingStartedChanged(bool showGettingStarted);

public slots:
    void setHost(QString host);

    void setPort(unsigned int port);

    void setBlockUIIdle(bool blockUIIdle)
    {
        if (m_blockUIIdle == blockUIIdle)
            return;

        m_blockUIIdle = blockUIIdle;
        emit blockUIIdleChanged(m_blockUIIdle);
    }

    void setShowGettingStarted(bool showGettingStarted);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onFinishedGetCanvasList();
    void onUIIdleTimeout();

private:
    void setStatus(Status newStatus);

    void rebuildConfigData();
    void rebuildSnapshotData();
    void clearConnections();

    void doSaveToFile(QString path, QString configName);

    QByteArray saveState(QString name) const;
    void restoreState(QByteArray bytes);

    QByteArray createSnapshot(QString name) const;

    void defineDefaultWindow();

    QString m_host;
    unsigned int m_port;
    QVariantList m_canvases;
    QList<CanvasConnection*> m_activeCanvases;
    QNetworkAccessManager* m_netAccess;
    Status m_status;
    QVariantList m_configs;
    QNetworkReply* m_query = nullptr;
    QVariantList m_snapshots;

    QList<WindowData*> m_windowList;

    bool m_daemonMode = false;
    bool m_showUI = true;
    bool m_blockUIIdle = false;
    QTimer* m_uiIdleTimer;
};

#endif // APPLICATIONCONTROLLER_H
