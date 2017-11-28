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

#include "localprop.h"

#include <QJsonValue>
#include <QDebug>

QDataStream& operator<<(QDataStream& stream, const NameIndexTuple& nameIndex)
{
    stream << nameIndex.name << nameIndex.index;
    return stream;
}

QDataStream& operator>>(QDataStream& stream, NameIndexTuple& nameIndex)
{
    stream >> nameIndex.name >> nameIndex.index;
    return stream;
}

LocalProp *LocalProp::getOrCreateWithPath(const QByteArray &path, QVariant defaultValue)
{
    if (path.isEmpty()) {
        return this;
    }

    QList<QByteArray> segments = path.split('/');
    LocalProp* result = this;
    while (segments.size() > 1) {
        QByteArray nameIndex = segments.front();
        result = result->getOrCreateChildWithNameAndIndex(nameIndex);
        segments.pop_front();
    }

    // for the final segment, pass the default value
    if (!segments.empty()) {
        result = result->getOrCreateChildWithNameAndIndex(segments.front(), defaultValue);
    }

    return result;
}

LocalProp *LocalProp::getWithPath(const QByteArray &path) const
{
    if (path.isEmpty()) {
        return const_cast<LocalProp*>(this);
    }

    QList<QByteArray> segments = path.split('/');
    LocalProp* result = const_cast<LocalProp*>(this);
    while (!segments.empty()) {
        QByteArray nameIndex = segments.front();
        result = result->childWithNameAndIndex(nameIndex);
        segments.pop_front();
        if (!result) {
            return nullptr;
        }
    }

    return result;
}

static bool lessThanPropNameIndex(const LocalProp* prop, const NameIndexTuple& ni)
{
    return prop->id() < ni;
}


LocalProp::LocalProp(LocalProp *pr, const NameIndexTuple& ni) :
    QObject(pr),
    _id(ni),
    _parent(pr)
{
}

LocalProp::~LocalProp()
{
    for (auto c : _children) {
        delete c;
    }
}

void LocalProp::processChange(QJsonValue json)
{
    QVariant newValue = json.toVariant();
    if (newValue != _value) {
        _value = newValue;
        emit valueChanged(_value);
    }
}

const NameIndexTuple &LocalProp::id() const
{
    return _id;
}

QByteArray LocalProp::path() const
{
    if (_parent) {
        return _parent->path() + '/' + _id.toString();
    }

    return _id.toString();
}

LocalProp *LocalProp::childWithNameAndIndex(const NameIndexTuple& ni) const
{
    auto it = std::lower_bound(_children.begin(), _children.end(), ni, lessThanPropNameIndex);
    if ((it != _children.end()) && ((*it)->id() == ni)) {
        return *it;
    }

    return nullptr;
}

bool LocalProp::hasChild(const char* name) const
{
    return childWithNameAndIndex(QByteArray::fromRawData(name, strlen(name))) != nullptr;
}

void LocalProp::changeValue(const char *path, QVariant value)
{
    LocalProp* p = getOrCreateWithPath(path);
    p->_value = value;
    p->valueChanged(value);
}

void LocalProp::saveToStream(QDataStream &stream) const
{
    stream << _id << _position << _value;
    stream << static_cast<int>(_children.size());
    for (auto child : _children) {
        child->saveToStream(stream);
    }
}

LocalProp* LocalProp::restoreFromStream(QDataStream &stream, LocalProp* parent)
{
    NameIndexTuple id;
    stream >> id;
    LocalProp* prop = new LocalProp(parent, id);
    stream >> prop->_position >> prop->_value;
    int childCount;
    stream >> childCount;
    for (int c=0; c< childCount; ++c) {
        prop->_children.push_back(restoreFromStream(stream, prop));
    }

    return prop;
}

void LocalProp::recursiveNotifyRestored()
{
    emit valueChanged(_value);
    for (auto child : _children) {
        emit childAdded(child);
    }

    for (auto cc : _children) {
        cc->recursiveNotifyRestored();
    }
}

LocalProp *LocalProp::getOrCreateChildWithNameAndIndex(const NameIndexTuple& ni,
                                                       QVariant defaultValue)
{
    auto it = std::lower_bound(_children.begin(), _children.end(), ni, lessThanPropNameIndex);
    if ((it != _children.end()) && ((*it)->id() == ni)) {
        return *it;
    }

    LocalProp* newChild = new LocalProp(this, ni);
    newChild->_value = defaultValue;
    _children.insert(it, newChild);
    emit childAdded(newChild);
    return newChild;
}

LocalProp *LocalProp::getOrCreateWithPath(const char *name)
{
    return getOrCreateWithPath(QByteArray::fromRawData(name, strlen(name)));
}

LocalProp *LocalProp::getWithPath(const char *name) const
{
    return getWithPath(QByteArray::fromRawData(name, strlen(name)));
}

QByteArray LocalProp::name() const
{
    return _id.name;
}

unsigned int LocalProp::index() const
{
    return _id.index;
}

void LocalProp::setPosition(unsigned int pos)
{
    _position = pos;
}

LocalProp *LocalProp::parent() const
{
    return const_cast<LocalProp*>(_parent);
}

std::vector<QVariant> LocalProp::valuesOfChildren(const char *name) const
{
    std::vector<QVariant> result;

    for (LocalProp* c : childrenWithName(name)) {
        result.push_back(c->value());
    }

    return result;
}

std::vector<LocalProp *> LocalProp::childrenWithName(const char *name) const
{
    std::vector<LocalProp *> result;
    for (LocalProp* child : _children) {
        if (child->_id.name == name)
            result.push_back(child);
    }
    return result;
}

QVariant LocalProp::value() const
{
    return _value;
}

QVariant LocalProp::value(const char *path, QVariant defaultValue) const
{
    LocalProp* n = getWithPath(path);
    if (!n || n->value().isNull()) {
        return defaultValue;
    }

    return n->value();
}

void LocalProp::removeChild(LocalProp *prop)
{
    Q_ASSERT(prop->parent() == this);
    auto it = std::find(_children.begin(), _children.end(), prop);
    Q_ASSERT(it != _children.end());
    _children.erase(it);
    emit childRemoved(prop);
    delete prop;
}
