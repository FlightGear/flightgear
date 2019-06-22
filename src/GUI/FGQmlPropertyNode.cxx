// FGQmlPropertyNode.cxx - expose SGPropertyNode to QML
//
// Copyright (C) 2019  James Turner  <james@flightgear.org>
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

#include "FGQmlPropertyNode.hxx"

#include <QVector3D>
#include <QVector4D>
#include <QDebug>

#include <simgear/props/props.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/math/SGMath.hxx>

#include <Main/fg_props.hxx>

/////////////////////////////////////////////////////////////////////////

FGQmlPropertyNode::FGQmlPropertyNode(QObject *parent) : QObject(parent)
{

}

bool FGQmlPropertyNode::set(QVariant newValue)
{
    if (!_prop || !_prop->getAttribute(SGPropertyNode::WRITE))
        return false;

    // we still need to short circuit setting
    if (newValue == value()) {
        return true;
    }

    switch (_prop->getType()) {
    case simgear::props::INT:
    case simgear::props::LONG:
        _prop->setIntValue(newValue.toInt());
    case simgear::props::BOOL:      _prop->setBoolValue(newValue.toBool());
    case simgear::props::DOUBLE:    _prop->setDoubleValue(newValue.toDouble());
    case simgear::props::FLOAT:     _prop->setFloatValue(newValue.toFloat());
    case simgear::props::STRING:    _prop->setStringValue(newValue.toString().toStdString());

    case simgear::props::VEC3D: {
        QVector3D v = newValue.value<QVector3D>();
        _prop->setValue(SGVec3d(v.x(), v.y(), v.z()));
    }

    case simgear::props::VEC4D: {
        QVector4D v = newValue.value<QVector4D>();
        _prop->setValue(SGVec4d(v.x(), v.y(), v.z(), v.w()));
    }

    default:
        qWarning() << Q_FUNC_INFO << "handle untyped property writes";
        break;
    }

    return true;
}

QVariant FGQmlPropertyNode::value() const
{
    return propertyValueAsVariant(_prop);
}

QString FGQmlPropertyNode::path() const
{
    if (!_prop)
        return QString();

    return QString::fromStdString(_prop->getPath());
}

FGQmlPropertyNode *FGQmlPropertyNode::parentProp() const
{
    if (!_prop || !_prop->getParent())
        return nullptr;

    auto pp = new FGQmlPropertyNode;
    pp->setNode(_prop->getParent());
    return pp;
}

void FGQmlPropertyNode::setNode(SGPropertyNode_ptr node)
{
    if (node == _prop) {
        return;
    }

    if (_prop) {
        _prop->removeChangeListener(this);
    }

    _prop = node;
    if (_prop) {
        _prop->addChangeListener(this, false);
    }

    emit parentPropChanged(parentProp());
    emit pathChanged(path());
    emit valueChangedNotify(value());
    emit childPropsChanged();
}

SGPropertyNode_ptr FGQmlPropertyNode::node() const
{
    return _prop;
}

QVariant FGQmlPropertyNode::propertyValueAsVariant(SGPropertyNode* p)
{
    if (!p)
        return {};

    switch (p->getType()) {
    case simgear::props::INT:
    case simgear::props::LONG:
        return p->getIntValue();
    case simgear::props::BOOL: return p->getBoolValue();
    case simgear::props::DOUBLE: return p->getDoubleValue();
    case simgear::props::FLOAT: return p->getFloatValue();
    case simgear::props::STRING: return QString::fromStdString(p->getStringValue());

    case simgear::props::VEC3D: {
        const SGVec3d v3 = p->getValue<SGVec3d>();
        return QVariant::fromValue(QVector3D(v3.x(), v3.y(), v3.z()));
    }

    case simgear::props::VEC4D: {
        const SGVec4d v4 = p->getValue<SGVec4d>();
        return QVariant::fromValue(QVector4D(v4.x(), v4.y(), v4.z(), v4.w()));
    }
    default:
        break;
    }

    return {}; // null qvariant
}

void FGQmlPropertyNode::valueChanged(SGPropertyNode *node)
{
    if (node != _prop) {
        return;
    }
    emit valueChangedNotify(value());
}

void FGQmlPropertyNode::childAdded(SGPropertyNode* pr, SGPropertyNode* child)
{
    if (pr == _prop) {
        emit childPropsChanged();
    }
}

void FGQmlPropertyNode::childRemoved(SGPropertyNode* pr, SGPropertyNode* child)
{
    if (pr == _prop) {
        emit childPropsChanged();
    }
}

void FGQmlPropertyNode::setPath(QString path)
{
    SGPropertyNode_ptr node = fgGetNode(path.toStdString(), false /* don't create */);
    setNode(node);
}


int FGQmlPropertyNode::childCount() const
{
    if (!_prop)
        return 0;
    return _prop->nChildren();
}

FGQmlPropertyNode* FGQmlPropertyNode::childAt(int index) const
{
    if (!_prop || (_prop->nChildren() <= index))
        return nullptr;

    auto pp = new FGQmlPropertyNode;
    pp->setNode(_prop->getChild(index));
    return pp;
}
