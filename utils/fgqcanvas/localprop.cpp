#include "localprop.h"

#include <QJsonValue>
#include <QDebug>

LocalProp *LocalProp::getOrCreateWithPath(const QByteArray &path)
{
    if (path.isEmpty()) {
        return this;
    }

    QList<QByteArray> segments = path.split('/');
    LocalProp* result = this;
    while (!segments.empty()) {
        QByteArray nameIndex = segments.front();
        result = result->getOrCreateChildWithNameAndIndex(nameIndex);
        segments.pop_front();
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


LocalProp *LocalProp::getOrCreateChildWithNameAndIndex(const NameIndexTuple& ni)
{
    auto it = std::lower_bound(_children.begin(), _children.end(), ni, lessThanPropNameIndex);
    if ((it != _children.end()) && ((*it)->id() == ni)) {
        return *it;
    }

    LocalProp* newChild = new LocalProp(this, ni);
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
    _children.erase(it);
    emit childRemoved(prop);
    delete prop;
}
