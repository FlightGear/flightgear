#include "MPServersModel.h"

#include <QDebug>
#include <QSettings>

#include <Network/RemoteXMLRequest.hxx>
#include <Network/HTTPClient.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include "LaunchConfig.hxx"

const int IsCustomIndexRole = Qt::UserRole + 1;

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
    return m_servers.size() + 1;
}

QVariant MPServersModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if ((row < 0) || (row > m_servers.size())) {
        return QVariant();
    }

    if (row == m_servers.size()) {
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
    } else if (role == IsCustomIndexRole) {
        return false;
    }

    return QVariant();
}

QHash<int, QByteArray> MPServersModel::roleNames() const
{
    QHash<int, QByteArray> result;
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
    restoreMPServerSelection();
}

void MPServersModel::restoreMPServerSelection()
{
    if (m_doRestoreMPServer) {
        QSettings settings;
        settings.beginGroup("mpSettings");
        QString host = settings.value("mp-server").toString();
        if (host == "__custom__") {
            emit restoreIndex(m_servers.size());
        } else {
            // restore a built-in server
            auto it = std::find_if(m_servers.begin(), m_servers.end(), [host](const ServerInfo& info)
            { return (info.host == host); });

            if (it != m_servers.end()) {
                emit restoreIndex(std::distance(m_servers.begin(), it));
            }
        }

        m_doRestoreMPServer = false;
    }
}

void MPServersModel::requestRestore()
{
    m_doRestoreMPServer = true;
}

QString MPServersModel::serverForIndex(int index) const
{
    if ((index < 0) || (index > m_servers.size())) {
        return QString();
    }

    if (index == m_servers.size()) {
        return "__custom__";
    }

    return m_servers.at(index).host;
}

int MPServersModel::portForIndex(int index) const
{
    if ((index < 0) || (index >= m_servers.size())) {
        return 0;
    }

    return m_servers.at(index).port;
}

MPServersModel::ServerInfo::ServerInfo(QString n, QString l, QString h, int p)
{
    name = n;
    location = l;
    host = h;
    port = p;
}
