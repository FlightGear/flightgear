#include "renderingsettings.h"
#include "ui_renderingsettings.h"

RenderingSettings::RenderingSettings(QWidget *parent) :
    SettingsSection(parent),
    ui(new Ui::RenderingSettings)
{
    ui->setupUi(this);
    insertSettingsHeader();
}

RenderingSettings::~RenderingSettings()
{
    delete ui;
}
