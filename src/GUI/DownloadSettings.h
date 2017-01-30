#ifndef DOWNLOAD_SETTINGS_H
#define DOWNLOAD_SETTINGS_H

#include <GUI/settingssection.h>

namespace Ui {
class DownloadSettings;
}

class DownloadSettings : public SettingsSection
{
    Q_OBJECT

public:
    explicit DownloadSettings(QWidget *parent = 0);
    ~DownloadSettings();

private:
    Ui::DownloadSettings *ui;
};

#endif // DOWNLOAD_SETTINGS_H
