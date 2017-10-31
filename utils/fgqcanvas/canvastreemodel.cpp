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

#include "canvastreemodel.h"

#include <QDebug>

#include "localprop.h"

CanvasTreeModel::CanvasTreeModel(FGCanvasGroup* root) :
    _root(root)
{
    connect(_root, &FGCanvasGroup::childAdded, this, &CanvasTreeModel::onGroupChildAdded);
}

FGCanvasElement* CanvasTreeModel::elementFromIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }

    FGCanvasElement* e = static_cast<FGCanvasElement*>(index.internalPointer());
    return e;
}

int CanvasTreeModel::rowCount(const QModelIndex &parent) const
{
    FGCanvasElement* e = static_cast<FGCanvasElement*>(parent.internalPointer());
    if (!e) {
        return _root->childCount();
    }

    FGCanvasGroup* group = qobject_cast<FGCanvasGroup*>(e);
    if (group) {
        return group->childCount();
    }

    return 0;
}

int CanvasTreeModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant CanvasTreeModel::data(const QModelIndex &index, int role) const
{
    FGCanvasElement* e = static_cast<FGCanvasElement*>(index.internalPointer());
    if (!e) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return e->property()->value("id", QVariant("<noid>"));

    case Qt::CheckStateRole:
        return e->property()->value("visible", true).toBool() ? Qt::Checked : Qt::Unchecked;

    default:
        break;
    }

    return QVariant();
}

bool CanvasTreeModel::hasChildren(const QModelIndex &parent) const
{
    FGCanvasElement* e;
    if (parent.isValid()) {
        e = static_cast<FGCanvasElement*>(parent.internalPointer());
    } else {
        e = _root;
    }

    FGCanvasGroup* group = qobject_cast<FGCanvasGroup*>(e);
    if (group) {
        return group->hasChilden();
    }

    return false;
}

QModelIndex CanvasTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    FGCanvasGroup* group;
    if (parent.isValid()) {
        group = qobject_cast<FGCanvasGroup*>(static_cast<FGCanvasElement*>(parent.internalPointer()));
    } else {
        group = _root;
    }

    if (!group) {
        return QModelIndex(); // invalid
    }

    if ((row < 0) || (row >= (int) group->childCount())) {
        return QModelIndex(); // invalid
    }

    return createIndex(row, column, group->childAt(row));
}

QModelIndex CanvasTreeModel::parent(const QModelIndex &child) const
{
    FGCanvasElement* e = static_cast<FGCanvasElement*>(child.internalPointer());
    if (!child.isValid() || !e) {
        return QModelIndex();
    }

    return indexForGroup(const_cast<FGCanvasGroup*>(e->parentGroup()));
}

Qt::ItemFlags CanvasTreeModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
}

bool CanvasTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    FGCanvasElement* e = static_cast<FGCanvasElement*>(index.internalPointer());
    if (!e) {
        return false;
    }

    qDebug() << Q_FUNC_INFO;
    if (role == Qt::CheckStateRole) {
        e->property()->changeValue("visible", (value.toInt() == Qt::Checked));
        emit dataChanged(index, index, QVector<int>() << Qt::CheckStateRole);
        return true;
    }

    return false;
}

QModelIndex CanvasTreeModel::indexForGroup(FGCanvasGroup* group) const
{
    if (!group) {
        return QModelIndex();
    }

    if (group->parentGroup()) {
        int prIndex = group->parentGroup()->indexOfChild(group);
        return createIndex(prIndex, 0, group);
    } else {
        return createIndex(0, 0, group);
    }
}

void CanvasTreeModel::onGroupChildAdded()
{
    FGCanvasGroup* group = qobject_cast<FGCanvasGroup*>(sender());
    int newChild = group->childCount() - 1;

    FGCanvasGroup* childGroup = qobject_cast<FGCanvasGroup*>(group->childAt(newChild));
    if (childGroup) {
        connect(childGroup, &FGCanvasGroup::childAdded, this, &CanvasTreeModel::onGroupChildAdded);
    }

    beginInsertRows(indexForGroup(group),
                    newChild, newChild);
    endInsertRows();
}

void CanvasTreeModel::onGroupChildRemoved(int index)
{
    FGCanvasGroup* group = qobject_cast<FGCanvasGroup*>(sender());
    beginRemoveRows(indexForGroup(group), index, index);
}
