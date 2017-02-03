#include "EnvironmentPage.h"
#include "ui_EnvironmentPage.h"

EnvironmentPage::EnvironmentPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnvironmentPage)
{
    ui->setupUi(this);

    ui->weatherSection->insertSettingsHeader();
    ui->timeSection->insertSettingsHeader();
}

EnvironmentPage::~EnvironmentPage()
{
    delete ui;
}
