#include "QmlPositionedModel.hxx"

#include <algorithm>

#include <QQmlEngine>

#include <Airports/parking.hxx>
#include <Airports/runways.hxx>

#include "QmlPositioned.hxx"

class QmlPositionedModel::QmlPositionedModelPrivate
{
public:
    FGPositionedList m_items;
};

QmlPositionedModel::QmlPositionedModel(QObject* parent) : QAbstractListModel(parent),
                                                          d(new QmlPositionedModelPrivate)
{
}

QmlPositionedModel::~QmlPositionedModel() = default;

void QmlPositionedModel::setValues(const FGPositionedList& items)
{
    beginResetModel();
    d->m_items = items;
    endResetModel();
    emit sizeChanged();
}

void QmlPositionedModel::setValues(const FGRunwayList& runways)
{
    FGPositionedList p;
    std::transform(runways.begin(), runways.end(), std::back_inserter(p), [](const FGRunwayRef& rwy) {
        return static_cast<FGPositioned*>(rwy.get());
    });
    setValues(p);
}

void QmlPositionedModel::setValues(const FGParkingList& parks)
{
    FGPositionedList p;
    std::transform(parks.begin(), parks.end(), std::back_inserter(p), [](const FGParkingRef& pk) {
        return static_cast<FGPositioned*>(pk.get());
    });
    setValues(p);
}

void QmlPositionedModel::clear()
{
    beginResetModel();
    d->m_items.clear();
    endResetModel();
    emit sizeChanged();
}

int QmlPositionedModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return d->m_items.size();
}

QHash<int, QByteArray> QmlPositionedModel::roleNames() const
{
    auto roles = QAbstractListModel::roleNames();
    return roles;
}

QVariant QmlPositionedModel::data(const QModelIndex& m, int role) const
{
    auto pos = d->m_items.at(m.row());
    if (role == Qt::DisplayRole) {
        return QString::fromStdString(pos->ident());
    }

    return {};
}

int QmlPositionedModel::indexOf(QmlPositioned* qpos) const
{
    auto it = std::find_if(d->m_items.begin(), d->m_items.end(), [qpos](const FGPositionedRef& pos) {
        return pos == qpos->inner();
    });

    if (it == d->m_items.end()) {
        return -1;
    }

    return static_cast<int>(std::distance(d->m_items.begin(), it));
}

QmlPositioned* QmlPositionedModel::itemAt(int index) const
{
    if ((index < 0) || (index >= d->m_items.size())) {
        return nullptr;
    }

    auto qp = new QmlPositioned(d->m_items.at(index));
    QQmlEngine::setObjectOwnership(qp, QQmlEngine::JavaScriptOwnership);
    return qp;
}

bool QmlPositionedModel::isEmpty() const
{
    return d->m_items.empty();
}
