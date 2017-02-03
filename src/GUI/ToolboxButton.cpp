#include "ToolboxButton.h"

#include <QPainter>
#include <QFontMetrics>

const int ICON_SIZE = 50;

ToolboxButton::ToolboxButton(QWidget *pr) :
    QAbstractButton(pr)
{

}

void ToolboxButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    const int iconLeft = (width() - ICON_SIZE) / 2;
    QRect iconRect(iconLeft, 0, ICON_SIZE, ICON_SIZE);

    icon().paint(&painter, iconRect);

    painter.setPen(Qt::white);
    QRect textRect = rect();
    textRect.setTop(ICON_SIZE);
    painter.drawText(textRect, Qt::AlignHCenter, text());
}

QSize ToolboxButton::sizeHint() const
{
    QSize icon(ICON_SIZE, ICON_SIZE);

    QFontMetrics metrics(font());
    QRect bounds = metrics.boundingRect(text());

    return QSize(qMax(ICON_SIZE, bounds.width()), ICON_SIZE + bounds.height());
}
