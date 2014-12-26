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