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
    if (!_prop)
        return {};

    switch (_prop->getType()) {
    case simgear::props::INT:
    case simgear::props::LONG:
        return _prop->getIntValue();
    case simgear::props::BOOL:      return _prop->getBoolValue();
    case simgear::props::DOUBLE:    return _prop->getDoubleValue();
    case simgear::props::FLOAT:     return _prop->getFloatValue();
    case simgear::props::STRING:    return QString::fromStdString(_prop->getStringValue());

    case simgear::props::VEC3D: {
         const SGVec3d v3 = _prop->getValue<SGVec3d>();
         return QVariant::fromValue(QVector3D(v3.x(), v3.y(), v3.z()));
    }

    case simgear::props::VEC4D: {
         const SGVec4d v4 = _prop->getValue<SGVec4d>();
         return QVariant::fromValue(QVector4D(v4.x(), v4.y(), v4.z(), v4.w()));
    }
    default:
        break;
    }

    return {}; // null qvariant
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
}

SGPropertyNode_ptr FGQmlPropertyNode::node() const
{
    return _prop;
}

void FGQmlPropertyNode::valueChanged(SGPropertyNode *node)
{
    if (node != _prop) {
        return;
    }
    emit valueChangedNotify(value());
}

void FGQmlPropertyNode::setPath(QString path)
{
    SGPropertyNode_ptr node = fgGetNode(path.toStdString(), false /* don't create */);
    setNode(node);
}

