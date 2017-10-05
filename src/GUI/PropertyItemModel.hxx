#ifndef PROPERTYITEMMODEL_HXX
#define PROPERTYITEMMODEL_HXX

#include <QAbstractListModel>
#include <QQmlListProperty>

#include <simgear/props/props.hxx>

class PropertyItemModelRoleMapping;
class FGQmlPropertyNode;

class PropertyItemModel : public QAbstractListModel,
                          public SGPropertyChangeListener
{
    Q_OBJECT

    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootChanged)

    Q_PROPERTY(FGQmlPropertyNode* root READ root WRITE setRoot NOTIFY rootChanged)

    Q_PROPERTY(QString childNameFilter READ childNameFilter WRITE setChildNameFilter NOTIFY childNameFilterChanged)
    Q_PROPERTY(QQmlListProperty<PropertyItemModelRoleMapping> mappings READ mappings NOTIFY mappingsChanged)
    // list property storing child/role mapping

    QQmlListProperty<PropertyItemModelRoleMapping> mappings();
public:
    PropertyItemModel();

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    QString rootPath() const;

    QString childNameFilter() const;

    FGQmlPropertyNode* root() const;

public slots:
    void setRootPath(QString rootPath);

    void setChildNameFilter(QString childNameFilter);

    void setRoot(FGQmlPropertyNode* root);

signals:
    void rootChanged();

    void childNameFilterChanged(QString childNameFilter);

    void mappingsChanged();
protected:
    // implement property-change listener to watch for changes to the
    // underlying nodes
    void valueChanged(SGPropertyNode *node) override;
    void childAdded(SGPropertyNode *parent, SGPropertyNode *child) override;
    void childRemoved(SGPropertyNode *parent, SGPropertyNode *child) override;

private:
    void rebuild();

    void innerSetRoot(SGPropertyNode_ptr root);

    static void append_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list,
                               PropertyItemModelRoleMapping *mapping);
    static PropertyItemModelRoleMapping* at_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list, int index);
    static int count_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list);
    static void clear_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list);

    SGPropertyNode_ptr _modelRoot;
    QVector<PropertyItemModelRoleMapping*> _roleMappings;
    QString _childNameFilter;

    mutable std::vector<SGPropertyNode_ptr> _nodes;
};

#endif // PROPERTYITEMMODEL_HXX
