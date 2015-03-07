// EditEatingsFilterDialog.hxx - part of GUI launcher using Qt5
//
// Written by James Turner, started December 2014.
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

#ifndef EDITRATINGSFILTERDIALOG_HXX
#define EDITRATINGSFILTERDIALOG_HXX

#include <QDialog>

namespace Ui {
class EditRatingsFilterDialog;
}

class QAbstractSlider;

class EditRatingsFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditRatingsFilterDialog(QWidget *parent = 0);
    ~EditRatingsFilterDialog();

    void setRatings(int* ratings);

    int getRating(int index) const;
private:
    Ui::EditRatingsFilterDialog *ui;

    QAbstractSlider* sliderForIndex(int index) const;
};

#endif // EDITRATINGSFILTERDIALOG_HXX
