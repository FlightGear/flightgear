#ifndef MPSERVERSMODEL_H
#define MPSERVERSMODEL_H

#include <QAbstractListModel>

#include <Network/RemoteXMLRequest.hxx>


class MPServersModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

    Q_PROPERTY(QString currentServer READ currentServer NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentPort READ currentPort NOTIFY currentIndexChanged)
public:
    MPServersModel(QObject* parent = nullptr);
    ~MPServersModel() override;

    int rowCount(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    void onRefreshMPServersDone(simgear::HTTP::Request*);
    void onRefreshMPServersFailed(simgear::HTTP::Request*);
    int findMPServerPort(const std::string& host);
    void restoreMPServerSelection();

    void refresh();

    void requestRestore();

    QString currentServer() const;
    int currentPort() const;

    bool valid() const;

    int currentIndex() const
    {
        return m_currentIndex;
    }

public slots:
    void setCurrentIndex(int currentIndex);

signals:
    void validChanged();
    void currentIndexChanged(int currentIndex);

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
    int m_currentIndex = 0;
};

#endif // MPSERVERSMODEL_H
