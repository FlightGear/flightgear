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

#ifndef LOCALPROP_H
#define LOCALPROP_H

#include <QByteArray>
#include <QVariant>
#include <QVector>
#include <QObject>
#include <QDataStream>

struct NameIndexTuple
{
    QByteArray name;
    unsigned int index = 0;

    NameIndexTuple()
    {}

    NameIndexTuple(const char* nm, unsigned int idx) :
        name(nm),
        index(idx)
    {}

    NameIndexTuple(const QByteArray& bytes) :
        name(bytes)
    {
        Q_ASSERT(bytes.indexOf('/') == -1);
        if (bytes.endsWith(']')) {
            int leftBracket = bytes.indexOf('[');
            index = bytes.mid(leftBracket + 1, bytes.length() - (leftBracket + 2)).toInt();
            name = bytes.left(leftBracket);
        }
    }

    QByteArray toString() const
    {
        QByteArray p = name;
        if (index > 0) {
            p += '[' + QByteArray::number(index) + ']';
        }
        return p;
    }

    bool operator==(const NameIndexTuple& other) const
    {
        return (name == other.name) && (index == other.index);
    }

    bool operator<(const NameIndexTuple& other) const
    {
        if (name == other.name) {
            return index < other.index;
        }

        return name < other.name;
    }

};

QDataStream& operator<<(QDataStream& stream, const NameIndexTuple& nameIndex);
QDataStream& operator>>(QDataStream& stream, NameIndexTuple& nameIndex);

class LocalProp : public QObject
{
    Q_OBJECT
public:
    LocalProp(LocalProp* parent, const NameIndexTuple& ni);

    virtual ~LocalProp();

    void processChange(QJsonValue newValue);

    const NameIndexTuple& id() const;

    QByteArray path() const;

    LocalProp* getOrCreateWithPath(const QByteArray& path, QVariant defaultValue = {});

    LocalProp* childWithNameAndIndex(const NameIndexTuple& ni) const;

    LocalProp* getOrCreateChildWithNameAndIndex(const NameIndexTuple& ni, QVariant defaultValue = {});

    LocalProp* getOrCreateWithPath(const char* name);

    LocalProp* getWithPath(const QByteArray& path) const;

    LocalProp* getWithPath(const char* name) const;

    QByteArray name() const;

    unsigned int index() const;

    /// position in the main FG propery tree. Normally
    /// irrelevant but unfortunately necessary for correct
    /// z-ordering of Canvas elements
    unsigned int position() const
    {
        return _position;
    }

    void setPosition(unsigned int pos);

    LocalProp* parent() const;

    std::vector<LocalProp*> children() const
    { return _children; }

    std::vector<QVariant> valuesOfChildren(const char* name) const;

    std::vector<LocalProp*> childrenWithName(const char* name) const;

    QVariant value() const;

    QVariant value(const char* path, QVariant defaultValue) const;

    void removeChild(LocalProp* prop);

    bool hasChild(const char* name) const;

    void changeValue(const char* path, QVariant value);

    void saveToStream(QDataStream& stream) const;

    static LocalProp* restoreFromStream(QDataStream& stream, LocalProp *parent);

    void recursiveNotifyRestored();
signals:
    void valueChanged(QVariant val);

    void childAdded(LocalProp* child);
    void childRemoved(LocalProp* child);

private:
    const NameIndexTuple _id;
    const LocalProp* _parent;
    std::vector<LocalProp*> _children;
    QVariant _value;
    unsigned int _position = 0;
};

#endif // LOCALPROP_H
