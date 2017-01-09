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

#ifndef ELEMENTDATAMODEL_H
#define ELEMENTDATAMODEL_H

#include <QAbstractTableModel>

class FGCanvasElement;

class ElementDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ElementDataModel(QObject* pr);

    void setElement(FGCanvasElement* e);

    virtual int rowCount(const QModelIndex &parent) const override;

    virtual int columnCount(const QModelIndex &parent) const override;

    virtual QVariant data(const QModelIndex &index, int role) const override;


private:
    void computeKeys();

    FGCanvasElement* m_element;
    QList<QByteArray> m_keys;
};

#endif // ELEMENTDATAMODEL_H
