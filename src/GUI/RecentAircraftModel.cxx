#include "RecentAircraftModel.hxx"

#include <QSettings>
#include <QDebug>

#include "AircraftItemModel.hxx"

const int MAX_RECENT_AIRCRAFT = 20;

const QString recentAircraftKey = "recent-aircraft-2020";

RecentAircraftModel::RecentAircraftModel(AircraftItemModel* acModel, QObject* pr) :
    QAbstractListModel(pr),
    m_aircraftModel(acModel)
{
    QSettings settings;
    const QStringList urls = settings.value(recentAircraftKey).toStringList();
    m_data = QUrl::fromStringList(urls);

    connect(m_aircraftModel, &AircraftItemModel::contentsChanged,
            this, &RecentAircraftModel::onModelContentsChanged);
}

void RecentAircraftModel::saveToSettings()
{
    QSettings settings;
    settings.setValue(recentAircraftKey, QUrl::toStringList(m_data));
}

QUrl RecentAircraftModel::uriAt(int index) const
{
    return m_data.at(index);
}

bool RecentAircraftModel::isEmpty() const
{
    return m_data.empty();
}

QVariant RecentAircraftModel::data(const QModelIndex &index, int role) const
{
    const QUrl uri = m_data.at(index.row());
    if (role == Qt::DisplayRole) {
        return m_aircraftModel->nameForAircraftURI(uri);
    } else if (role == Qt::UserRole) {
        return uri;
    }

    return {};
}

int RecentAircraftModel::rowCount(const QModelIndex&) const
{
    return m_data.size();
}

QHash<int, QByteArray> RecentAircraftModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();
    result[Qt::DisplayRole] = "display";
    result[Qt::UserRole] = "uri";
    return result;
}

QUrl RecentAircraftModel::mostRecent() const
{
    if (m_data.empty()) {
        return {};
    }

    return m_data.front();
}

void RecentAircraftModel::insert(QUrl aircraftUrl)
{
    if (aircraftUrl.isEmpty())
        return;

    int existingIndex = m_data.indexOf(aircraftUrl);
    if (existingIndex == 0) {
        // special, common case - nothing to do
        return;
    }

    if (existingIndex >= 0) {
        beginRemoveRows(QModelIndex(), existingIndex, existingIndex);
        m_data.removeAt(existingIndex);
        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_data.push_front(aircraftUrl);
    endInsertRows();

    if (m_data.size() > MAX_RECENT_AIRCRAFT) {
        beginRemoveRows(QModelIndex(), MAX_RECENT_AIRCRAFT, m_data.size() - 1);
        // truncate the data at the correct size
        m_data = m_data.mid(0, MAX_RECENT_AIRCRAFT);
        endRemoveRows();
    }

    emit isEmptyChanged();
}

void RecentAircraftModel::onModelContentsChanged()
{
    emit dataChanged(index(0), index(m_data.size() - 1));
}
