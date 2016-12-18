#include "fgcanvasgroup.h"

#include <QDebug>

#include "localprop.h"
#include "fgcanvaspaintcontext.h"
#include "fgcanvaspath.h"
#include "fgcanvastext.h"
#include "fgqcanvasmap.h"
#include "fgqcanvasimage.h"

FGCanvasGroup::FGCanvasGroup(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop)
{

}

const FGCanvasElementVec &FGCanvasGroup::children() const
{
    return _children;
}

void FGCanvasGroup::markChildZIndicesDirty() const
{
    _zIndicesDirty = true;
}

bool FGCanvasGroup::hasChilden() const
{
    return !_children.empty();
}

unsigned int FGCanvasGroup::childCount() const
{
    return _children.size();
}

FGCanvasElement *FGCanvasGroup::childAt(unsigned int index) const
{
    return _children.at(index);
}

unsigned int FGCanvasGroup::indexOfChild(const FGCanvasElement *e) const
{
    auto it = std::find(_children.begin(), _children.end(), e);
    if (it == _children.end()) {
        qWarning() << Q_FUNC_INFO << "not found";
        return 0;
    }

    return std::distance(_children.begin(), it);
}

void FGCanvasGroup::doPaint(FGCanvasPaintContext *context) const
{
    if (_zIndicesDirty) {
        std::sort(_children.begin(), _children.end(), [](const FGCanvasElement* a, FGCanvasElement* b)
                 { return a->zIndex() < b->zIndex(); });
        _zIndicesDirty = false;
    }

    if (_cachedSymbolDirty) {
        qDebug() << _propertyRoot->path() << "should use symbol cache:" << _propertyRoot->value("symbol-type", QVariant()).toByteArray();
        _cachedSymbolDirty = false;
    }

    for (FGCanvasElement* element : _children) {
        element->paint(context);
    }
}

bool FGCanvasGroup::onChildAdded(LocalProp *prop)
{
    const bool isRootGroup = (parent() == nullptr);
    int newChildCount = 0;

    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    const QByteArray nm = prop->name();
    if (nm == "group") {
        _children.push_back(new FGCanvasGroup(this, prop));
        newChildCount++;
        return true;
    } else if (nm == "path") {
        _children.push_back(new FGCanvasPath(this, prop));
        newChildCount++;
        return true;
    } else if (nm == "text") {
        _children.push_back(new FGCanvasText(this, prop));
        newChildCount++;
        return true;
    } else if (nm == "image") {
        _children.push_back(new FGQCanvasImage(this, prop));
        newChildCount++;
        return true;
    } else if (nm == "map") {
        _children.push_back(new FGQCanvasMap(this, prop));
        newChildCount++;
        return true;
    } else if (nm == "symbol-type") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasGroup::markCachedSymbolDirty);
        return true;
    }

    if (isRootGroup) {
        // ignore all of these, handled by the enclosing canvas view
        if ((nm == "view") || (nm == "size") || nm.startsWith("status") || (nm == "name") || (nm == "mipmapping") || (nm == "placement")) {
            return true;
        }
    }

    if (newChildCount > 0) {
        emit childAdded();
    }

    qDebug() << "saw unknown group child" << prop->name();
    return false;
}

bool FGCanvasGroup::onChildRemoved(LocalProp *prop)
{
    if (FGCanvasElement::onChildRemoved(prop)) {
        return true;
    }

    const QByteArray nm = prop->name();
    if ((nm == "group") || (nm == "image") || (nm == "path") || (nm == "text") || (nm == "map")) {
        int removedChildIndex = indexOfChildWithProp(prop);
        if (removedChildIndex >= 0) {
            auto it = _children.begin() + removedChildIndex;
            delete *it;
            _children.erase(it);
            emit childRemoved(removedChildIndex);
        }
        return true;
    }

    return false;
}

int FGCanvasGroup::indexOfChildWithProp(LocalProp* prop) const
{
    auto it = std::find_if(_children.begin(), _children.end(), [prop](FGCanvasElement* child)
    {
       return (child->property() == prop);
    });

    if (it == _children.end()) {
        return -1;
    }

    return std::distance(_children.begin(), it);
}

void FGCanvasGroup::markStyleDirty()
{
    for (FGCanvasElement* element : _children) {
        element->markStyleDirty();
    }
}

void FGCanvasGroup::markCachedSymbolDirty()
{
    _cachedSymbolDirty = true;
}
