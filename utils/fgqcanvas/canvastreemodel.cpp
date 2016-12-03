#include "canvastreemodel.h"

#include "localprop.h"

CanvasTreeModel::CanvasTreeModel(FGCanvasGroup* root) :
    _root(root)
{
    connect(_root, &FGCanvasGroup::childAdded, this, &CanvasTreeModel::onGroupChildAdded);
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
    return 2;
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
