#ifndef LOCALPROP_H
#define LOCALPROP_H

#include <QByteArray>
#include <QVariant>
#include <QVector>
#include <QObject>

struct NameIndexTuple
{
    QByteArray name;
    unsigned int index = 0;

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

class LocalProp : public QObject
{
    Q_OBJECT
public:
    LocalProp(LocalProp* parent, const NameIndexTuple& ni);

    void processChange(QJsonValue newValue);

    const NameIndexTuple& id() const;

    QByteArray path() const;

    LocalProp* getOrCreateWithPath(const QByteArray& path);

    LocalProp* childWithNameAndIndex(const NameIndexTuple& ni) const;

    LocalProp* getOrCreateChildWithNameAndIndex(const NameIndexTuple& ni);

    LocalProp* getOrCreateWithPath(const char* name);

    LocalProp* getWithPath(const QByteArray& path) const;

    LocalProp* getWithPath(const char* name) const;

    QByteArray name() const;

    unsigned int index() const;

    LocalProp* parent() const;

    std::vector<QVariant> valuesOfChildren(const char* name) const;

    std::vector<LocalProp*> childrenWithName(const char* name) const;

    QVariant value() const;

    QVariant value(const char* path, QVariant defaultValue) const;

    void removeChild(LocalProp* prop);

    bool hasChild(const char* name) const;
signals:
    void valueChanged(QVariant val);

    void childAdded(LocalProp* child);
    void childRemoved(LocalProp* child);

private:
    const NameIndexTuple _id;
    const LocalProp* _parent;
    std::vector<LocalProp*> _children;
    QVariant _value;
};

#endif // LOCALPROP_H
