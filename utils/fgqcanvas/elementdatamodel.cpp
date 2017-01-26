#include "elementdatamodel.h"

#include "fgcanvaselement.h"
#include "localprop.h"

ElementDataModel::ElementDataModel(QObject* pr)
    : QAbstractTableModel(pr)
    , m_element(nullptr)
{

}

void ElementDataModel::setElement(FGCanvasElement *e)
{
    beginResetModel();
    m_element = e;
    computeKeys();
    endResetModel();
}

int ElementDataModel::rowCount(const QModelIndex &parent) const
{
    return m_keys.size();
}

int ElementDataModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant ElementDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || (index.row() >= m_keys.size())) {
        return QVariant();
    }

    QByteArray key = m_keys.at(index.row());
    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return key;
        }

        if (key == "position") {
            return m_element->property()->position();
        }

        return m_element->property()->value(key.constData(), QVariant());
    }

    return QVariant();
}

void ElementDataModel::computeKeys()
{
    m_keys.clear();
    if (m_element == nullptr) {
        return;
    }

    LocalProp *prop = m_element->property();

    QByteArrayList directProps = QByteArrayList() << "fill" << "stroke" <<
                                                     "background" <<
                                                     "text" <<
                                                     "clip" << "file" << "src"
                                                     "font" << "character-size" <<
                                                     "z-index" << "visible";

    Q_FOREACH (QByteArray b, directProps) {
        if (prop->hasChild(b)) {
            m_keys.append(b);
        }
    }

    m_keys.append("position");
}
