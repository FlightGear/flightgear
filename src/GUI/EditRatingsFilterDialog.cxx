// EditEatingsFilterDialog.cxx - part of GUI launcher using Qt5
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

#include "EditRatingsFilterDialog.hxx"
#include "ui_EditRatingsFilterDialog.h"

EditRatingsFilterDialog::EditRatingsFilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditRatingsFilterDialog)
{
    ui->setupUi(this);
}

EditRatingsFilterDialog::~EditRatingsFilterDialog()
{
    delete ui;
}

void EditRatingsFilterDialog::setRatings(int *ratings)
{
    for (int i=0; i<4; ++i) {
        QAbstractSlider* s = sliderForIndex(i);
        s->setValue(ratings[i]);
    }
}

int EditRatingsFilterDialog::getRating(int index) const
{
    return sliderForIndex(index)->value();
}

QAbstractSlider* EditRatingsFilterDialog::sliderForIndex(int index) const
{
    switch (index) {
        case 0: return ui->fdmSlider;
        case 1: return ui->systemsSlider;
        case 2: return ui->cockpitSlider;
        case 3: return ui->modelSlider;
        default:
            return 0;
    }
}