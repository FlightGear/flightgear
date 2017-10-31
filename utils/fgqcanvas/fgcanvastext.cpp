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

#include "fgcanvastext.h"

#include <QPainter>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"
#include "fgqcanvasfontcache.h"
#include "canvasitem.h"

static QQmlComponent* static_textComponent = nullptr;

FGCanvasText::FGCanvasText(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop),
    _metrics(QFont())
{
    // this signal fires infrequently enough, it's simpler just to have
    // all texts watch it.
    connect(FGQCanvasFontCache::instance(), &FGQCanvasFontCache::fontLoaded,
            this, &FGCanvasText::onFontLoaded);
}

CanvasItem *FGCanvasText::createQuickItem(QQuickItem *parent)
{
    Q_ASSERT(static_textComponent);
    _quickItem = qobject_cast<CanvasItem*>(static_textComponent->create());
    _quickItem->setParentItem(parent);
    markFontDirty(); // so it gets set on the new item
    return _quickItem;
}

CanvasItem *FGCanvasText::quickItem() const
{
    return _quickItem;
}

void FGCanvasText::setEngine(QQmlEngine *engine)
{
    static_textComponent = new QQmlComponent(engine, QUrl("text.qml"));
    qDebug() << static_textComponent->errorString();
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
    if (_quickItem) {
        _quickItem->setProperty("text", var);
    }
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

void FGCanvasText::onFontLoaded(QByteArray name)
{
    QByteArray fontName = getCascadedStyle("font", QString()).toByteArray();
    if (name != fontName) {
        return; // not our font
    }

    markFontDirty();
}

void FGCanvasText::rebuildFont() const
{
    QByteArray fontName = getCascadedStyle("font", QString()).toByteArray();
    bool ok;
    QFont f = FGQCanvasFontCache::instance()->fontForName(fontName, &ok);
    if (!ok) {
        // wait for the correct font
    }

    const int pixelSize = getCascadedStyle("character-size", 16).toInt();
    f.setPixelSize(pixelSize);
    _font = f;
    _metrics = QFontMetricsF(_font);
    rebuildAlignment(getCascadedStyle("alignment"));

    if (_quickItem) {
        _quickItem->setProperty("fontFamily", f.family());
        _quickItem->setProperty("fontPixelSize", pixelSize);

        QColor fill = fillColor();
        _quickItem->setProperty("color", fill.name());
    }
}

