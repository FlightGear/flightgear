// FGQmlPropertyNode.hxx - expose SGPropertyNode to QML
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

#ifndef FGQMLPROPERTYNODE_HXX
#define FGQMLPROPERTYNODE_HXX

#include <QObject>
#include <QQmlListProperty>
#include <QVariant>

#include <simgear/props/props.hxx>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QML_LIST_INDEX_TYPE qsizetype
#else
#define QML_LIST_INDEX_TYPE int
#endif

class FGQmlPropertyNode : public QObject,
        public SGPropertyChangeListener
{
    Q_OBJECT
public:
    explicit FGQmlPropertyNode(QObject *parent = nullptr);

    Q_PROPERTY(QVariant value READ value NOTIFY valueChangedNotify)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(FGQmlPropertyNode* parentProp READ parentProp NOTIFY parentPropChanged)

    Q_PROPERTY(QQmlListProperty<FGQmlPropertyNode> childProps READ childProps NOTIFY childPropsChanged)


    Q_INVOKABLE bool set(QVariant newValue);

    // children accessor
    QVariant value() const;

    QString path() const;

    FGQmlPropertyNode* parentProp() const;

    // C++ API, not accessible from QML
    void setNode(SGPropertyNode_ptr node);

    SGPropertyNode_ptr node() const;

    QQmlListProperty<FGQmlPropertyNode> childProps()
    {
        return QQmlListProperty<FGQmlPropertyNode>(this, nullptr,
                                                   children_count,
                                                   child_at);
    }


    int childCount() const;
    FGQmlPropertyNode* childAt(int index) const;

    static QVariant propertyValueAsVariant(SGPropertyNode* p);

protected:
    // SGPropertyChangeListener API

      void valueChanged(SGPropertyNode * node) override;
      void childAdded(SGPropertyNode* pr, SGPropertyNode* child) override;
      void childRemoved(SGPropertyNode* pr, SGPropertyNode* child) override;

  signals:

      void valueChangedNotify(QVariant value);

      void pathChanged(QString path);

      void parentPropChanged(FGQmlPropertyNode* parentProp);
      void childPropsChanged();

  public slots:

      void setPath(QString path);

  private:
      static QML_LIST_INDEX_TYPE children_count(QQmlListProperty<FGQmlPropertyNode>* prop)
      {
          return static_cast<FGQmlPropertyNode*>(prop->object)->childCount();
      }

      static FGQmlPropertyNode* child_at(QQmlListProperty<FGQmlPropertyNode>* prop,
                                         QML_LIST_INDEX_TYPE index)
      {
          return static_cast<FGQmlPropertyNode*>(prop->object)->childAt(index);
      }

    SGPropertyNode_ptr _prop;
};

#endif // FGQMLPROPERTYNODE_HXX
