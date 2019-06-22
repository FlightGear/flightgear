// Copyright (C) 2020  James Turner  <james@flightgear.org>
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

#include "config.h"

#include "QmlPropertyModel.hxx"

#include <algorithm>

#include <QDebug>
#include <QVector>

#include <GUI/FGQmlPropertyNode.hxx>
#include <GUI/QmlPropertyModel.hxx>
#include <Main/fg_props.hxx>
#include <simgear/props/props.hxx>

class FGQmlPropertyModel::PropertyModelPrivate : public SGPropertyChangeListener
{
public:
    void clear()
    {
        if (_props) {
            _props->removeChangeListener(this);
            _props.clear();
            _roles.clear();
        }
    }

    void computeProps()
    {
        _props = fgGetNode(_rootPath.toStdString());
        if (!_props) {
            qWarning() << "Passed non-existant path to QmlPropertyModel:" << _rootPath;
        }

        _props->addChangeListener(this);

        if (_childName.isEmpty()) {
            // all children, no filtering by name
            const auto sz = _props->nChildren();
            _directChildren.resize(sz);
            for (auto i = 0; i < sz; ++i) {
                _directChildren[i] = _props->getChild(i);
            }
        } else {
            _directChildren = _props->getChildren(_childName.toStdString());
        }

        cacheRoleNames();
    }

    void cacheRoleNames()
    {
        auto oldRoles = _roles;
        _roles.clear();
        for (auto p : _directChildren) {
            const auto nc = p->nChildren();
            for (int i = 0; i < nc; ++i) {
                roleForNode(p->getChild(i)->getNameString());
            }
        }

        if (_roles != oldRoles) {
            // we need to model-reset to tell QML about new names
            p->beginResetModel();
            p->endResetModel();
        }
    }

    int roleForNode(const std::string& s)
    {
        auto it = std::find(_roles.begin(), _roles.end(), s);
        if (it != _roles.end()) {
            return static_cast<int>(std::distance(_roles.begin(), it)) + Qt::UserRole;
        }

        qDebug() << Q_FUNC_INFO << "adding" << QString::fromStdString(s);
        _roles.push_back(s);
        return Qt::UserRole + static_cast<int>(_roles.size() - 1);
    }

    void valueChanged(SGPropertyNode* node) override
    {
        auto it = std::find(_directChildren.begin(), _directChildren.end(), node);
        if (it == _directChildren.end())
            return;

        doDataChanged(it, node);
    }

    void doDataChanged(const simgear::PropertyList::iterator& it, SGPropertyNode* node)
    {
        QVector<int> roles;
        roles.append(roleForNode(node->getNameString()));
        QModelIndex m = p->index(std::distance(_directChildren.begin(), it));
        p->dataChanged(m, m, roles);
    }

    void childAdded(SGPropertyNode* parent, SGPropertyNode* child) override
    {
        if (parent == _props) {
            if (!_childName.isEmpty() && (child->getNameString() != _childName.toStdString())) {
                // doesn't pass name filter, don't care
                return;
            }

            int insertRow = child->getIndex();
            if (!_childName.isEmpty()) {
                // always an append
                insertRow = _directChildren.size();
            }

            p->beginInsertRows(QModelIndex{}, insertRow, insertRow);
            _directChildren.push_back(child);
            p->endInsertRows();
            return;
        }

        auto it = std::find(_directChildren.begin(), _directChildren.end(), parent);
        if (it == _directChildren.end())
            return;

        doDataChanged(it, child);
    }

    void childRemoved(SGPropertyNode* parent, SGPropertyNode* child) override
    {
        if (parent == _props) {
            if (!_childName.isEmpty() && (child->getNameString() != _childName.toStdString())) {
                // doesn't pass name filter, don't care
                return;
            }

            auto it = std::find(_directChildren.begin(), _directChildren.end(), child);
            if (it == _directChildren.end()) {
                SG_LOG(SG_GUI, SG_DEV_ALERT, "Bug in QmlPropertyModel - child not found when removing:" << parent->getPath() << " - " << child->getName());
                return;
            }

            int row = static_cast<int>(std::distance(_directChildren.begin(), it));
            p->beginRemoveRows(QModelIndex{}, row, row);
            _directChildren.erase(it);
            p->endInsertRows();
            return;
        }

        auto it = std::find(_directChildren.begin(), _directChildren.end(), parent);
        if (it == _directChildren.end())
            return;

        // actually the value will be null now
        doDataChanged(it, child);
    }

    QString _rootPath;
    QString _childName;

    FGQmlPropertyModel* p;
    SGPropertyNode_ptr _props;
    simgear::PropertyList _directChildren;
    std::vector<std::string> _roles;
};

FGQmlPropertyModel::~FGQmlPropertyModel()
{
    d->clear();
}

QString FGQmlPropertyModel::rootPath() const
{
    return d->_rootPath;
}

QString FGQmlPropertyModel::childName() const
{
    return d->_childName;
}

QHash<int, QByteArray> FGQmlPropertyModel::roleNames() const
{
    QHash<int, QByteArray> r;
    for (int i = 0; i < d->_roles.size(); ++i) {
        r[i] = QByteArray::fromStdString(d->_roles.at(i));
    }
    return r;
}

QVariant FGQmlPropertyModel::data(const QModelIndex& m, int role) const
{
    auto node = d->_directChildren.at(m.row());
    const int r = role - Qt::UserRole;
    assert(r < d->_roles.size());
    const auto& propName = d->_roles.at(r);
    const auto prop = node->getChild(propName);
    if (!prop)
        return {}; // no data for role
    return FGQmlPropertyNode::propertyValueAsVariant(prop);
}

void FGQmlPropertyModel::setRootPath(QString rootPath)
{
    if (d->_rootPath == rootPath)
        return;

    beginResetModel();
    d->_rootPath = rootPath;
    d->clear();
    d->computeProps();
    endResetModel();

    emit rootPathChanged(d->_rootPath);
}

void FGQmlPropertyModel::setChildName(QString childName)
{
    if (d->_childName == childName)
        return;

    beginResetModel();
    d->_childName = childName;
    d->clear();
    d->computeProps();
    endResetModel();

    emit childNameChanged(d->_childName);
}
