#include "QmlStringListModel.hxx"

#include <algorithm>


QmlStringListModel::QmlStringListModel(QObject* parent) : QAbstractListModel(parent)
{
}

QmlStringListModel::~QmlStringListModel() = default;

void QmlStringListModel::setValues(QStringList v)
{
    beginResetModel();
    m_values = v;
    endResetModel();
}

QStringList QmlStringListModel::values() const
{
    return m_values;
}

int QmlStringListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_values.size();
}

QHash<int, QByteArray> QmlStringListModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    return roles;
}

QVariant QmlStringListModel::data(const QModelIndex& m, int role) const
{
    auto s = m_values.at(m.row());
    if (role == Qt::DisplayRole) {
        return s;
    }

    return {};
}
