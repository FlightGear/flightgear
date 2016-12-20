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

const int DOT_SIZE = 11;
const int DOT_MARGIN = 2;

int dotBoxWidth()
{
    return (DOT_MARGIN * 6 + DOT_SIZE * 5);
}

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
    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    QVariant v = index.data(AircraftPackageStatusRole);
    const AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
    if (status == MessageWidget) {
        painter->setPen(QColor(0x7f, 0x7f, 0x7f));
        painter->setBrush(Qt::NoBrush);

        // draw bottom dividing line
        painter->drawLine(contentRect.left(), contentRect.bottom() + MARGIN,
                          contentRect.right(), contentRect.bottom() + MARGIN);

        return;
    }

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


    QPixmap thumbnail = index.data(Qt::DecorationRole).value<QPixmap>();
    quint32 yPos = contentRect.center().y() - (thumbnail.height() / 2);
    painter->drawPixmap(contentRect.left(), yPos, thumbnail);

    // draw 1px frame
    painter->setPen(QColor(0x7f, 0x7f, 0x7f));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(contentRect.left(), yPos, thumbnail.width(), thumbnail.height());

    // draw bottom dividing line
    painter->drawLine(contentRect.left(), contentRect.bottom() + MARGIN,
                      contentRect.right(), contentRect.bottom() + MARGIN);

    int variantCount = index.data(AircraftVariantCountRole).toInt();
    int currentVariant = index.data(AircraftVariantRole).toInt();
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
    QFontMetrics smallMetrics(f);

    painter->setFont(f);

    if (!authors.isEmpty()) {
        // ellide this beyond some maximum size, with a click to expand?
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

    painter->setRenderHint(QPainter::Antialiasing, true);

    if (index.data(AircraftHasRatingsRole).toBool()) {
        int ratingsWidth = smallMetrics.width("Flight model:") + dotBoxWidth();

        QRect r = contentRect;
        r.setWidth(ratingsWidth);
        r.moveLeft(contentRect.right() - (ratingsWidth * 2));
        r.moveTop(actualBounds.bottom() + MARGIN);
        r.setHeight(qMax(24, smallMetrics.height() + MARGIN));

        drawRating(painter, "Flight model:", r, index.data(AircraftRatingRole).toInt());
        r.moveTop(r.bottom());
        drawRating(painter, "Systems:", r, index.data(AircraftRatingRole + 1).toInt());

        r.moveTop(actualBounds.bottom() + MARGIN);
        r.moveLeft(r.right());
        drawRating(painter, "Cockpit:", r, index.data(AircraftRatingRole + 2).toInt());
        r.moveTop(r.bottom());
        drawRating(painter, "Exterior:", r, index.data(AircraftRatingRole + 3).toInt());
    }

    double downloadFraction = 0.0;
    QString buttonText, infoText;
    QColor buttonColor(27, 122, 211);
    
    double sizeInMBytes = index.data(AircraftPackageSizeRole).toInt();
    sizeInMBytes /= 0x100000;
    
    if (status == PackageDownloading) {
        buttonText = tr("Cancel");
        double downloadedMB = index.data(AircraftInstallDownloadedSizeRole).toInt();
        downloadedMB /= 0x100000;
        infoText = QStringLiteral("%1 MB of %2 MB").arg(downloadedMB, 0, 'f', 1).arg(sizeInMBytes, 0, 'f', 1);
        buttonColor = QColor(0xcf, 0xcf, 0xcf);
        downloadFraction = downloadedMB / sizeInMBytes;
    } else if (status == PackageQueued) {
        buttonText = tr("Cancel");
        infoText = tr("Waiting to download %1 MB").arg(sizeInMBytes, 0, 'f', 1);
        buttonColor = QColor(0xcf, 0xcf, 0xcf);
    } else {
        infoText = QStringLiteral("%1MB").arg(sizeInMBytes, 0, 'f', 1);
        if (status == PackageNotInstalled) {
            buttonText = tr("Install");
        } else if (status == PackageUpdateAvailable) {
            buttonText = tr("Update");
        } else if (status == PackageInstalled) {
            bool canUninstall = index.data(AircraftPackageIdRole).isValid(); // local aircraft have no package ID
            if (canUninstall) {
                buttonText = tr("Uninstall");
            } else {
                infoText.clear();
            }
        }
    }

    QRect buttonRect = packageButtonRect(option.rect, index);
    if (!buttonText.isEmpty()) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(Qt::NoPen);
        painter->setBrush(buttonColor);
        painter->drawRoundedRect(buttonRect, 5, 5);
        painter->setPen(Qt::white);
        painter->drawText(buttonRect, Qt::AlignCenter, buttonText);
    }

    QRect infoTextRect = buttonRect;
    infoTextRect.setLeft(buttonRect.right() + MARGIN);
    infoTextRect.setWidth(200);
    
    if (status == PackageDownloading) {
        QRect progressRect = infoTextRect;
        progressRect.setHeight(6);
        painter->setPen(QPen(QColor(0xcf, 0xcf, 0xcf), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(progressRect, 3, 3);
        infoTextRect.setTop(progressRect.bottom() + 1);

        QRect progressBarRect = progressRect.marginsRemoved(QMargins(2, 2, 2, 2));
        
        progressBarRect.setWidth(static_cast<int>(progressBarRect.width() * downloadFraction));
        
        painter->setBrush(QColor(27, 122, 211));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(progressBarRect, 2, 2);
    }

    if (!infoText.isEmpty()) {
        painter->setPen(Qt::black);
        painter->drawText(infoTextRect, Qt::AlignLeft | Qt::AlignVCenter, infoText);
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
}

QSize AircraftItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QVariant v = index.data(AircraftPackageStatusRole);
    const AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());

    if (status == MessageWidget) {
        QSize r = option.rect.size();
        r.setHeight(100);
        return r;
    }

    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    QSize thumbnailSize = index.data(AircraftThumbnailSizeRole).toSize();
    contentRect.setLeft(contentRect.left() + MARGIN + thumbnailSize.width());
    contentRect.setBottom(9999); // large value to avoid clipping
    contentRect.adjust(ARROW_SIZE, 0, -ARROW_SIZE, 0);

    QFont f;
    f.setPointSize(18);
    QFontMetrics metrics(f);

    int textHeight = metrics.boundingRect(contentRect, Qt::TextWordWrap,
                                          index.data().toString()).height();

    f.setPointSize(12);
    QFontMetrics smallMetrics(f);

    QString authors = QString("by: %1").arg(index.data(AircraftAuthorsRole).toString());

    if (!authors.isEmpty()) {
        textHeight += MARGIN;
        textHeight += smallMetrics.boundingRect(contentRect, Qt::TextWordWrap, authors).height();
    }

    QString desc = index.data(AircraftLongDescriptionRole).toString();
    if (!desc.isEmpty()) {
        textHeight += MARGIN;
        textHeight += smallMetrics.boundingRect(contentRect, Qt::TextWordWrap, desc).height();
    }

    if (index.data(AircraftHasRatingsRole).toBool()) {
        // ratings
        int ratingHeight = qMax(24, smallMetrics.height() + MARGIN);
        textHeight += ratingHeight * 2;
    }

    textHeight += BUTTON_HEIGHT;

    textHeight = qMax(textHeight, thumbnailSize.height());
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
        QRect vr = m_view->visualRect(index);

        if ( (event->type() == QEvent::MouseButtonRelease) && (variantCount > 0) )
        {
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
        
        if ((event->type() == QEvent::MouseButtonRelease) &&
            packageButtonRect(vr, index).contains(me->pos()))
        {
            QVariant v = index.data(AircraftPackageStatusRole);
            const AircraftItemStatus status = static_cast<AircraftItemStatus>(v.toInt());
            if (status == PackageNotInstalled) {
                emit requestInstall(index);
            } else if ((status == PackageDownloading) || (status == PackageQueued)) {
                emit cancelDownload(index);
            } else if (status == PackageUpdateAvailable) {
                emit requestInstall(index);
            } else if (status == PackageInstalled) {
                emit requestUninstall(index);
            }
            
            return true;
        }
    } else if ( event->type() == QEvent::MouseMove ) {

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

    return QRect(contentRect.left() + ARROW_SIZE, contentRect.bottom() - BUTTON_HEIGHT,
                 BUTTON_WIDTH, BUTTON_HEIGHT);
}

void AircraftItemDelegate::drawRating(QPainter* painter, QString label, const QRect& box, int value) const
{
    QRect dotBox = box;
    dotBox.setLeft(box.right() - dotBoxWidth());

    painter->setPen(Qt::black);
    QRect textBox = box;
    textBox.setRight(dotBox.left());
    painter->drawText(textBox, Qt::AlignVCenter | Qt::AlignRight, label);

    painter->setPen(Qt::NoPen);
    // magic +1 offset in to account for fonts having more empty ascent
    // space than descent space
    QRect dot(dotBox.left() + DOT_MARGIN,
              dotBox.center().y() - (DOT_SIZE / 2) + 1,
              DOT_SIZE,
              DOT_SIZE);
    for (int i=0; i<5; ++i) {
        painter->setBrush((i < value) ? QColor(0x3f, 0x3f, 0x3f) : QColor(0xaf, 0xaf, 0xaf));
        painter->drawEllipse(dot);
        dot.moveLeft(dot.right() + DOT_MARGIN);
    }
}
