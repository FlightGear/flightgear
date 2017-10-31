//
// Copyright (C) 2017 James Turner  zakalawe@mac.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
