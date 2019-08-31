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

#include "applicationcontroller.h"

#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDataStream>
#include <QWindow>
#include <QTimer>
#include <QGuiApplication>
#include <QSettings>
#include <QQuickView>
#include <QQmlContext>

#include "jsonutils.h"
#include "canvasconnection.h"
#include "WindowData.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
    , m_status(Idle)
{
    m_netAccess = new QNetworkAccessManager;

    QSettings settings;
    m_host = settings.value("last-host", "localhost").toString();
    m_port = settings.value("last-port", 8080).toUInt();

    QNetworkDiskCache* cache = new QNetworkDiskCache;
    cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_netAccess->setCache(cache); // takes ownership

    setStatus(Idle);
    rebuildConfigData();
    rebuildSnapshotData();

    m_uiIdleTimer = new QTimer(this);
    m_uiIdleTimer->setInterval(10 * 1000);
    connect(m_uiIdleTimer, &QTimer::timeout, this,
            &ApplicationController::onUIIdleTimeout);
    m_uiIdleTimer->start();

    qApp->installEventFilter(this);
}

ApplicationController::~ApplicationController()
{
    delete m_netAccess;
}

void ApplicationController::loadFromFile(QString path)
{
    if (!QFile::exists(path)) {
        qWarning() << Q_FUNC_INFO << "no such file:" << path;
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "failed to open" << path;
        return;
    }

    restoreState(f.readAll());
}

void ApplicationController::setDaemonMode()
{
    m_daemonMode = true;
}

void ApplicationController::createWindows()
{
    if (m_windowList.empty()) {
        defineDefaultWindow();
    }

    for (int index = 0; index < m_windowList.size(); ++index) {
        auto wd = m_windowList.at(index);
        QQuickView* qqv = new QQuickView;
        qqv->rootContext()->setContextProperty("_application", this);
        qqv->rootContext()->setContextProperty("_windowNumber", index);
        qqv->setResizeMode(QQuickView::SizeRootObjectToView);
        qqv->setSource(QUrl{"qrc:///qml/Window.qml"});
        qqv->setTitle(wd->title());

        if (m_daemonMode) {
            qqv->setScreen(wd->screen());
            qqv->setGeometry(wd->windowRect());
            qqv->setWindowState(wd->windowState());
        } else {
            // interactive mode, restore window size etc

        }

        qqv->show();
    }
}

void ApplicationController::defineDefaultWindow()
{
    auto w = new WindowData(this);
    w->setWindowRect(QRect{0, 0, 1024, 768});
    m_windowList.append(w);
    emit windowListChanged();
}

void ApplicationController::save(QString configName)
{
    QDir d(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!d.exists()) {
        d.mkpath(".");
    }

    // convert spaces to underscores
    QString filesystemCleanName = configName.replace(QRegularExpression("[\\s-\\\"/]"), "_");

    QFile f(d.filePath(filesystemCleanName + ".json"));
    if (f.exists()) {
        qWarning() << "not over-writing" << f.fileName();
        return;
    }

    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(saveState(configName));

    QVariantMap m;
    m["path"] = f.fileName();
    m["name"] = configName;
    m_configs.append(m);
    emit configListChanged(m_configs);
}

void ApplicationController::rebuildConfigData()
{
    m_configs.clear();
    QDir d(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!d.exists()) {
        emit configListChanged(m_configs);
        return;
    }

    // this requires parsing each config in its entirety just to extract
    // the name, which is horrible.
    Q_FOREACH (auto entry, d.entryList(QStringList() << "*.json")) {
        QString path = d.filePath(entry);
        QFile f(path);
        f.open(QIODevice::ReadOnly);
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());

        QVariantMap m;
        m["path"] = path;
        m["name"] = doc.object().value("configName").toString();
        m_configs.append(m);
    }

    emit configListChanged(m_configs);
}

void ApplicationController::saveSnapshot(QString snapshotName)
{
    QDir d(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    d.cd("Snapshots");
    if (!d.exists()) {
        d.mkpath(".");
    }

    // convert spaces to underscores
    QString filesystemCleanName = snapshotName.replace(QRegularExpression("[\\s-\\\"/]"), "_");
    QFile f(d.filePath(filesystemCleanName + ".fgcanvassnapshot"));
    if (f.exists()) {
        qWarning() << "not over-writing" << f.fileName();
        return;
    }

    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(createSnapshot(snapshotName));

    QVariantMap m;
    m["path"] = f.fileName();
    m["name"] = snapshotName;
    m_snapshots.append(m);
    emit snapshotListChanged();
}

void ApplicationController::restoreSnapshot(int index)
{
    QString path = m_snapshots.at(index).toMap().value("path").toString();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "failed to open the file";
        return;
    }

    clearConnections();

    {
        QDataStream ds(&f);
        int version, canvasCount;
        QString name;
        ds >> version >> name >> canvasCount;

        for (int i=0; i < canvasCount; ++i) {
            CanvasConnection* cc = new CanvasConnection(this);
            cc->restoreSnapshot(ds);
            m_activeCanvases.append(cc);
        }
    }

    emit activeCanvasesChanged();
}

void ApplicationController::rebuildSnapshotData()
{
    m_snapshots.clear();
    QDir d(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    d.cd("Snapshots");
    if (!d.exists()) {
        emit snapshotListChanged();
        return;
    }

    Q_FOREACH (auto entry, d.entryList(QStringList() << "*.fgcanvassnapshot")) {
        QFile f(d.filePath(entry));
        f.open(QIODevice::ReadOnly);
        {
            QDataStream ds(&f);
            int version;
            QString name;
            ds >> version;

            QVariantMap m;
            m["path"] = f.fileName();
            ds >>name;
            m["name"] = name;
            m_snapshots.append(m);
        }
    }

    emit snapshotListChanged();
}

void ApplicationController::query()
{
    if (m_query) {
        cancelQuery();
    }

    if (m_host.isEmpty() || (m_port == 0))
        return;

    QSettings settings;
    settings.setValue("last-host", m_host);
    settings.setValue("last-port", m_port);

    QUrl queryUrl;
    queryUrl.setScheme("http");
    queryUrl.setHost(m_host);
    queryUrl.setPort(static_cast<int>(m_port));
    queryUrl.setPath("/json/canvas/by-index");
    queryUrl.setQuery("d=2");

    m_query = m_netAccess->get(QNetworkRequest(queryUrl));
    connect(m_query, &QNetworkReply::finished,
            this, &ApplicationController::onFinishedGetCanvasList);

    setStatus(Querying);
}

void ApplicationController::cancelQuery()
{
    setStatus(Idle);
    if (m_query) {
        m_query->abort();
        m_query->deleteLater();
    }

    m_query = nullptr;
    m_canvases.clear();
    emit canvasListChanged();
}

void ApplicationController::clearQuery()
{
    cancelQuery();
}

void ApplicationController::restoreConfig(int index)
{
    QString path = m_configs.at(index).toMap().value("path").toString();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "failed to open the file";
        return;
    }

    restoreState(f.readAll());
}

void ApplicationController::deleteConfig(int index)
{
    QString path = m_configs.at(index).toMap().value("path").toString();
    QFile f(path);
    if (!f.remove()) {
        qWarning() << "failed to remove file";
        return;
    }

    m_configs.removeAt(index);
    emit configListChanged(m_configs);
}

void ApplicationController::saveConfigChanges(int index)
{
    QString path = m_configs.at(index).toMap().value("path").toString();
    QString name = m_configs.at(index).toMap().value("name").toString();
    doSaveToFile(path, name);
}

void ApplicationController::doSaveToFile(QString path, QString configName)
{
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(saveState(configName));
}

void ApplicationController::openCanvas(QString path)
{
    CanvasConnection* cc = new CanvasConnection(this);

    cc->setNetworkAccess(m_netAccess);
    m_activeCanvases.append(cc);

    cc->setRootPropertyPath(path.toUtf8());
    cc->connectWebSocket(m_host.toUtf8(), m_port);

    emit activeCanvasesChanged();
}

void ApplicationController::closeCanvas(CanvasConnection *canvas)
{
    Q_ASSERT(m_activeCanvases.indexOf(canvas) >= 0);
    m_activeCanvases.removeOne(canvas);
    canvas->deleteLater();
    emit activeCanvasesChanged();
}

QString ApplicationController::host() const
{
    return m_host;
}

unsigned int ApplicationController::port() const
{
    return m_port;
}

QVariantList ApplicationController::canvases() const
{
    return m_canvases;
}

QQmlListProperty<CanvasConnection> ApplicationController::activeCanvases()
{
    return QQmlListProperty<CanvasConnection>(this, m_activeCanvases);
}

QQmlListProperty<WindowData> ApplicationController::windowList()
{
    return QQmlListProperty<WindowData>(this, m_windowList);
}

QNetworkAccessManager *ApplicationController::netAccess() const
{
    return m_netAccess;
}

bool ApplicationController::showUI() const
{
    if (m_daemonMode)
        return false;

    if (m_blockUIIdle)
        return true;

    return m_showUI;
}

QString ApplicationController::gettingStartedText() const
{
    QFile f(":/doc/gettingStarted.html");
    f.open(QIODevice::ReadOnly);
    return QString::fromUtf8(f.readAll());
}

bool ApplicationController::showGettingStarted() const
{
    if (m_daemonMode) return false;
    QSettings settings;
    return settings.value("show-getting-started", true).toBool();
}

void ApplicationController::setHost(QString host)
{
    if (m_host == host)
        return;

    m_host = host;
    emit hostChanged(m_host);
    setStatus(Idle);
}

void ApplicationController::setPort(unsigned int port)
{
    if (m_port == port)
        return;

    m_port = port;
    emit portChanged(m_port);
    setStatus(Idle);
}

void ApplicationController::setShowGettingStarted(bool show)
{
    QSettings settings;
    if (settings.value("show-getting-started", true).toBool() == show)
        return;

    settings.setValue("show-getting-started", show);
    emit showGettingStartedChanged(show);
}

QJsonObject jsonPropNodeFindChild(QJsonObject obj, QByteArray name)
{
    Q_FOREACH (QJsonValue v, obj.value("children").toArray()) {
        QJsonObject vo = v.toObject();
        if (vo.value("name").toString() == name) {
            return vo;
        }
    }

    return QJsonObject();
}

void ApplicationController::onFinishedGetCanvasList()
{
    m_canvases.clear();
    QNetworkReply* reply = m_query;
    m_query = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        setStatus(QueryFailed);
        emit canvasListChanged();
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());

    QJsonArray canvasArray = json.object().value("children").toArray();
    Q_FOREACH (QJsonValue canvasValue, canvasArray) {
        QJsonObject canvas = canvasValue.toObject();
        QString canvasName = jsonPropNodeFindChild(canvas, "name").value("value").toString();
        QString propPath = canvas.value("path").toString();

        QVariantMap info;
        info["name"] = canvasName;
        info["path"] = propPath;
        m_canvases.append(info);
    }

    emit canvasListChanged();
    setStatus(SuccessfulQuery);
}

void ApplicationController::onUIIdleTimeout()
{
    m_showUI = false;
    emit showUIChanged();
}

void ApplicationController::setStatus(ApplicationController::Status newStatus)
{
    if (newStatus == m_status)
        return;

    m_status = newStatus;
    emit statusChanged(m_status);
}

QByteArray ApplicationController::saveState(QString name) const
{
    QJsonObject json;
    json["configName"] = name;

    QJsonArray canvases;
    Q_FOREACH (auto canvas, m_activeCanvases) {
        canvases.append(canvas->saveState());
    }

    json["canvases"] = canvases;

    QJsonArray windows;
    Q_FOREACH (auto w, m_windowList) {
        windows.append(w->saveState());
    }
    json["windows"] = windows;

    // background color?

    QJsonDocument doc;
    doc.setObject(json);
    return doc.toJson();
}

void ApplicationController::restoreState(QByteArray bytes)
{
    clearConnections();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes);
    QJsonObject json = jsonDoc.object();

    // clear windows
    Q_FOREACH(auto w, m_windowList) {
        w->deleteLater();
    }
    m_windowList.clear();

    for (auto w : json.value("windows").toArray()) {
        auto wd = new WindowData(this);
        m_windowList.append(wd);
        wd->restoreState(w.toObject());
    }

    if (m_windowList.isEmpty()) {
        // check for previous single-window data
        auto w = new WindowData(this);
        if (json.contains("window-rect")) {
            w->setWindowRect(jsonArrayToRect(json.value("window-rect").toArray()));
        }
        if (json.contains("window-state")) {
            w->setWindowState(static_cast<Qt::WindowState>(json.value("window-state").toInt()));
        }
        m_windowList.append(w);
    }

    for (auto c : json.value("canvases").toArray()) {
        auto cc = new CanvasConnection(this);
        if (m_daemonMode)
            cc->setAutoReconnect();

        cc->setNetworkAccess(m_netAccess);
        m_activeCanvases.append(cc);
        cc->restoreState(c.toObject());
        cc->reconnect();
    }

    emit windowListChanged();
    emit activeCanvasesChanged();
}

void ApplicationController::clearConnections()
{
    Q_FOREACH(auto c, m_activeCanvases) {
        c->deleteLater();
    }
    m_activeCanvases.clear();
    emit activeCanvasesChanged();
}

QByteArray ApplicationController::createSnapshot(QString name) const
{
    QByteArray bytes;
    const int version = 1;
    {
         QDataStream ds(&bytes, QIODevice::WriteOnly);
         ds << version << name;

         ds << m_activeCanvases.size();
         Q_FOREACH(auto c, m_activeCanvases) {
             c->saveSnapshot(ds);
         }
    }

    return bytes;
}

bool ApplicationController::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj);
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::TouchUpdate:
    case QEvent::MouseMove:
    case QEvent::TouchBegin:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        if (!m_showUI) {
            m_showUI = true;
            emit showUIChanged();
        } else {
            m_uiIdleTimer->start();
        }

        break;
    default:
        break;
    }

    return false; //process as normal
}
