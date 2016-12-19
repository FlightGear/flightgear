#include "fgcanvastext.h"

#include <QPainter>
#include <QDebug>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"

FGCanvasText::FGCanvasText(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop),
    _metrics(QFont())
{
}

void FGCanvasText::doPaint(FGCanvasPaintContext *context) const
{
    if (_fontDirty) {
        rebuildFont();
        _fontDirty = false;
    }

    context->painter()->setFont(_font);
    context->painter()->setPen(fillColor());
    context->painter()->setBrush(Qt::NoBrush);
    QRectF rect(0, 0, 1000, 1000);

    if (_alignment & Qt::AlignBottom) {
        rect.moveBottom(0.0);
    } else if (_alignment & Qt::AlignVCenter) {
        rect.moveCenter(QPointF(rect.center().x(), 0.0));
    } else if (_alignment & Qt::AlignBaseline) {
        // this is really annoying. Point-based drawing would align
        // with the baseline automatically, but with no line-wrapping
        // we need to work out the offset from the top of the box to
        // the base line. font metrics time!
        rect.moveTop(-_metrics.ascent());
    }

    if (_alignment & Qt::AlignRight) {
        rect.moveRight(0.0);
    } else if (_alignment & Qt::AlignHCenter) {
        rect.moveCenter(QPointF(0.0, rect.center().y()));
    }

    context->painter()->drawText(rect, _alignment, _text);
}

void FGCanvasText::markStyleDirty()
{
    markFontDirty();
}

bool FGCanvasText::onChildAdded(LocalProp *prop)
{
    if (FGCanvasElement::onChildAdded(prop)) {
        return true;
    }

    if (prop->name() == "text") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasText::onTextChanged);
        return true;
    }

    if (prop->name() == "draw-mode") {
        connect(prop, &LocalProp::valueChanged, this, &FGCanvasText::setDrawMode);
        return true;
    }

    if (prop->name() == "character-aspect-ratio") {
        return true;
    }

    qDebug() << "text saw child:" << prop->name() << prop->index();
    return false;
}

void FGCanvasText::onTextChanged(QVariant var)
{
    _text = var.toString();
}

void FGCanvasText::setDrawMode(QVariant var)
{
    int mode = var.toInt();
    if (mode != 1) {
        qDebug() << _propertyRoot->path() << "draw mode is now" << mode;
    }
}

void FGCanvasText::rebuildAlignment(QVariant var) const
{
    QByteArray alignString = var.toByteArray();
    if (alignString.isEmpty()) {
        _alignment = Qt::AlignBaseline | Qt::AlignLeft;
        return;
    }

    if (alignString == "center") {
        _alignment = Qt::AlignCenter;
        return;
    }

    Qt::Alignment newAlignment;
    if (alignString.startsWith("left-")) {
        newAlignment |= Qt::AlignLeft;
    } else if (alignString.startsWith("center-")) {
        newAlignment |= Qt::AlignHCenter;
    } else if (alignString.startsWith("right-")) {
        newAlignment |= Qt::AlignRight;
    }

    if (alignString.endsWith("-baseline")) {
        newAlignment |= Qt::AlignBaseline;
    } else if (alignString.endsWith("-top")) {
        newAlignment |= Qt::AlignTop;
    } else if (alignString.endsWith("-bottom")) {
        newAlignment |= Qt::AlignBottom;
    } else if (alignString.endsWith("-center")) {
        newAlignment |= Qt::AlignVCenter;
    }

    if (newAlignment == 0) {
        qWarning() << "implement me" << alignString;
    }

    _alignment = newAlignment;
}

void FGCanvasText::markFontDirty()
{
    _fontDirty = true;
}

void FGCanvasText::rebuildFont() const
{
    QString fontName = getCascadedStyle("font", QString()).toString();
    QFont f(fontName);
    f.setPixelSize(getCascadedStyle("character-size", 16).toInt());
    _font = f;
    _metrics = QFontMetricsF(_font);
    rebuildAlignment(getCascadedStyle("alignment"));
}

