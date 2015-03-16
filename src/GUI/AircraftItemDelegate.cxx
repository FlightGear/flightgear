// AircraftItemDelegate.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started March 2015.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
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


#include "AircraftItemDelegate.hxx"

#include <QDebug>
#include <QPainter>
#include <QLinearGradient>
#include <QListView>
#include <QMouseEvent>
#include <QFontMetrics>

#include "AircraftModel.hxx"

AircraftItemDelegate::AircraftItemDelegate(QListView* view) :
    m_view(view)
{
    view->viewport()->installEventFilter(this);
    view->viewport()->setMouseTracking(true);

    m_leftArrowIcon.load(":/left-arrow-icon");
    m_rightArrowIcon.load(":/right-arrow-icon");
}

void AircraftItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->setRenderHint(QPainter::Antialiasing);
    // selection feedback rendering
    if (option.state & QStyle::State_Selected) {
        QLinearGradient grad(option.rect.topLeft(), option.rect.bottomLeft());
        grad.setColorAt(0.0, QColor(152, 163, 180));
        grad.setColorAt(1.0, QColor(90, 107, 131));

        QBrush backgroundBrush(grad);
        painter->fillRect(option.rect, backgroundBrush);

        painter->setPen(QColor(90, 107, 131));
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
    }

    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    quint32 yPos = contentRect.center().y() - (thumbnail.height() / 2);
    painter->drawPixmap(contentRect.left(), yPos, thumbnail);

    // draw 1px frame
    painter->setPen(QColor(0x7f, 0x7f, 0x7f));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(contentRect.left(), yPos, thumbnail.width(), thumbnail.height());

    int variantCount = index.data(AircraftVariantCountRole).toInt();
    int currentVariant =index.data(AircraftVariantRole).toInt();
    QString description = index.data(Qt::DisplayRole).toString();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

    painter->setPen(Qt::black);
    QFont f;
    f.setPointSize(18);
    painter->setFont(f);

    QRect descriptionRect = contentRect.adjusted(ARROW_SIZE, 0, -ARROW_SIZE, 0),
        actualBounds;

    if (variantCount > 0) {
        bool canLeft = (currentVariant > 0);
        bool canRight =  (currentVariant < variantCount );

        QRect leftArrowRect = leftCycleArrowRect(option.rect, index);
        if (canLeft) {
            painter->drawPixmap(leftArrowRect.topLeft() + QPoint(2, 2), m_leftArrowIcon);
        }

        QRect rightArrowRect = rightCycleArrowRect(option.rect, index);
        if (canRight) {
            painter->drawPixmap(rightArrowRect.topLeft() + QPoint(2, 2), m_rightArrowIcon);
        }
    }

    painter->drawText(descriptionRect, Qt::TextWordWrap, description, &actualBounds);

    QString authors = index.data(AircraftAuthorsRole).toString();

    f.setPointSize(12);
    painter->setFont(f);

    if (!authors.isEmpty()) {
        QRect authorsRect = descriptionRect;
        authorsRect.moveTop(actualBounds.bottom() + MARGIN);
        painter->drawText(authorsRect, Qt::TextWordWrap,
                          QString("by: %1").arg(authors),
                          &actualBounds);
    }

    QString longDescription = index.data(AircraftLongDescriptionRole).toString();
    if (!longDescription.isEmpty()) {
        QRect longDescriptionRect = descriptionRect;
        longDescriptionRect.moveTop(actualBounds.bottom() + MARGIN);
        painter->drawText(longDescriptionRect, Qt::TextWordWrap,
                          longDescription, &actualBounds);
    }

    QRect r = contentRect;
    r.setWidth(contentRect.width() / 3);
    r.moveTop(actualBounds.bottom() + MARGIN);
    r.moveLeft(r.right());
    r.setHeight(24);

    drawRating(painter, "Flight model:", r, index.data(AircraftRatingRole).toInt());
    r.moveTop(r.bottom());
    drawRating(painter, "Systems:", r, index.data(AircraftRatingRole + 1).toInt());

    r.moveTop(actualBounds.bottom() + MARGIN);
    r.moveLeft(r.right());
    drawRating(painter, "Cockpit:", r, index.data(AircraftRatingRole + 2).toInt());
    r.moveTop(r.bottom());
    drawRating(painter, "Exterior model:", r, index.data(AircraftRatingRole + 3).toInt());

    QVariant v = index.data(AircraftPackageStatusRole);
    AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
    status = PackageNotInstalled;
    if (status != PackageInstalled) {
        painter->setBrush(Qt::NoBrush);
        QRect buttonRect = packageButtonRect(option.rect, index);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(27, 122, 211));
        painter->drawRoundedRect(buttonRect, 5, 5);
        painter->setPen(Qt::white);

        if (status == PackageNotInstalled) {
            painter->drawText(buttonRect, Qt::AlignCenter, "Install");
        } else if (status == PackageUpdateAvailable) {
            painter->drawText(buttonRect, Qt::AlignCenter, "Update");
        }
    }
}

QSize AircraftItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

    QFont f;
    f.setPointSize(18);
    QFontMetrics metrics(f);

    int textHeight = metrics.boundingRect(contentRect, Qt::TextWordWrap,
                                          index.data().toString()).height();

    f.setPointSize(12);
    QFontMetrics smallMetrics(f);

    QString authors = index.data(AircraftAuthorsRole).toString();
    if (!authors.isEmpty()) {
        textHeight += MARGIN;
        textHeight += smallMetrics.boundingRect(contentRect, Qt::TextWordWrap, authors).height();
    }

    QString desc = index.data(AircraftLongDescriptionRole).toString();
    if (!desc.isEmpty()) {
        textHeight += MARGIN;
        textHeight += smallMetrics.boundingRect(contentRect, Qt::TextWordWrap, desc).height();
    }

    // ratings
    textHeight += 48; // (24px per rating box)

    textHeight = qMax(textHeight, 128);

    return QSize(option.rect.width(), textHeight + (MARGIN * 2));
}

bool AircraftItemDelegate::eventFilter( QObject*, QEvent* event )
{
    if ( event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        QModelIndex index = m_view->indexAt( me->pos() );
        int variantCount = index.data(AircraftVariantCountRole).toInt();
        int variantIndex = index.data(AircraftVariantRole).toInt();

        if ( (event->type() == QEvent::MouseButtonRelease) && (variantCount > 0) )
        {
            QRect vr = m_view->visualRect(index);
            QRect leftCycleRect = leftCycleArrowRect(vr, index),
                rightCycleRect = rightCycleArrowRect(vr, index);

            if ((variantIndex > 0) && leftCycleRect.contains(me->pos())) {
                m_view->model()->setData(index, variantIndex - 1, AircraftVariantRole);
                emit variantChanged(index);
                return true;
            } else if ((variantIndex < variantCount) && rightCycleRect.contains(me->pos())) {
                m_view->model()->setData(index, variantIndex + 1, AircraftVariantRole);
                emit variantChanged(index);
                return true;
            }
        }
    } else if ( event->type() == QEvent::MouseMove ) {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        QModelIndex index = m_view->indexAt( me->pos() );
        QRect vr = m_view->visualRect(index);

        if (packageButtonRect(vr, index).contains(me->pos())) {
            qDebug() << "mouse inside button";
        }

    }
    
    return false;
}


QRect AircraftItemDelegate::leftCycleArrowRect(const QRect& visualRect, const QModelIndex& index) const
{
    QRect contentRect = visualRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

    QRect r = contentRect;
    r.setRight(r.left() + ARROW_SIZE);
    r.setBottom(r.top() + ARROW_SIZE);
    return r;

}

QRect AircraftItemDelegate::rightCycleArrowRect(const QRect& visualRect, const QModelIndex& index) const
{
    QRect contentRect = visualRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

    QRect r = contentRect;
    r.setLeft(r.right() - ARROW_SIZE);
    r.setBottom(r.top() + ARROW_SIZE);
    return r;

}

QRect AircraftItemDelegate::packageButtonRect(const QRect& visualRect, const QModelIndex& index) const
{
    QRect contentRect = visualRect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnail.width());

    return QRect(contentRect.left() + ARROW_SIZE, contentRect.bottom() - 24, 60, 24);
}

void AircraftItemDelegate::drawRating(QPainter* painter, QString label, const QRect& box, int value) const
{
    const int DOT_SIZE = 10;
    const int DOT_MARGIN = 4;

    QRect dotBox = box;
    dotBox.setLeft(box.right() - (DOT_MARGIN * 6 + DOT_SIZE * 5));

    painter->setPen(Qt::black);
    QRect textBox = box;
    textBox.setRight(dotBox.left() - DOT_MARGIN);
    painter->drawText(textBox, Qt::AlignVCenter | Qt::AlignRight, label);

    painter->setPen(Qt::NoPen);
    QRect dot(dotBox.left() + DOT_MARGIN,
              dotBox.center().y() - (DOT_SIZE / 2),
              DOT_SIZE,
              DOT_SIZE);
    for (int i=0; i<5; ++i) {
        painter->setBrush((i < value) ? QColor(0x3f, 0x3f, 0x3f) : QColor(0xaf, 0xaf, 0xaf));
        painter->drawEllipse(dot);
        dot.moveLeft(dot.right() + DOT_MARGIN);
    }
}
