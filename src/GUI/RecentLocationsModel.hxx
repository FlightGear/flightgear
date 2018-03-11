#ifndef RECENTLOCATIONSMODEL_HXX
#define RECENTLOCATIONSMODEL_HXX

#include <QAbstractListModel>
#include <QvariantList>


class RecentLocationsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    RecentLocationsModel(QObject* pr = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent) const override;

    QHash<int, QByteArray> roleNames() const override;

    QVariantMap mostRecent() const;

    void insert(QVariant location);

    void saveToSettings();

    Q_INVOKABLE QVariantMap locationAt(int index) const;
private:
    QVariantList m_data;
};

#endif // RECENTLOCATIONSMODEL_HXX
