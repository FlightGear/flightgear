#include "AdditionalSettings.h"

#include "ui_AdditionalSettings.h"

AdditionalSettings::AdditionalSettings(QWidget *parent) :
    SettingsSection(parent),
    ui(new Ui::AdditionalSettings)
{
    ui->setupUi(this);
    insertSettingsHeader();
}

AdditionalSettings::~AdditionalSettings()
{
    delete ui;
}
