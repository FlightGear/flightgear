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
    FGCanvasPaintContext context(&painter);

    // set scaling by canvas size

    const double verticalScaling = height() / sz.height();
    const double horizontalScaling = width() / sz.width();
    const double usedScale = std::min(verticalScaling, horizontalScaling);

    painter.scale(usedScale, usedScale);
    _rootElement->paint(&context);

}
