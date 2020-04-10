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

    if (!m_stringsModel.empty()) {
        if ((m_index < 0) || (m_index >= m_stringsModel.size()))
            return {};

        return m_stringsModel.at(m_index);
    }

    if (m_rawModel.isArray()) {
        quint32 uIndex = static_cast<quint32>(m_index);
        auto v = m_rawModel.property(uIndex);
        if (v.isQObject()) {
            // handle the QList<QObject*> case
            auto obj = v.toQObject();
            return obj->property(m_role.toUtf8().constData());
        }

        return m_rawModel.property(uIndex).toVariant();
    }

    if (!m_rawModel.isUndefined() && !m_rawModel.isNull()) {
        qWarning() << "Unable to convert model data:" << m_rawModel.toString();
    }

    return {};
}

void ModelDataExtractor::clear()
{
    if (m_model) {
        // disconnect from everything
        disconnect(m_model, nullptr, this, nullptr);
        m_model = nullptr;
    }

}

void ModelDataExtractor::setModel(QJSValue raw)
{
    if (m_rawModel.strictlyEquals(raw))
        return;

    clear();
    m_rawModel = raw;

    if (raw.isQObject()) {
        m_model = qobject_cast<QAbstractItemModel*>(raw.toQObject());
        if (m_model) {
            connect(m_model, &QAbstractItemModel::modelReset,
                    this, &ModelDataExtractor::dataChanged);
            connect(m_model, &QAbstractItemModel::dataChanged,
                    this, &ModelDataExtractor::onDataChanged);

            // ToDo: handle rows added / removed
        } else {
            qWarning() << "object but not a QAIM" << raw.toQObject();
        }
    } else if (raw.isArray()) {

    } else if (raw.isVariant() || raw.isObject()) {
        // special case the QStringList case

        // for reasons I don't understand yet, QStringList returned as a
        // property value to JS, does not show up as a variant-in-JS-Value above
        // (so ::isVariant returns false), but conversion to a variant
        // works. Hence the 'raw.isObject' above

        const auto var = raw.toVariant();
        if (var.type() == QVariant::StringList) {
            m_stringsModel = var.toStringList();
        } else {
            qWarning() << Q_FUNC_INFO << "variant but not a QStringList" << var;
        }
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
