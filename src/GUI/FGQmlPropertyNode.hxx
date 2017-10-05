#ifndef FGQMLPROPERTYNODE_HXX
#define FGQMLPROPERTYNODE_HXX

#include <QObject>
#include <QVariant>

#include <simgear/props/props.hxx>

class FGQmlPropertyNode : public QObject,
        public SGPropertyChangeListener
{
    Q_OBJECT
public:
    explicit FGQmlPropertyNode(QObject *parent = nullptr);

    Q_PROPERTY(QVariant value READ value NOTIFY valueChangedNotify)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(FGQmlPropertyNode* parentProp READ parentProp NOTIFY parentPropChanged)

    Q_INVOKABLE bool set(QVariant newValue);

    // children accessor
    QVariant value() const;

    QString path() const;

    FGQmlPropertyNode* parentProp() const;

    // C++ API, not accessible from QML
    void setNode(SGPropertyNode_ptr node);

    SGPropertyNode_ptr node() const;
protected:
    // SGPropertyChangeListener API

      void valueChanged(SGPropertyNode * node) override;

signals:

      void valueChangedNotify(QVariant value);

      void pathChanged(QString path);

      void parentPropChanged(FGQmlPropertyNode* parentProp);

public slots:

    void setPath(QString path);

private:
    SGPropertyNode_ptr _prop;
};

#endif // FGQMLPROPERTYNODE_HXX
