#include "PathListModel.hxx"

#include <QSettings>
#include <QDebug>

PathListModel::PathListModel(QObject *pr) :
    QAbstractListModel(pr)
{

}

PathListModel::~PathListModel()
{

}

void PathListModel::loadFromSettings(QString key)
{
    QSettings settings;
    if (!settings.contains(key))
        return;

    QVariantList vl = settings.value(key).toList();
    mPaths.clear();
    mPaths.reserve(static_cast<size_t>(vl.size()));

    beginResetModel();

    Q_FOREACH(QVariant v, vl) {
        QVariantMap m = v.toMap();
        PathEntry entry;
        entry.path = m.value("path").toString();
        if (entry.path.isEmpty()) {
            continue;
        }

        entry.enabled = m.value("enabled", QVariant{true}).toBool();
        mPaths.push_back(entry);
    }

    endResetModel();
    emit enabledPathsChanged();
    emit countChanged();
}

void PathListModel::saveToSettings(QString key) const
{
    QVariantList vl;
    for (const auto &e : mPaths) {
        QVariantMap v;
        v["path"] = e.path;
        v["enabled"] = e.enabled;
        vl.append(v);
    }

    QSettings settings;
    settings.setValue(key, vl);
}

QStringList PathListModel::readEnabledPaths(QString settingsKey)
{
    QSettings settings;
    if (!settings.contains(settingsKey))
        return {};

    QStringList result;
    QVariantList vl = settings.value(settingsKey).toList();
    Q_FOREACH(QVariant v, vl) {
        QVariantMap m = v.toMap();
        if (!m.value("enabled").toBool())
            continue;

        result.append(m.value("path").toString());
    }

    return result;
}

QStringList PathListModel::enabledPaths() const
{
    QStringList result;
    for (const auto& e : mPaths) {
        if (e.enabled) {
            result.append(e.path);
        }
    }
    return result;
}

int PathListModel::count()
{
    return static_cast<int>(mPaths.size());
}

int PathListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(mPaths.size());
}

QVariant PathListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    const auto& entry = mPaths.at(static_cast<size_t>(row));
    switch (role) {
    case Qt::DisplayRole:
    case PathRole:
        return entry.path;

    case PathEnabledRole:
        return entry.enabled;

    default:
        break;
    }

    return {};
}

bool PathListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    int row = index.row();
    auto& entry = mPaths.at(static_cast<size_t>(row));
    if (role == PathEnabledRole) {
        entry.enabled = value.toBool();
        emit dataChanged(index, index, {PathEnabledRole});
        emit enabledPathsChanged();
        return true;
    }

    return false;
}

QHash<int, QByteArray> PathListModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();
    result[Qt::DisplayRole] = "path";
    result[PathEnabledRole] = "enabled";
    return result;
}

void PathListModel::removePath(int index)
{
    if ((index < 0) || (index >= static_cast<int>(mPaths.size()))) {
        qWarning() << Q_FUNC_INFO << "index invalid:" << index;
        return;
    }

    beginRemoveRows({}, index, index);
    auto it = mPaths.begin() + index;
    mPaths.erase(it);
    endRemoveRows();
    emit enabledPathsChanged();
    emit countChanged();
}

void PathListModel::appendPath(QString path)
{
    PathEntry entry;
    entry.path = path;
    entry.enabled = true; // enable by default
    const int newRow = static_cast<int>(mPaths.size());
    beginInsertRows({}, newRow, newRow);
    mPaths.push_back(entry);
    endInsertRows();
    emit enabledPathsChanged();
    emit countChanged();
}

void PathListModel::swapIndices(int indexA, int indexB)
{
    if ((indexA < 0) || (indexA >= static_cast<int>(mPaths.size()))) {
        qWarning() << Q_FUNC_INFO << "index invalid:" << indexA;
        return;
    }

    if ((indexB < 0) || (indexB >= static_cast<int>(mPaths.size()))) {
        qWarning() << Q_FUNC_INFO << "index invalid:" << indexB;
        return;
    }

    std::swap(mPaths[static_cast<size_t>(indexA)],
              mPaths[static_cast<size_t>(indexB)]);
    emit dataChanged(index(indexA), index(indexA));
    emit dataChanged(index(indexB), index(indexB));
    emit enabledPathsChanged();
}

