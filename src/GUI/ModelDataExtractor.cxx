#include "ModelDataExtractor.hxx"

#include <QAbstractItemModel>
#include <QDebug>

ModelDataExtractor::ModelDataExtractor(QObject *parent) : QObject(parent)
{

}

QVariant ModelDataExtractor::data() const
{
    if (m_model) {
        QModelIndex m = m_model->index(m_index, 0);
        if (!m.isValid())
            return {};

        int role = Qt::DisplayRole;
        if (!m_role.isEmpty()) {
            const auto& names = m_model->roleNames();
            role = names.key(m_role.toUtf8(), Qt::DisplayRole);
        }

        return m_model->data(m, role);
    }

    if (m_value.isArray()) {
        quint32 uIndex = static_cast<quint32>(m_index);
        auto v = m_value.property(uIndex);
        if (v.isQObject()) {
            // handle the QList<QObject*> case
            auto obj = v.toQObject();
            return obj->property(m_role.toUtf8().constData());
        }

        return m_value.property(uIndex).toVariant();
    }

    return {};
}

void ModelDataExtractor::setModel(QJSValue model)
{
    if (m_value.equals(model))
        return;

    if (m_model) {
        // disconnect from everything
        disconnect(m_model, nullptr, this, nullptr);
        m_model = nullptr;
    }

    m_value = model;
    if (m_value.isQObject()) {
        m_model = qobject_cast<QAbstractItemModel*>(m_value.toQObject());
        if (m_model) {
            connect(m_model, &QAbstractItemModel::modelReset,
                    this, &ModelDataExtractor::dataChanged);
            connect(m_model, &QAbstractItemModel::dataChanged,
                    this, &ModelDataExtractor::onDataChanged);

            // ToDo: handle rows added / removed
        } else {
            qWarning() << "object but not a QAIM" << m_value.toQObject();
        }
    } else {
        // might be null, or an array
    }

    emit modelChanged();
    emit dataChanged();
}

void ModelDataExtractor::setIndex(int index)
{
    if (m_index == index)
        return;

    m_index = index;
    emit indexChanged(m_index);
    emit dataChanged();
}

void ModelDataExtractor::setRole(QString role)
{
    if (m_role == role)
        return;

    m_role = role;
    emit roleChanged(m_role);
    emit dataChanged();
}

void ModelDataExtractor::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if ((topLeft.row() >= m_index) && (bottomRight.row() <= m_index)) {
        emit dataChanged();
    }
}
