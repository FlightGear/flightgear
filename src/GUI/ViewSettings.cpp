#include "ViewSettings.h"
#include "ui_ViewSettings.h"

ViewSettings::ViewSettings(QWidget *parent) :
    SettingsSection(parent),
    ui(new Ui::ViewSettings)
{
    ui->setupUi(this);
    insertSettingsHeader();
}

ViewSettings::~ViewSettings()
{
    delete ui;
}
