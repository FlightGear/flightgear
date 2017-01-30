#ifndef ENVIRONMENTPAGE_H
#define ENVIRONMENTPAGE_H

#include <QWidget>

namespace Ui {
class EnvironmentPage;
}

class EnvironmentPage : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentPage(QWidget *parent = 0);
    ~EnvironmentPage();

private:
    Ui::EnvironmentPage *ui;
};

#endif // ENVIRONMENTPAGE_H
