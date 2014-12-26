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
