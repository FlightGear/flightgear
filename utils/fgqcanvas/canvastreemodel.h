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

#ifndef CANVASTREEMODEL_H
#define CANVASTREEMODEL_H

#include <QAbstractItemModel>

#include "fgcanvasgroup.h"

class CanvasTreeModel : public QAbstractItemModel
{
public:
    CanvasTreeModel(FGCanvasGroup* root);

    FGCanvasElement* elementFromIndex(const QModelIndex& index) const;

protected:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;

    virtual QVariant data(const QModelIndex &index, int role) const override;

    virtual bool hasChildren(const QModelIndex &parent) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    virtual QModelIndex parent(const QModelIndex &child) const override;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
private:
    QModelIndex indexForGroup(FGCanvasGroup *group) const;

    void onGroupChildAdded();
    void onGroupChildRemoved(int index);

    FGCanvasGroup* _root;
};

#endif // CANVASTREEMODEL_H
