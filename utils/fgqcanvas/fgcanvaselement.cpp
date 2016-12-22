#include "fgcanvaselement.h"

#include "localprop.h"
#include "fgcanvaspaintcontext.h"
#include "fgcanvasgroup.h"

#include <QDebug>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

QTransform qTransformFromCanvas(LocalProp* prop)
{
    float m[6] = { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 }; // identity matrix
    for (int i =0; i< 6; ++i) {
        LocalProp* mProp =  prop->getOrCreateChildWithNameAndIndex(NameIndexTuple("m", i));
        if (!mProp->value().isNull()) {
            m[i] = mProp->value().toFloat();
        }
    }

    return QTransform(m[0], m[1], 0.0,
                      m[2], m[3], 0.0,
                      m[4], m[5], 1.0);
}

bool FGCanvasElement::isStyleProperty(QByteArray name)
{
    if ((name == "font") || (name == "line-height") || (name == "alignment")
        || (name == "character-size") || (name == "fill") || (name == "background"))
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

FGCanvasElement::FGCanvasElement(FGCanvasGroup* pr, LocalProp* prop) :
    QObject(pr),
    _propertyRoot(prop),
    _parent(pr)
{
    connect(prop->getOrCreateWithPath("visible"), &LocalProp::valueChanged,
            [this](QVariant val) { _visible = val.toBool(); });
    connect(prop, &LocalProp::childAdded, this, &FGCanvasElement::onChildAdded);
    connect(prop, &LocalProp::childRemoved, this, &FGCanvasElement::onChildRemoved);

    if (pr) {
        pr->markChildZIndicesDirty();
    }
}

void FGCanvasElement::paint(FGCanvasPaintContext *context) const
{
    if (!isVisible()) {
        return;
    }

    QPainter* p = context->painter();

    if (_clipDirty) {
        parseCSSClip(_propertyRoot->value("clip", QVariant()).toByteArray());
        _clipDirty = false;
    }

    p->save();

    if (_hasClip) {
#if 0
        p->save();
        p->setPen(Qt::yellow);
        p->setBrush(QBrush(Qt::yellow, Qt::DiagCrossPattern));
        p->drawRect(_clipRect);
        p->restore();
#endif
        context->painter()->setClipRect(_clipRect);
        context->painter()->setClipping(true);
    }

    p->setTransform(combinedTransform(), true /* combine */);

    if (_styleDirty) {
        _fillColor = parseColorValue(getCascadedStyle("fill"));
        _styleDirty = false;
    }

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

bool FGCanvasElement::onChildAdded(LocalProp *prop)
{
    const QByteArray nm = prop->name();
    if (nm == "tf") {
        connect(prop, &LocalProp::childAdded, this, &FGCanvasElement::onChildAdded);
        return true;
    } else if (nm == "visible") {
        return true;
    } else if (nm == "tf-rot-index") {
        // ignored, this is noise from the Nasal SVG parswer
        return true;
    } else if (nm.startsWith("center-offset-")) {
        // ignored, this is noise from the Nasal SVG parswer
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
        connect(prop, &LocalProp::valueChanged, [this](QVariant value) { _svgElementId = value.toByteArray(); });
        return true;
    } else if (prop->name() == "update") {
        // disable updates optionally?
        return true;
    } else if (nm == "clip") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markClipDirty);
        return true;
    }

    if (isStyleProperty(nm)) {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasElement::markStyleDirty);
        return true;
    }

    if (prop->name() == "layer-type") {
        connect(prop, &LocalProp::valueChanged, [this](QVariant value)
        {qDebug() << "layer-type:" << value.toByteArray() << "on" << _propertyRoot->path(); });
        return true;
    } else if (prop->name() == "z-index") {
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

QColor FGCanvasElement::fillColor() const
{
    return _fillColor;
}

void FGCanvasElement::onCenterChanged(QVariant value)
{
    LocalProp* senderProp = static_cast<LocalProp*>(sender());
    int centerTerm = senderProp->index();

    if (centerTerm == 0) {
        _center.setX(value.toFloat());
    } else {
        _center.setY(value.toFloat());
    }
}

void FGCanvasElement::markTransformsDirty()
{
    _transformsDirty = true;
}

void FGCanvasElement::markClipDirty()
{
    _clipDirty = true;
}

float FGCanvasElement::parseCSSValue(QByteArray value) const
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
    float v = value.toFloat(&ok);
    if (!ok) {
        qWarning() << "failed to parse:" << value;
    }
    return v;
}

QColor FGCanvasElement::parseColorValue(QVariant value) const
{
    QString colorString = value.toString();
    if (colorString.isEmpty() || colorString == "none") {
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

void FGCanvasElement::onVisibleChanged(QVariant value)
{
    _visible = value.toBool();
}

void FGCanvasElement::parseCSSClip(QByteArray value) const
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

    const float top = parseCSSValue(clipRectDesc.at(0));
    const float right = parseCSSValue(clipRectDesc.at(1));
    const float bottom = parseCSSValue(clipRectDesc.at(2));
    const float left = parseCSSValue(clipRectDesc.at(3));

    _clipRect = QRectF(left, top, right - left, bottom - top);
    qDebug() << "final clip rect:" << _clipRect << "from" << value;
    _hasClip = true;
}
