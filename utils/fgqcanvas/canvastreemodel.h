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
private:
    QModelIndex indexForGroup(FGCanvasGroup *group) const;

    void onGroupChildAdded();
    void onGroupChildRemoved(int index);

    FGCanvasGroup* _root;
};

#endif // CANVASTREEMODEL_H
