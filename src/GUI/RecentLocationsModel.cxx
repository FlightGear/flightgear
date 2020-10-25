#include "RecentLocationsModel.hxx"

#include <QSettings>
#include <QDebug>


const int MAX_RECENT_LOCATIONS = 20;

// avoid clashng with previous versions, with incompatible data
const QString recentLocationsKey = "recent-locations-2020";

RecentLocationsModel::RecentLocationsModel(QObject* pr) :
    QAbstractListModel(pr)
{
    QSettings settings;
    m_data = settings.value(recentLocationsKey).toList();
}

void RecentLocationsModel::saveToSettings()
{
    QSettings settings;
    settings.setValue(recentLocationsKey, m_data);
}

QVariantMap RecentLocationsModel::locationAt(int index) const
{
    return m_data.at(index).toMap();
}

bool RecentLocationsModel::isEmpty() const
{
    return m_data.empty();
}

QVariant RecentLocationsModel::data(const QModelIndex &index, int role) const
{
    const QVariantMap loc = m_data.at(index.row()).toMap();
    if (role == Qt::DisplayRole) {
        return loc.value("text");
    } else if (role == Qt::UserRole) {
        return loc;
    }

    return {};
}

int RecentLocationsModel::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

QHash<int, QByteArray> RecentLocationsModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();
    result[Qt::DisplayRole] = "display";
   // result[Qt::UserRole] = "uri";
    return result;
}

QVariantMap RecentLocationsModel::mostRecent() const
{
    if (m_data.empty()) {
        return {};
    }

    return m_data.front().toMap();
}

void RecentLocationsModel::insert(QVariant location)
{
    if (location.toMap().isEmpty())
        return;

    QVariant locDesc = location.toMap().value("text");
    auto it = std::find_if(m_data.begin(), m_data.end(),
                   [locDesc](QVariant v) { return v.toMap().value("text") == locDesc; });
    if (!m_data.empty() && (it == m_data.begin())) {
        // special, common case - nothing to do
        // we use the description to determine equality,
        // but it doesn't mention altitude/speed/heading so always
        // update the actual stored value
        *it = location;
        return;
    }

    if (it != m_data.end()) {
        int existingIndex = std::distance(m_data.begin(), it);
        beginRemoveRows(QModelIndex(), existingIndex, existingIndex);
        m_data.erase(it);
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_data.push_front(location);
    endInsertRows();

    if (m_data.size() > MAX_RECENT_LOCATIONS) {
        beginRemoveRows(QModelIndex(), MAX_RECENT_LOCATIONS, m_data.size() - 1);
        // truncate the data at the correct size
        m_data = m_data.mid(0, MAX_RECENT_LOCATIONS);
        endRemoveRows();
    }

    emit isEmptyChanged();
}
