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
#include <QTextLayout>

#include <private/qquicktextnode_p.h>

#include "fgcanvaspaintcontext.h"
#include "localprop.h"
#include "fgqcanvasfontcache.h"
#include "canvasitem.h"
#include "canvasconnection.h"

class TextCanvasItem : public CanvasItem
{
    Q_OBJECT

public:
    TextCanvasItem(QQuickItem* parent)
        : CanvasItem(parent)
    {
        setFlag(ItemHasContents);
    }

    void setText(QString t)
    {
        if (t == m_text) {
            return;
        }

        m_text = t;
        updateTextLayout();
        update();
    }

    void setColor(QColor c)
    {
        m_color = c;
        update();
    }

    void setAlignment(Qt::Alignment textAlign)
    {
        m_alignment = textAlign;
        updateTextLayout();
        update();
    }

    void setFont(QFont font)
    {
        m_font = font;
        updateTextLayout();
        update();
    }

    QSGNode* updateRealPaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData *data) override
    {

        if (!m_textNode) {
            m_textNode = new QQuickTextNode(this);
        }

        m_textNode->deleteContent();
        QPointF pos = posForAlignment();

        m_textNode->addTextLayout(pos,
                                  &m_layout,
                                  m_color,
                                  QQuickText::Normal);

        return m_textNode;
    }

protected:
    QPointF posForAlignment()
    {
        float x = 0.0f;
        float y = 0.0f;
        const float itemWidth = width();
        const float itemHeight = height();
        const float textWidth = m_layout.boundingRect().width();
        const float textHeight = m_layout.boundingRect().height();

        switch (m_alignment & Qt::AlignHorizontal_Mask) {
        case Qt::AlignLeft:
        case Qt::AlignJustify:
            break;
        case Qt::AlignRight:
            x = itemWidth - textWidth;
            break;
        case Qt::AlignHCenter:
            x = (itemWidth - textWidth) / 2;
            break;
        }

        switch (m_alignment & Qt::AlignVertical_Mask) {
        case Qt::AlignTop:
            break;
        case Qt::AlignBottom:
            y = itemHeight - textHeight;
            break;
        case Qt::AlignVCenter:
            y = (itemHeight - textHeight) / 2;
            break;
        case Qt::AlignBaseline:
            y = -m_baselineOffset;
            break;
        }

        return QPointF(x,y);
    }

    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override
    {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        update();
    }

    QRectF boundingRect() const override
    {
        if ((width() == 0.0) || (height() == 0.0)) {
            return m_layout.boundingRect();
        }

        return QQuickItem::boundingRect();
    }

    void updateTextLayout()
    {
        QFontMetricsF fm(m_font);
        m_baselineOffset = fm.ascent(); // for aligning to first base line

        m_layout.setText(m_text);
        QTextOption textOpt(m_alignment);
        m_layout.setTextOption(textOpt);
        m_layout.setFont(m_font);

        m_layout.beginLayout();
        float leading = fm.leading();
        int lineCount = 0;
        float y = 0.0;

        QTextLine line = m_layout.createLine();
        while (line.isValid()) {
            int nextBreak = m_text.indexOf('\n', line.textStart());
            int columnsToNextBreak = nextBreak >= 0 ? nextBreak : INT_MAX;
            line.setNumColumns(columnsToNextBreak);
            line.setPosition(QPointF(0.0, y));
            ++lineCount;
            y += leading + line.height();
            line = m_layout.createLine();
        }

        m_layout.endLayout();
    }

private:
    QQuickTextNode* m_textNode = nullptr;
    QColor m_color;
    QFont m_font;
    QString m_text;
    Qt::Alignment m_alignment = Qt::AlignCenter;
    QTextLayout m_layout;
    float m_baselineOffset;
};

FGCanvasText::FGCanvasText(FGCanvasGroup* pr, LocalProp* prop) :
    FGCanvasElement(pr, prop),
    _metrics(QFont())
{
}

CanvasItem *FGCanvasText::createQuickItem(QQuickItem *parent)
{
    _quickItem = new TextCanvasItem(parent);
    markFontDirty(); // so it gets set on the new item
    return _quickItem;
}

CanvasItem *FGCanvasText::quickItem() const
{
    return _quickItem;
}

void FGCanvasText::dumpElement()
{
    qDebug() << "Text:" << _text << " at " << _propertyRoot->path();
}

void FGCanvasText::doPaint(FGCanvasPaintContext *context) const
{
    context->painter()->setFont(_font);
    QColor c = fillColor();
    if (!c.isValid()) {
        c = Qt::white;
    }

    context->painter()->setPen(c);
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

   // context->painter()->setPen(Qt::cyan);
   // context->painter()->drawRect(rect);
}

void FGCanvasText::doPolish()
{
    if (_fontDirty) {
        rebuildFont();
        _fontDirty = false;
    }
}

void FGCanvasText::markStyleDirty()
{
    markFontDirty();
}

void FGCanvasText::doDestroy()
{
    delete _quickItem;
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
        _quickItem->setText(var.toString());
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

        if (_quickItem) {
            _quickItem->setAlignment(_alignment);
        }
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
    if (_quickItem) {
        _quickItem->setAlignment(_alignment);
    }
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

    auto fontCache = connection()->fontCache();
    disconnect(fontCache, &FGQCanvasFontCache::fontLoaded, this, &FGCanvasText::onFontLoaded);
    markFontDirty();
}

void FGCanvasText::rebuildFont() const
{
    QByteArray fontName = getCascadedStyle("font", QString()).toByteArray();
    bool ok;
    auto fontCache = connection()->fontCache();
    QFont f = fontCache->fontForName(fontName, &ok);
    if (!ok) {
        // wait for the correct font
        connect(fontCache, &FGQCanvasFontCache::fontLoaded, this, &FGCanvasText::onFontLoaded);
        return;
    }

    const int pixelSize = getCascadedStyle("character-size", 16).toInt();
    f.setPixelSize(pixelSize);
    _font = f;
    _metrics = QFontMetricsF(_font);
    rebuildAlignment(getCascadedStyle("alignment"));

    if (_quickItem) {
        _quickItem->setFont(f);
        _quickItem->setColor(fillColor());
       // _quickItem->setProperty("fontPixelSize", pixelSize);
    }
}

#include "fgcanvastext.moc"
