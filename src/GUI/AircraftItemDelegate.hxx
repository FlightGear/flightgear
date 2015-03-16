// AircraftItemDelegate.hxx - part of GUI launcher using Qt5
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

#ifndef FG_GUI_AIRCRAFT_ITEM_DELEGATE
#define FG_GUI_AIRCRAFT_ITEM_DELEGATE

#include <QStyledItemDelegate>

class QListView;

class AircraftItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static const int MARGIN = 4;
    static const int ARROW_SIZE = 20;

    AircraftItemDelegate(QListView* view);
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

    virtual bool eventFilter( QObject*, QEvent* event );

Q_SIGNALS:
    void variantChanged(const QModelIndex& index);

private:
    QRect leftCycleArrowRect(const QRect& visualRect, const QModelIndex& index) const;
    QRect rightCycleArrowRect(const QRect& visualRect, const QModelIndex& index) const;

    QRect packageButtonRect(const QRect& visualRect, const QModelIndex& index) const;

    void drawRating(QPainter* painter, QString label, const QRect& box, int value) const;

    QListView* m_view;
    QPixmap m_leftArrowIcon,
        m_rightArrowIcon;
};

#endif
