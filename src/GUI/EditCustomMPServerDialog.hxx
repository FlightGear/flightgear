#ifndef EDITCUSTOMMPSERVERDIALOG_HXX
#define EDITCUSTOMMPSERVERDIALOG_HXX

#include <QDialog>

namespace Ui {
class EditCustomMPServerDialog;
}

class QComboBox;

class EditCustomMPServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCustomMPServerDialog(QWidget *parent = 0);
    ~EditCustomMPServerDialog();

    QString hostname() const;

    virtual void accept();

    static void addCustomItem(QComboBox* combo);
private:
    Ui::EditCustomMPServerDialog *ui;
};

#endif // EDITCUSTOMMPSERVERDIALOG_HXX
