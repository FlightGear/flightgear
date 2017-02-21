#ifndef MPSERVERSMODEL_H
#define MPSERVERSMODEL_H

#include <QAbstractListModel>

#include <Network/RemoteXMLRequest.hxx>


class MPServersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    MPServersModel(QObject* parent = nullptr);
    ~MPServersModel();

    int rowCount(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    void onRefreshMPServersDone(simgear::HTTP::Request*);
    void onRefreshMPServersFailed(simgear::HTTP::Request*);
    int findMPServerPort(const std::string& host);
    void restoreMPServerSelection();

    void refresh();

    void requestRestore();

    Q_INVOKABLE QString serverForIndex(int index) const;
    Q_INVOKABLE int portForIndex(int index) const;
signals:
    void restoreIndex(int index);

private:

    SGSharedPtr<RemoteXMLRequest> m_mpServerRequest;
    bool m_doRestoreMPServer = false;

    struct ServerInfo
    {
        ServerInfo(QString n, QString l, QString h, int port);

        QString name, location, host;
        int port = 0;
    };

    std::vector<ServerInfo> m_servers;
};

#endif // MPSERVERSMODEL_H
