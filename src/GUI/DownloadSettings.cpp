#include "DownloadSettings.h"
#include "ui_DownloadSettings.h"

DownloadSettings::DownloadSettings(QWidget *parent) :
    SettingsSection(parent),
    ui(new Ui::DownloadSettings)
{
    ui->setupUi(this);
    insertSettingsHeader();
}

DownloadSettings::~DownloadSettings()
{
    delete ui;
}
