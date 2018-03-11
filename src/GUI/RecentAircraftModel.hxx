#ifndef RECENTAIRCRAFTMODEL_HXX
#define RECENTAIRCRAFTMODEL_HXX

#include <QAbstractListModel>
#include <QUrl>

// forward decls
class AircraftItemModel;

class RecentAircraftModel : public QAbstractListModel
{
    Q_OBJECT
public:
    RecentAircraftModel(AircraftItemModel *acModel, QObject* pr = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent) const override;

    QHash<int, QByteArray> roleNames() const override;

    QUrl mostRecent() const;

    void insert(QUrl aircraftUrl);

    void saveToSettings();

    Q_INVOKABLE QUrl uriAt(int index) const;
private:
    AircraftItemModel* m_aircraftModel;
    QList<QUrl> m_data;
};

#endif // RECENTAIRCRAFTMODEL_HXX
