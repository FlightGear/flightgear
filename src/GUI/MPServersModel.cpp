#include "MPServersModel.h"

#include <QDebug>
#include <QSettings>

#include <Network/RemoteXMLRequest.hxx>
#include <Network/HTTPClient.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "LaunchConfig.hxx"

const int IsCustomIndexRole = Qt::UserRole + 1;
const int ServerNameRole = Qt::UserRole + 2;

MPServersModel::MPServersModel(QObject* parent) :
    QAbstractListModel(parent)
{

}

MPServersModel::~MPServersModel()
{
    // if we don't cancel this now, it may complete after we are gone,
    // causing a crash when the SGCallback fires (SGCallbacks don't clean up
    // when their subject is deleted)
    globals->get_subsystem<FGHTTPClient>()->client()->cancelRequest(m_mpServerRequest);
}

int MPServersModel::rowCount(const QModelIndex&) const
{
    // if query failed, we have two item:
    // 1) - 'no servers found'
    // 2) - 'custom server'
    if (m_servers.empty())
        return 2;

    return m_servers.size() + 1;
}

QVariant MPServersModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    const int customServerRow = (m_servers.empty() ? 1 : m_servers.size());

    if ((row == 0) && m_servers.empty()) {
        if (role == Qt::DisplayRole) {
            return tr("No servers available");
        }

        return QVariant();
    }

    if (row == customServerRow) {
        if (role == Qt::DisplayRole) {
            return tr("Custom server");
        } else if (role == IsCustomIndexRole) {
            return true;
        }

        return QVariant();
    }

    const ServerInfo& sv(m_servers.at(row));
    if (role == Qt::DisplayRole) {
        return tr("%1 - %2").arg(sv.name).arg(sv.location);
    } else if (role == ServerNameRole) {
        return sv.name;
    } else if (role == IsCustomIndexRole) {
        return false;
    }

    return QVariant();
}

QHash<int, QByteArray> MPServersModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();
    result[IsCustomIndexRole] = "isCustomIndex";
    return result;
}

void MPServersModel::refresh()
{
    if (m_mpServerRequest.get()) {
        return; // in-progress
    }

    string url(fgGetString("/sim/multiplay/serverlist-url",
                           "http://liveries.flightgear.org/mpstatus/mpservers.xml"));

    if (url.empty()) {
        return;
    }

    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    m_mpServerRequest.reset(new RemoteXMLRequest(url, targetnode));
    m_mpServerRequest->done(this, &MPServersModel::onRefreshMPServersDone);
    m_mpServerRequest->fail(this, &MPServersModel::onRefreshMPServersFailed);
    globals->get_subsystem<FGHTTPClient>()->makeRequest(m_mpServerRequest);
}

void MPServersModel::onRefreshMPServersDone(simgear::HTTP::Request*)
{
    beginResetModel();
    // parse the properties
    SGPropertyNode *targetnode = fgGetNode("/sim/multiplay/server-list", true);
    m_servers.clear();

    for (int i=0; i<targetnode->nChildren(); ++i) {
        SGPropertyNode* c = targetnode->getChild(i);
        if (c->getName() != std::string("server")) {
            continue;
        }

        if (c->getBoolValue("online") != true) {
            // only list online servers
            continue;
        }

        QString name = QString::fromStdString(c->getStringValue("name"));
        QString loc = QString::fromStdString(c->getStringValue("location"));
        QString host = QString::fromStdString(c->getStringValue("hostname"));
        int port = c->getIntValue("port");
        m_servers.push_back(ServerInfo(name, loc, host, port));
    }
    endResetModel();
    emit validChanged();

    restoreMPServerSelection();
    m_mpServerRequest.clear();
}

void MPServersModel::onRefreshMPServersFailed(simgear::HTTP::Request*)
{
    qWarning() << "refreshing MP servers failed:" << QString::fromStdString(m_mpServerRequest->responseReason());
    m_mpServerRequest.clear();
    beginResetModel();
    m_servers.clear();
    endResetModel();
    emit validChanged();
    emit currentIndexChanged(m_currentIndex);
    restoreMPServerSelection();
}

void MPServersModel::restoreMPServerSelection()
{
    if (m_doRestoreMPServer) {
        QSettings settings;
        settings.beginGroup("mpSettings");
        QString host = settings.value("mp-server").toString();
        if (host == "__custom__") {
            setCurrentIndex(m_servers.size());
        } else {
            // restore a built-in server
            auto it = std::find_if(m_servers.begin(), m_servers.end(), [host](const ServerInfo& info)
            { return (info.host == host); });

            if (it != m_servers.end()) {
                setCurrentIndex(std::distance(m_servers.begin(), it));
            } else {
                setCurrentIndex(0);
            }
        }

        // force a refresh on the QML side, since the model may have changed
        emit currentIndexChanged(m_currentIndex);
        m_doRestoreMPServer = false;
    }
}

void MPServersModel::requestRestore()
{
    m_doRestoreMPServer = true;
}

QString MPServersModel::currentServer() const
{
    if (!valid()) {
        return (m_currentIndex == 1) ? "__custom__" : "__noservers__";
    }

    if (m_currentIndex == m_servers.size()) {
        return "__custom__";
    }

    return m_servers.at(m_currentIndex).host;
}

int MPServersModel::currentPort() const
{
    if (!valid())
        return 0;

    return m_servers.at(m_currentIndex).port;
}

bool MPServersModel::valid() const
{
    return !m_servers.empty();
}

void MPServersModel::setCurrentIndex(int currentIndex)
{
    if (m_currentIndex == currentIndex)
        return;

    m_currentIndex = currentIndex;
    emit currentIndexChanged(m_currentIndex);
}

MPServersModel::ServerInfo::ServerInfo(QString n, QString l, QString h, int p)
{
    name = n;
    location = l;
    host = h;
    port = p;
}
