#include "ToolboxButton.h"

#include <QPainter>
#include <QFontMetrics>
#include <QPalette>

const int ICON_SIZE = 50;
const int INDICATOR_WIDTH = 4;
const int PADDING = 8;

ToolboxButton::ToolboxButton(QWidget *pr) :
    QAbstractButton(pr)
{
    setCheckable(true);
}

void ToolboxButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    if (isChecked()) {
        QRect indicatorRect = rect();
        indicatorRect.setWidth(INDICATOR_WIDTH);
        painter.fillRect(indicatorRect, Qt::white);
    }

    const int iconLeft = (width() - ICON_SIZE) / 2;
    QRect iconRect(iconLeft, PADDING, ICON_SIZE, ICON_SIZE);

    // QIcon can paint itself disabled, but it's hard to synchronize the
    // text appearance with that
    icon().paint(&painter, iconRect, Qt::AlignCenter,
                 isEnabled() ? QIcon::Normal : QIcon::Disabled);

    // white with 50% opacity is what Mac style uses
    const QColor iconColor = isEnabled() ? Qt::white : QColor(255, 255, 255, 128);
    painter.setPen(iconColor);
    QRect textRect = rect();
    textRect.setTop(iconRect.bottom());
    painter.drawText(textRect, Qt::AlignHCenter, text());
}

QSize ToolboxButton::sizeHint() const
{
    QSize icon(ICON_SIZE, ICON_SIZE);

    QFontMetrics metrics(font());
    QRect bounds = metrics.boundingRect(text());

    const int PAD_2 = PADDING * 2;
    return QSize(qMax(ICON_SIZE, bounds.width()) + PAD_2, ICON_SIZE + bounds.height() + PAD_2);
}
