#include "MPSettings.h"
#include "ui_MPSettings.h"

MPSettings::MPSettings(QWidget *parent) :
    SettingsSection(parent),
    ui(new Ui::MPSettings)
{
    ui->setupUi(this);
    insertSettingsHeader();
}

MPSettings::~MPSettings()
{
    delete ui;
}
