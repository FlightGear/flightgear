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

#include "fgcanvaselement.h"

#include "localprop.h"
#include "fgcanvaspaintcontext.h"
#include "fgcanvasgroup.h"
#include "canvasitem.h"
#include "canvasconnection.h"

#include <QDebug>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QMatrix4x4>

QTransform qTransformFromCanvas(LocalProp* prop)
{
    double m[6] = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // identity matrix
    for (unsigned int i =0; i< 6; ++i) {
        LocalProp* mProp =  prop->getOrCreateChildWithNameAndIndex(NameIndexTuple("m", i));
        if (!mProp->value().isNull()) {
            m[i] = mProp->value().toDouble();
        }
    }

    return QTransform(m[0], m[1], 0.0,
                      m[2], m[3], 0.0,
                      m[4], m[5], 1.0);
}

bool FGCanvasElement::isStyleProperty(QByteArray name)
{
    if ((name == "font") || (name == "line-height") || (name == "alignment")
        || (name == "character-size") || (name == "fill") || (name == "background")
        || (name == "fill-opacity"))
    {
        return true;
    }

    return false;
}

LocalProp *FGCanvasElement::property() const
{
    return const_cast<LocalProp*>(_propertyRoot);
}

void FGCanvasElement::setHighlighted(bool hilighted)
{
    _highlighted = hilighted;
}

bool FGCanvasElement::isHighlighted() const
{
    return _highlighted;
}

CanvasItem *FGCanvasElement::createQuickItem(QQuickItem *parent)
{
    Q_UNUSED(parent)
    return nullptr;
}

CanvasItem *FGCanvasElement::quickItem() const
{
    return nullptr;
}

FGCanvasElement::FGCanvasElement(FGCanvasGroup* pr, LocalProp* prop) :
    QObject(pr),
    _propertyRoot(prop),
    _parent(pr)
{
    connect(prop->getOrCreateWithPath("visible", true), &LocalProp::valueChanged,
            this, &FGCanvasElement::onVisibleChanged);
    connect(prop, &LocalProp::childAdded, this, &FGCanvasElement::onChildAdded);
    connect(prop, &LocalProp::childRemoved, this, &FGCanvasElement::onChildRemoved);
    connect(prop, &LocalProp::destroyed, this, &FGCanvasElement::onPropDestroyed);

    if (pr) {
        pr->markChildZIndicesDirty();
    }

    requestPolish();
}

void FGCanvasElement::onPropDestroyed()
{
    doDestroy();
    if (_parent) {
        const_cast<FGCanvasGroup*>(_parent)->removeChild(this);
    }
    deleteLater();
}

void FGCanvasElement::requestPolish()
{
    _polishRequired = true;
}

void FGCanvasElement::polish()
{
    bool vis = isVisible();
    auto qq = quickItem();
    if (qq && (qq->isVisible() != vis)) {
        qq->setVisible(vis);
    }

    if (!vis) {
        return;
    }

    if (_clipDirty) {
        _clipDirty = false;
        if (qq) {
            if (_hasClip) {
                if (_clipFrame == ReferenceFrame::GLOBAL) {
                    qq->setClipReferenceFrameItem(rootGroup()->quickItem());
                } else if (_clipFrame == ReferenceFrame::PARENT) {
                    qq->setClipReferenceFrameItem(parentGroup()->quickItem());
                }
                qq->setObjectName(_propertyRoot->path());
                qq->setClip(_clipRect, _clipFrame);
            } else {
                qq->clearClip();
            }
        }
    }

    if (qq) {
        qq->setTransform(combinedTransform());
    }

    if (_styleDirty) {
        _fillColor = parseColorValue(getCascadedStyle("fill"));
        const auto opacity = getCascadedStyle("fill-opacity");
        if (!opacity.isNull()) {
            _fillColor.setAlphaF(opacity.toReal());
        }

        _styleDirty = false;
    }

    doPolish();
    _polishRequired = false;
}

void FGCanvasElement::dumpElement()
{

}

void FGCanvasElement::paint(FGCanvasPaintContext *context) const
{
    if (!isVisible()) {
        return;
    }

    QPainter* p = context->painter();
    p->save();

    QTransform combined = combinedTransform();

    if (_hasClip)
    {
        QTransform t = p->transform();

        // clip is defined in the global coordinate system
        if (_clipFrame == ReferenceFrame::GLOBAL) {
            // this rpelaces the transform entirely
            p->setTransform(context->globalCoordinateTransform());
        } else if (_clipFrame == ReferenceFrame::LOCAL) {
            p->setTransform(combined,  true /* combine */);
        } else if (_clipFrame == ReferenceFrame::PARENT) {
            // incoming transform is already our parent
        } else {
            qWarning() << "Unhandled clip type:" << static_cast<int>(_clipFrame) << "at" << property()->path();
        }

#if defined(DEBUG_PAINTING)
        p->setPen(Qt::yellow);
        p->setBrush(QBrush(Qt::yellow, Qt::DiagCrossPattern));
        p->drawRect(_clipRect);
#endif
        p->setClipping(true);
        p->setClipRect(_clipRect);
        p->setTransform(t); // restore the previous transformation
    }

    p->setTransform(combined,  true /* combine */);

    if (!_fillColor.isValid()) {
        p->setBrush(Qt::NoBrush);
    } else {
        p->setBrush(_fillColor);
    }

    doPaint(context);

    if (_hasClip) {
        p->setClipping(false);
    }

    p->restore();
}

void FGCanvasElement::doPaint(FGCanvasPaintContext* context) const
{
    Q_UNUSED(context);
}

void FGCanvasElement::doPolish()
{
}

QTransform FGCanvasElement::combinedTransform() const
{
    if (_transformsDirty) {
        _combinedTransform.reset();

        for (LocalProp* tfProp : _propertyRoot->childrenWithName("tf")) {
            _combinedTransform *= qTransformFromCanvas(tfProp);
        }

#if 0
        QPointF offset(_propertyRoot->value("center-offset-x", 0.0).toFloat(),
                        _propertyRoot->value("center-offset-y", 0.0).toFloat());

        _combinedTransform.translate(offset.x(), offset.y());
#endif
        _transformsDirty = false;
    }

    return _combinedTransform;
}

bool FGCanvasElement::isVisible() const
{
    return _visible;
}

int FGCanvasElement::zIndex() const
{
    return _zIndex;
}

const FGCanvasGroup *FGCanvasElement::parentGroup() const
{
    return _parent;
}

const FGCanvasGroup *FGCanvasElement::rootGroup() const
{
    if (!_parent) {
        return qobject_cast<const FGCanvasGroup*>(this);
    }

    return _parent->rootGroup();
}

CanvasConnection *FGCanvasElement::connection() const
{
    if (_parent)
        return _parent->connection();
    return qobject_cast<CanvasConnection*>(parent());
}

bool FGCanvasElement::onChildAdded(LocalProp *prop)
{
    const QByteArray nm = prop->name();
    if (nm == "tf") {
        connect(prop, &LocalProp::childAdded, this, &FGCanvasElement::onChildAdded);
        return true;
    } else if (nm == "visible") {
        return true;
    } else if (nm == "tf-rot-index") {
        // ignored, this is noise from the Nasal SVG parser
        return true;
    } else if (nm.startsWith("center-offset-")) {
        // ignored, this is noise from the Nasal SVG parser
        return true;
    } else if (nm == "center") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::onCenterChanged);
        return true;
    } else if (nm == "m") {
        if ((prop->parent()->name() == "tf") && (prop->parent()->parent() == _propertyRoot)) {
            connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markTransformsDirty);
            return true;
        } else {
            qWarning() << "saw confusing 'm' property" << prop->path();
        }
    } else if (nm == "m-geo") {
        // ignore for now, we do geo projection server-side
        return true;
    } else if (nm == "id") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markSVGIDDirty);
        return true;
    } else if (nm == "update") {
        // disable updates optionally?
        return true;
    } else if ((nm == "clip") || (nm == "clip-frame")) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markClipDirty);
        return true;
    }

    if (isStyleProperty(nm)) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markStyleDirty);
        return true;
    }

    if (nm == "symbol-type") {
        // ignored for now
        return true;
    }

    if (nm == "layer-type") {
        connect(prop, &LocalProp::valueChanged, [this](QVariant value)
        {qDebug() << "layer-type:" << value.toByteArray() << "on" << _propertyRoot->path(); });
        return true;
    } else if (nm == "z-index") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markZIndexDirty);
        return true;
    }

    return false;
}

bool FGCanvasElement::onChildRemoved(LocalProp *prop)
{
    const QByteArray nm = prop->name();
    if ((nm == "tf") || (nm == "m")) {
        markTransformsDirty();
        return true;
    }

    return false;
}

void FGCanvasElement::doDestroy()
{
}

QColor FGCanvasElement::fillColor() const
{
    return _fillColor;
}

void FGCanvasElement::onCenterChanged(QVariant value)
{
    LocalProp* senderProp = static_cast<LocalProp*>(sender());
    const unsigned int centerTerm = senderProp->index();

    if (centerTerm == 0) {
        _center.setX(value.toReal());
    } else {
        _center.setY(value.toReal());
    }

    requestPolish();
}

void FGCanvasElement::markTransformsDirty()
{
    _transformsDirty = true;
    requestPolish();
}

void FGCanvasElement::markClipDirty()
{
    _clipDirty = true;
    parseCSSClip(_propertyRoot->value("clip", QVariant()).toByteArray());
    _clipFrame = static_cast<ReferenceFrame>(_propertyRoot->value("clip-frame", 0).toInt());
    requestPolish();
}

double FGCanvasElement::parseCSSValue(QByteArray value) const
{
    value = value.trimmed();
    // deal with %, px suffixes
    if (value.indexOf('%') >= 0) {
        qWarning() << Q_FUNC_INFO << "extend parsing to deal with:" << value;
    }
    if (value.endsWith("px")) {
        value.truncate(value.length() - 2);
    }
    bool ok = false;
    qreal v = value.toDouble(&ok);
    if (!ok) {
        qWarning() << "failed to parse:" << value;
    }
    return v;
}

QColor FGCanvasElement::parseColorValue(QVariant value) const
{
    QString colorString = value.toString();
    if (colorString.isEmpty() || (colorString == QStringLiteral("none"))) {
        return QColor(); // return an invalid color
    }

    int alpha = 255;
    int red = 0;
    int green = 0;
    int blue = 0;
    bool good = false;

    if (colorString.startsWith('#')) {
        // web style
        if (colorString.length() == 9) {
            QRegularExpression re("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");
            QRegularExpressionMatch match = re.match(colorString);
            if (match.hasMatch()) {
                red = match.captured(1).toInt(nullptr, 16);
                green = match.captured(2).toInt(nullptr, 16);
                blue = match.captured(3).toInt(nullptr, 16);
                alpha = match.captured(4).toInt(nullptr, 16);
                good = true;
            }
        } else if (colorString.length() == 7) {
            // long form, RGB
            QRegularExpression re("#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})");
            QRegularExpressionMatch match = re.match(colorString);
            if (match.hasMatch()) {
                red = match.captured(1).toInt(nullptr, 16);
                green = match.captured(2).toInt(nullptr, 16);
                blue = match.captured(3).toInt(nullptr, 16);
                good = true;
            }
        }
    } else if (colorString.startsWith("rgb")) {
        // try rgb(ddd, ddd, ddd) syntax
        QRegularExpression re("rgb\\((\\d*),(\\d*),(\\d*)\\)");
        QRegularExpressionMatch match = re.match(value.toString());
        if (match.hasMatch()) {
            red = match.captured(1).toInt();
            green = match.captured(2).toInt();
            blue = match.captured(3).toInt();
            good = true;
        }

        QRegularExpression re2("rgba\\((\\d*),(\\d*),(\\d*),(\\d*)\\)");
        match = re2.match(value.toString());
        if (match.hasMatch()) {
            red = match.captured(1).toInt();
            green = match.captured(2).toInt();
            blue = match.captured(3).toInt();
            alpha = match.captured(4).toInt() * 255;
            good = true;
        }
    }

    if (good) {
        return QColor(red, green, blue, alpha);
    }

    qWarning() << _propertyRoot->path() << "failed to parse color:" << colorString;
    return Qt::magenta; // default horrible colour
}

void FGCanvasElement::markStyleDirty()
{
    _styleDirty = true;
    requestPolish();
    // group will cascade
}

QVariant FGCanvasElement::getCascadedStyle(const char *name, QVariant defaultValue) const
{
    LocalProp* style = _propertyRoot->childWithNameAndIndex(NameIndexTuple(name, 0));
    if (style) {
        return style->value();
    }

    if (_parent) {
        return _parent->getCascadedStyle(name);
    }

    return defaultValue;
}

void FGCanvasElement::markZIndexDirty(QVariant value)
{
    _zIndex = value.toInt();
    _parent->markChildZIndicesDirty();
}

void FGCanvasElement::markSVGIDDirty(QVariant value)
{
    _svgElementId = value.toByteArray();
}

void FGCanvasElement::onVisibleChanged(QVariant value)
{
    _visible = value.toBool();
    requestPolish();
}

void FGCanvasElement::parseCSSClip(QByteArray value)
{
    if (value.isEmpty()) {
        _hasClip = false;
        return;
    }

    // https://www.w3.org/wiki/CSS/Properties/clip for the stupid order here
    if (value.startsWith("rect(")) {
        int closingParen = value.indexOf(')');
        value = value.mid(5, closingParen - 5); // trim front portion
    }

    QByteArrayList clipRectDesc = value.split(',');
    const int parts = clipRectDesc.size();
    if (parts != 4) {
        qWarning() << "implement parsing for non-standard clip" << value;
        return;
    }

    const qreal top = parseCSSValue(clipRectDesc.at(0));
    const qreal right = parseCSSValue(clipRectDesc.at(1));
    const qreal bottom = parseCSSValue(clipRectDesc.at(2));
    const qreal left = parseCSSValue(clipRectDesc.at(3));

    _clipRect = QRectF(left, top, right - left, bottom - top);
  //  qDebug() << "final clip rect:" << _clipRect << "from" << value;
    _hasClip = true;

    requestPolish();
}
