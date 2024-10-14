#include "PropertyItemModel.hxx"

#include <Main/fg_props.hxx>

#include "FGQmlPropertyNode.hxx"

class PropertyItemModelRoleMapping : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString roleName READ roleName WRITE setRoleName NOTIFY roleNameChanged)
public:

    QString path() const
    {
        return QString::fromStdString(_propertyPath);
    }

    QString roleName() const
    {
        return QString::fromLatin1(_roleName);
    }

public slots:
    void setPath(QString path)
    {
        if (_propertyPath == path.toStdString())
            return;

        _propertyPath = path.toStdString();
        emit pathChanged(path);
    }

    void setRoleName(QString roleName)
    {
        if (_roleName == roleName.toLatin1())
            return;

        _roleName = roleName.toLatin1();
        emit roleNameChanged(roleName);
    }

signals:
    void pathChanged(QString path);

    void roleNameChanged(QString roleName);

private:
    friend class PropertyItemModel;
    std::string _propertyPath;
    QByteArray _roleName;
};

PropertyItemModel::PropertyItemModel()
{

}

int PropertyItemModel::rowCount(const QModelIndex &parent) const
{
    return _nodes.size();
}

QVariant PropertyItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (static_cast<size_t>(index.row()) >= _nodes.size())
        return {};

    SGPropertyNode_ptr n = _nodes.at(index.row());
    Q_ASSERT(n);
    int mappingIndex = role - Qt::UserRole;
    if ((mappingIndex < 0) || (mappingIndex >= _roleMappings.size()))
        return {};

    const std::string& nodePath = _roleMappings.at(mappingIndex)->_propertyPath;
    SGPropertyNode_ptr valueNode = n->getNode(nodePath);

    // convert property value to a variant

    return {};
}

QHash<int, QByteArray> PropertyItemModel::roleNames() const
{
    QHash<int, QByteArray> result;
    int count = 0;
    for (auto m : _roleMappings) {
        result[Qt::UserRole + count++] = m->_roleName;
    }
    return result;
}

QString PropertyItemModel::rootPath() const
{
    return QString::fromStdString(_modelRoot->getPath());
}

QString PropertyItemModel::childNameFilter() const
{
    return _childNameFilter;
}

FGQmlPropertyNode *PropertyItemModel::root() const
{
    auto n = new FGQmlPropertyNode;
    n->setNode(_modelRoot);
    return n;
}

void PropertyItemModel::setRootPath(QString rootPath)
{
    if (_modelRoot->getPath() == rootPath.toStdString())
        return;

    innerSetRoot(fgGetNode(rootPath.toStdString()));
}

void PropertyItemModel::setChildNameFilter(QString childNameFilter)
{
    if (_childNameFilter == childNameFilter)
        return;

    _childNameFilter = childNameFilter;
    emit childNameFilterChanged(_childNameFilter);

    beginResetModel();
    rebuild();
    endResetModel();
}

void PropertyItemModel::setRoot(FGQmlPropertyNode *root)
{
    if (root->node() == _modelRoot) {
        return;
    }

    innerSetRoot(root->node());
}

void PropertyItemModel::valueChanged(SGPropertyNode *node)
{
    // if node's parent is one of our nodes, emit data changed
}

void PropertyItemModel::childAdded(SGPropertyNode *parent, SGPropertyNode *child)
{
    if (parent == _modelRoot) {
        if (_childNameFilter.isEmpty() || (child->getNameString() == _childNameFilter.toStdString())) {
            // insert
            // need to find appropriate index based on position
        }
    }

    // if parent is one of our nodes, emit data changed for it
}

void PropertyItemModel::childRemoved(SGPropertyNode *parent, SGPropertyNode *child)
{
    if (parent == _modelRoot) {
        // remove row
    }

    // if parent is one of our nodes, emit data changed for it

}

QQmlListProperty<PropertyItemModelRoleMapping> PropertyItemModel::mappings()
{
    return QQmlListProperty<PropertyItemModelRoleMapping>(this, nullptr,
                                                          &PropertyItemModel::append_mapping,
                                                          &PropertyItemModel::count_mapping,
                                                          &PropertyItemModel::at_mapping,
                                                          &PropertyItemModel::clear_mapping
                                                          );
}

void PropertyItemModel::rebuild()
{
    if (!_modelRoot) {
        _nodes.clear();
        return;
    }

    if (_childNameFilter.isEmpty()) {
        // all children
        _nodes.clear();
        const int nChildren = _modelRoot->nChildren();
        _nodes.resize(nChildren);
        for (int c=0; c < nChildren; ++c) {
            _nodes[c] = _modelRoot->getChild(c);
        }
    } else {
        _nodes = _modelRoot->getChildren(_childNameFilter.toStdString());
    }
}

void PropertyItemModel::innerSetRoot(SGPropertyNode_ptr root)
{
    if (_modelRoot) {
        _modelRoot->removeChangeListener(this);;
    }
    beginResetModel();

    _modelRoot = root;
    if (_modelRoot) {
        _modelRoot->addChangeListener(this);
    }

    rebuild();
    emit rootChanged();
    endResetModel();
}

void PropertyItemModel::append_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list,
                                       PropertyItemModelRoleMapping *mapping)
{
    PropertyItemModel *model = qobject_cast<PropertyItemModel *>(list->object);
    if (model) {
        model->_roleMappings.append(mapping);
        model->mappingsChanged();
        model->rebuild();
    }
}

PropertyItemModelRoleMapping* PropertyItemModel::at_mapping(QQmlListProperty<PropertyItemModelRoleMapping>* list, QML_LIST_INDEX_TYPE index)
{
    PropertyItemModel *model = qobject_cast<PropertyItemModel *>(list->object);
    if (model) {
        return model->_roleMappings.at(index);
    }
    return 0;
}

QML_LIST_INDEX_TYPE PropertyItemModel::count_mapping(QQmlListProperty<PropertyItemModelRoleMapping>* list)
{
    PropertyItemModel *model = qobject_cast<PropertyItemModel *>(list->object);
    if (model) {
        return model->_roleMappings.size();
    }
    return 0;
}

void PropertyItemModel::clear_mapping(QQmlListProperty<PropertyItemModelRoleMapping> *list)
{
    PropertyItemModel *model = qobject_cast<PropertyItemModel *>(list->object);
    if (model) {
        model->_roleMappings.clear();
        model->mappingsChanged();
        model->rebuild();
    }
}


#include "PropertyItemModel.moc"
