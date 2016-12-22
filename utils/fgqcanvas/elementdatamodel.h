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
