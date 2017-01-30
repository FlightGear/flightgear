#include "AdvancedSettingsButton.h"

#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QDebug>

const int MARGIN = 3;

AdvancedSettingsButton::AdvancedSettingsButton()
{
    setCheckable(true);
    setChecked(false);
    setIcon(QIcon());

    connect(this, &QAbstractButton::toggled, this, &AdvancedSettingsButton::updateUi);

    updateUi();
}

void AdvancedSettingsButton::paintEvent(QPaintEvent *event)
{
    QPixmap icon(":/settings-gear-white");

    QPainter painter(this);

    painter.setPen(Qt::white);
    const int h = height();
    QRect textRect = rect();
    textRect.setRight(textRect.right() - (h + MARGIN));
    painter.drawText(textRect, text(), Qt::AlignVCenter | Qt::AlignRight);

    QRect iconRect = rect();
    iconRect.setLeft(iconRect.right() - h);
    iconRect.adjust(MARGIN, MARGIN, -MARGIN, -MARGIN);
    painter.drawPixmap(iconRect, icon);
}

QSize AdvancedSettingsButton::sizeHint() const
{
    const QFontMetrics f(font());
    const QRect bounds = f.boundingRect(text());
    const int height = bounds.height() + (MARGIN * 2);
    return QSize(bounds.width() + 100 + height, height);
}

void AdvancedSettingsButton::updateUi()
{
    const bool showAdvanced = isChecked();
    setText(showAdvanced ? tr("Show less") : tr("Show more"));
    updateGeometry();
}
