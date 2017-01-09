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

#include "fgcanvaswidget.h"

#include <QDebug>
#include <QPainter>

#include "localprop.h"
#include "fgcanvasgroup.h"
#include "fgcanvaspaintcontext.h"

FGCanvasWidget::FGCanvasWidget(QWidget *parent) :
    QWidget(parent)
{
}

void FGCanvasWidget::setRootProperty(LocalProp* root)
{
    _canvasRoot = root;
    _rootElement = new FGCanvasGroup(nullptr, root);

    update();
}

FGCanvasGroup *FGCanvasWidget::rootElement()
{
    return _rootElement;
}

void FGCanvasWidget::paintEvent(QPaintEvent *pe)
{
    if (!_rootElement) {
        return;
    }

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QSizeF sz(_canvasRoot->value("size[0]", 512).toInt(),
             _canvasRoot->value("size[1]", 512).toInt());


    // set scaling by canvas size

    const double verticalScaling = height() / sz.height();
    const double horizontalScaling = width() / sz.width();
    const double usedScale = std::min(verticalScaling, horizontalScaling);

    painter.scale(usedScale, usedScale);

    // captures the view scaling
    FGCanvasPaintContext context(&painter);
    _rootElement->paint(&context);

}
